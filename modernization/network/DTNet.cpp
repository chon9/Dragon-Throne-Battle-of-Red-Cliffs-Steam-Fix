#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <initguid.h>
#include <windows.h>
#ifndef DUMMYUNIONNAME1
#define DUMMYUNIONNAME1
#endif
#include "JoinDialog.h"
#include "Protocol.h"
#include "LatencyStats.h"
#include "vendor/directplay/dplay.h"
#include "vendor/steam/isteamnetworkingmessages.h"
#include "vendor/steam/isteamnetworkingutils.h"
#include "vendor/steam/steam_api.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
// The networking adapter must not install HUD drawing hooks. The retired
// replacement resized live controls and could corrupt later native redraws.

static const GUID SteamProvider = {
    0xe9155d1f, 0xd2c3, 0x469b, {0xa2, 0x9e, 0x6a, 0x19, 0x63, 0x42, 0x11, 0x01}};
static std::recursive_mutex NetworkMutex;
#define NET_LOCK std::lock_guard<std::recursive_mutex> networkGuard(NetworkMutex)
static bool SteamReady = false;
static HMODULE Module = nullptr;
static std::string LogPath() {
    char p[MAX_PATH]{};
    GetModuleFileNameA(Module, p, MAX_PATH);
    std::string s = p;
    return s.substr(0, s.find_last_of("\\/") + 1) + "DTNet.log";
}
static void Log(const char *fmt, ...) {
    static std::mutex lock;
    std::lock_guard<std::mutex> guard(lock);
    FILE *f = nullptr;
    fopen_s(&f, LogPath().c_str(), "a");
    if (!f)
        return;
    fprintf(f, "%lu ", GetTickCount());
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}
static HRESULT Unsupported(const char *name) {
    Log("Unsupported DirectPlay call: %s", name);
    return DPERR_UNSUPPORTED;
}
// This game ignores EnumConnections' supplied name for unknown providers and
// instead indexes five built-in localized labels. Use the actual ANSI provider
// name already stored in its provider record, converted into the UI's UTF-16
// buffer. The exact instruction guard limits this to the investigated build.
static void __cdecl CopyProviderLabel(wchar_t *out, const char *name) {
    if (!MultiByteToWideChar(CP_ACP, 0, name, -1, out, 256))
        out[0] = 0;
}
static void __declspec(naked) ProviderLabelHook() {
    __asm {
        mov [esp+28h], esi
        lea eax, [edi-100h]
        push eax
        push dword ptr [esp+28h]
        call CopyProviderLabel
        add esp, 8
        mov eax, 52ad0bh
        jmp eax
    }
}
static void PatchProviderLabel() {
    static bool installed = false;
    if (installed) return;
    char path[MAX_PATH]{};
    GetModuleFileNameA(nullptr,path,MAX_PATH);
    const char* leaf=strrchr(path,'\\');
    if (!leaf || _stricmp(leaf+1,"dragonthrone.exe") || GetModuleHandleW(nullptr)!=(HMODULE)0x400000) return;
    auto* target=reinterpret_cast<uint8_t*>(0x52ac9b);
    const uint8_t expected[]={0x48,0x89,0x74,0x24,0x28};
    if (memcmp(target,expected,sizeof(expected))) {Log("Provider-label instruction guard failed");return;}
    DWORD old=0;
    if (!VirtualProtect(target,5,PAGE_EXECUTE_READWRITE,&old))return;
    uint8_t jump[5]={0xe9};
    int32_t relative=static_cast<int32_t>(reinterpret_cast<uintptr_t>(&ProviderLabelHook)-(reinterpret_cast<uintptr_t>(target)+5));
    memcpy(jump+1,&relative,4);memcpy(target,jump,5);
    DWORD ignored;VirtualProtect(target,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),target,5);
    installed=true;
}
static bool InitSteam() {
    if (SteamReady)
        return true;
    SetEnvironmentVariableW(L"SteamAppId", L"2633500");
    SetEnvironmentVariableW(L"SteamGameId", L"2633500");
    if (!SteamAPI_Init()) {
        Log("Steam initialization failed");
        return false;
    }
    if (!SteamUtils() || SteamUtils()->GetAppID() != 2633500 || !SteamUser() ||
        !SteamUser()->BLoggedOn() || !SteamNetworkingMessages() || !SteamNetworkingUtils()) {
        SteamAPI_Shutdown();
        Log("Steam app/account/interface check failed");
        return false;
    }
    SteamNetworkingUtils()->InitRelayNetworkAccess();
    SteamReady = true;
    Log("Steam ready; protocol 1; app 2633500");
    return true;
}
template <class T> static bool Await(SteamAPICall_t call, T &output) {
    if (call == k_uAPICallInvalid)
        return false;
    DWORD start = GetTickCount();
    bool failed = false;
    while (GetTickCount() - start < 15000) {
        SteamAPI_RunCallbacks();
        if (SteamUtils()->IsAPICallCompleted(call, &failed))
            return !failed &&
                   SteamUtils()->GetAPICallResult(call, &output, sizeof(output), T::k_iCallback,
                                                  &failed) &&
                   !failed;
        Sleep(10);
    }
    return false;
}
static GUID LobbyGuid(uint64_t id) {
    GUID g = SteamProvider;
    memcpy(&g, &id, 8);
    return g;
}
static uint64_t GuidLobby(const GUID &g) {
    GUID check = SteamProvider;
    if (memcmp(g.Data4, check.Data4, 8))
        return 0;
    uint64_t id = 0;
    memcpy(&id, &g, 8);
    return id;
}
static std::string SafeName(const char *s, size_t limit) {
    return s ? std::string(s, strnlen_s(s, limit)) : std::string();
}
static HRESULT CopyOut(const void *src, DWORD count, void *dst, DWORD *bytes) {
    if (!bytes)
        return DPERR_INVALIDPARAM;
    if (!dst || *bytes < count) {
        *bytes = count;
        return DPERR_BUFFERTOOSMALL;
    }
    if (count)
        memcpy(dst, src, count);
    *bytes = count;
    return DP_OK;
}

struct PlayerState {
    dt::Player wire{};
    std::vector<uint8_t> data, localData;
};
struct Event {
    ULONGLONG queuedAt = 0;
    DWORD type = 0, from = 0, to = 0;
    PlayerState player;
    std::vector<uint8_t> bytes;
};
class Provider final : public IDirectPlay4 {
    friend struct ProviderTest;
    LONG refs = 1;
    uint64_t lobby = 0, self = 0, owner = 0;
    DPID local = 0, next = 2;
    bool hosting = false, lost = false, opened = false, askedCode = false;
    HANDLE eventHandle = nullptr;
    dt::SessionInfo session{};
    std::map<DPID, PlayerState> players;
    std::deque<Event> queue;
    size_t queuedBytes = 0;
    DWORD lastMembers = 0;
    dt::LatencyStats latency;
    bool diagnostics = true;
    CCallback<Provider, SteamNetworkingMessagesSessionRequest_t> request;
    CCallback<Provider, SteamNetworkingMessagesSessionFailed_t> failure;
    void OnRequest(SteamNetworkingMessagesSessionRequest_t *p) {
        NET_LOCK;
        if (Member(p->m_identityRemote.GetSteamID64()) &&
            (hosting || p->m_identityRemote.GetSteamID64() == owner))
            SteamNetworkingMessages()->AcceptSessionWithUser(p->m_identityRemote);
    }
    void OnFailure(SteamNetworkingMessagesSessionFailed_t *p) {
        NET_LOCK;
        auto id = p->m_info.m_identityRemote.GetSteamID64();
        if (!hosting && id == owner)
            Lose();
        else if (hosting)
            RemovePeer(id);
    }
    bool Member(uint64_t id) {
        if (!lobby || !id)
            return false;
        for (int i = 0; i < SteamMatchmaking()->GetNumLobbyMembers(CSteamID(lobby)); ++i)
            if (SteamMatchmaking()->GetLobbyMemberByIndex(CSteamID(lobby), i).ConvertToUint64() ==
                id)
                return true;
        return false;
    }
    bool WireSend(uint64_t peer, dt::Kind kind, const void *data, size_t count) {
        if (!peer || peer == self)
            return false;
        auto packet = dt::Packet(kind, lobby, data, count);
        SteamNetworkingIdentity ident{};
        ident.SetSteamID64(peer);
        auto r = SteamNetworkingMessages()->SendMessageToUser(
            ident, packet.data(), static_cast<uint32>(packet.size()),
            k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_NoNagle, dt::Channel);
        if (r != k_EResultOK)
            Log("Send failed: %d", r);
        return r == k_EResultOK;
    }
    void Broadcast(dt::Kind kind, const void *data, size_t count, uint64_t skip = 0) {
        for (int i = 0; i < SteamMatchmaking()->GetNumLobbyMembers(CSteamID(lobby)); ++i) {
            uint64_t peer =
                SteamMatchmaking()->GetLobbyMemberByIndex(CSteamID(lobby), i).ConvertToUint64();
            if (peer != self && peer != skip)
                WireSend(peer, kind, data, count);
        }
    }
    std::vector<uint8_t> Serialize(const PlayerState &p) {
        std::vector<uint8_t> out(sizeof(p.wire) + p.data.size());
        memcpy(out.data(), &p.wire, sizeof(p.wire));
        if (!p.data.empty())
            memcpy(out.data() + sizeof(p.wire), p.data.data(), p.data.size());
        return out;
    }
    void Push(Event e) {
        size_t cost = e.bytes.size() + e.player.data.size() + sizeof(Event);
        if (queue.size() >= 8192 || queuedBytes + cost > 8 * 1024 * 1024) {
            Log("Receive queue limit reached");
            Lose();
            return;
        }
        queuedBytes += cost;
        e.queuedAt = GetTickCount64();
        queue.push_back(std::move(e));
        if (eventHandle)
            SetEvent(eventHandle);
    }
    void Lose() {
        if (lost)
            return;
        lost = true;
        Log("Session lost");
        Event e;
        e.type = DPSYS_SESSIONLOST;
        queue.clear();
        queuedBytes = 0;
        Push(e);
    }
    void Publish() {
        if (!hosting || !lobby)
            return;
        session.currentPlayers = static_cast<uint32_t>(players.size());
        auto hex = dt::Hex(&session, sizeof(session));
        SteamMatchmaking()->SetLobbyData(CSteamID(lobby), "dt_desc", hex.c_str());
        SteamMatchmaking()->SetLobbyJoinable(
            CSteamID(lobby),
            !(session.flags & (DPSESSION_JOINDISABLED | DPSESSION_NEWPLAYERSDISABLED)));
        Broadcast(dt::Session, &session, sizeof(session));
    }
    void Upsert(PlayerState p) {
        auto it = players.find(p.wire.id);
        bool fresh = it == players.end();
        bool nameChanged =
            !fresh && (memcmp(it->second.wire.name, p.wire.name, sizeof(p.wire.name)) ||
                       memcmp(it->second.wire.longName, p.wire.longName, sizeof(p.wire.longName)));
        bool dataChanged = !fresh && it->second.data != p.data;
        if (!fresh)
            p.localData = it->second.localData;
        players[p.wire.id] = p;
        // The game gets its initial roster from EnumPlayers. Creation events
        // are for later arrivals, excluding the newly created player itself.
        if (fresh && local && p.wire.owner != self) {
            Event e;
            e.type = DPSYS_CREATEPLAYERORGROUP;
            e.player = p;
            Push(e);
        } else if (!fresh) {
            if (nameChanged) {
                Event e;
                e.type = DPSYS_SETPLAYERORGROUPNAME;
                e.player = p;
                Push(e);
            }
            if (dataChanged) {
                Event e;
                e.type = DPSYS_SETPLAYERORGROUPDATA;
                e.player = p;
                Push(e);
            }
        }
    }
    void Remove(DPID id) {
        auto it = players.find(id);
        if (it == players.end())
            return;
        Event e;
        e.type = DPSYS_DESTROYPLAYERORGROUP;
        e.player = it->second;
        Push(e);
        players.erase(it);
        if (hosting) {
            Broadcast(dt::Removed, &id, sizeof(id));
            Publish();
        }
    }
    void RemovePeer(uint64_t peer) {
        std::vector<DPID> ids;
        for (auto &kv : players)
            if (kv.second.wire.owner == peer)
                ids.push_back(kv.first);
        for (DPID id : ids)
            Remove(id);
    }
    void AcceptCreate(uint64_t peer, PlayerState p) {
        if (players.size() >= session.maxPlayers ||
            (session.flags & (DPSESSION_NEWPLAYERSDISABLED | DPSESSION_JOINDISABLED)))
            return;
        for (auto &kv : players)
            if (kv.second.wire.owner == peer)
                return;
        if (peer != self) {
            for (auto &kv : players) {
                auto b = Serialize(kv.second);
                WireSend(peer, dt::PlayerInfo, b.data(), b.size());
            }
        }
        p.wire.owner = peer;
        p.wire.id = (peer == self) ? 1 : next++;
        p.wire.flags =
            (p.wire.flags & DPPLAYER_SPECTATOR) | ((peer == self) ? DPPLAYER_SERVERPLAYER : 0);
        auto b = Serialize(p);
        Upsert(p);
        Broadcast(dt::PlayerInfo, b.data(), b.size());
        if (peer == self)
            local = p.wire.id;
        else
            WireSend(peer, dt::Ready, &p.wire.id, sizeof(p.wire.id));
        Publish();
        Log("Player created: %lu; count %u", p.wire.id, static_cast<unsigned>(players.size()));
    }
    void Route(uint64_t peer, const uint8_t *payload, size_t count) {
        if (count < sizeof(dt::DataHeader))
            return;
        dt::DataHeader h{};
        memcpy(&h, payload, sizeof(h));
        auto from = players.find(h.from);
        if (from == players.end() || (hosting && from->second.wire.owner != peer))
            return;
        if (h.to && !players.count(h.to))
            return;
        if (local && h.from != local && (!h.to || h.to == local)) {
            Event e;
            e.from = h.from;
            e.to = local;
            e.bytes.assign(payload + sizeof(h), payload + count);
            Push(std::move(e));
        }
        if (hosting) {
            for (auto &kv : players)
                if (kv.second.wire.owner != peer && kv.second.wire.owner != self &&
                    (!h.to || h.to == kv.first))
                    WireSend(kv.second.wire.owner, dt::Data, payload, count);
        }
    }
    void Handle(uint64_t peer, const void *data, size_t count) {
        dt::Header h{};
        const uint8_t *p = nullptr;
        if (!Member(peer) || (!hosting && peer != owner) || !dt::Parse(data, count, lobby, h, p))
            return;
        if (h.kind == dt::Create && hosting) {
            PlayerState s;
            if (!dt::ReadPlayer(p, h.size, s.wire))
                return;
            s.data.assign(p + sizeof(s.wire), p + h.size);
            AcceptCreate(peer, s);
        } else if (h.kind == dt::PlayerInfo && !hosting) {
            PlayerState s;
            if (!dt::ReadPlayer(p, h.size, s.wire) || !s.wire.id || !Member(s.wire.owner) ||
                s.wire.id > 1024 || (!players.count(s.wire.id) && players.size() >= dt::MaxPlayers))
                return;
            s.data.assign(p + sizeof(s.wire), p + h.size);
            Upsert(s);
        } else if (h.kind == dt::UpdatePlayer && hosting) {
            PlayerState s;
            if (!dt::ReadPlayer(p, h.size, s.wire))
                return;
            auto it = players.find(s.wire.id);
            if (it == players.end() || it->second.wire.owner != peer)
                return;
            s.wire.owner = peer;
            s.wire.flags = it->second.wire.flags;
            s.data.assign(p + sizeof(s.wire), p + h.size);
            Upsert(s);
            auto b = Serialize(s);
            Broadcast(dt::PlayerInfo, b.data(), b.size());
        } else if (h.kind == dt::Ready && !hosting && h.size == sizeof(DPID)) {
            DPID id;
            memcpy(&id, p, sizeof(id));
            if (players.count(id) && players[id].wire.owner == self)
                local = id;
        } else if ((h.kind == dt::RemoveRequest && hosting) ||
                   (h.kind == dt::Removed && !hosting)) {
            if (h.size != sizeof(DPID))
                return;
            DPID id;
            memcpy(&id, p, sizeof(id));
            auto it = players.find(id);
            if (it != players.end() && (!hosting || it->second.wire.owner == peer))
                Remove(id);
        } else if (h.kind == dt::Data)
            Route(peer, p, h.size);
        else if (h.kind == dt::Session && !hosting && h.size == sizeof(session)) {
            dt::SessionInfo s;
            memcpy(&s, p, sizeof(s));
            if (s.maxPlayers >= 1 && s.maxPlayers <= dt::MaxPlayers &&
                s.currentPlayers <= s.maxPlayers && memchr(s.name, 0, sizeof(s.name)))
                session = s;
        }
    }
    void ReportLatency() {
        if (!diagnostics || !opened || players.size() < 2 || !latency.ReportDue(GetTickCount())) return;
        Log("Latency local: sends=%llu delivered=%llu poll_ms=%.2f/%.2f steam_inbox_ms=%.2f/%.2f game_inbox_ms=%.2f/%.2f queue=%u bytes=%u (mean/max; not click-to-action time)",
            latency.sent, latency.delivered, latency.pollGap.MeanMs(), latency.pollGap.MaxMs(),
            latency.steamInbox.MeanMs(), latency.steamInbox.MaxMs(), latency.gameInbox.MeanMs(),
            latency.gameInbox.MaxMs(), unsigned(queue.size()), unsigned(queuedBytes));
        for (const auto &kv : players) {
            const auto peer = kv.second.wire.owner;
            if (peer == self || (!hosting && peer != owner)) continue;
            SteamNetworkingIdentity ident{}; ident.SetSteamID64(peer);
            SteamNetConnectionRealTimeStatus_t status{};
            auto state = SteamNetworkingMessages()->GetSessionConnectionInfo(ident,nullptr,&status);
            Log("Latency player=%lu state=%d ping_ms=%d send_queue_ms=%.2f pending_reliable=%d unacked=%d quality_local=%.3f quality_remote=%.3f",
                kv.first, int(state), status.m_nPing, double(status.m_usecQueueTime)/1000.0,
                status.m_cbPendingReliable, status.m_cbSentUnackedReliable,
                status.m_flConnectionQualityLocal, status.m_flConnectionQualityRemote);
        }
        latency.ResetWindow();
    }
    void Pump() {
        if (!SteamReady)
            return;
        SteamAPI_RunCallbacks();
        if (!lobby || lost)
            return;
        if (diagnostics && opened && players.size() >= 2) latency.Poll(GetTickCount());
        SteamNetworkingMessage_t *messages[32];
        for (int batch = 0; batch < 8; ++batch) {
            int n = SteamNetworkingMessages()->ReceiveMessagesOnChannel(dt::Channel, messages, 32);
            for (int i = 0; i < n; ++i) {
                if (diagnostics && messages[i]->m_usecTimeReceived > 0)
                    latency.steamInbox.Add(SteamNetworkingUtils()->GetLocalTimestamp()-messages[i]->m_usecTimeReceived);
                Handle(messages[i]->m_identityPeer.GetSteamID64(), messages[i]->m_pData,
                       static_cast<size_t>(messages[i]->m_cbSize));
                messages[i]->Release();
            }
            if (n < 32)
                break;
        }
        if (GetTickCount() - lastMembers > 500) {
            lastMembers = GetTickCount();
            if (SteamMatchmaking()->GetLobbyOwner(CSteamID(lobby)).ConvertToUint64() != owner) {
                Lose();
                return;
            }
            if (hosting) {
                std::vector<uint64_t> gone;
                for (auto &kv : players)
                    if (!Member(kv.second.wire.owner))
                        gone.push_back(kv.second.wire.owner);
                for (auto id : gone)
                    RemovePeer(id);
            }
        }
        ReportLatency();
    }
    HRESULT Join(uint64_t id) {
        if (lobby == id)
            return DP_OK;
        if (lobby)
            return DPERR_ALREADYINITIALIZED;
        CSteamID sid(id);
        if (!sid.IsValid() || !sid.IsLobby())
            return DPERR_INVALIDPARAM;
        LobbyEnter_t r{};
        if (!Await(SteamMatchmaking()->JoinLobby(sid), r) ||
            r.m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess)
            return DPERR_NOCONNECTION;
        if (strcmp(SteamMatchmaking()->GetLobbyData(sid, "dt_protocol"), "dt-steam-1") ||
            !dt::Unhex(SteamMatchmaking()->GetLobbyData(sid, "dt_desc"), &session,
                       sizeof(session)) ||
            !memchr(session.name, 0, sizeof(session.name)) || session.maxPlayers < 1 ||
            session.maxPlayers > dt::MaxPlayers || session.currentPlayers > session.maxPlayers) {
            SteamMatchmaking()->LeaveLobby(sid);
            return DPERR_UNKNOWNAPPLICATION;
        }
        lobby = id;
        owner = SteamMatchmaking()->GetLobbyOwner(sid).ConvertToUint64();
        self = SteamUser()->GetSteamID().ConvertToUint64();
        hosting = false;
        lost = false;
        return DP_OK;
    }
    DPSESSIONDESC2 Description() {
        DPSESSIONDESC2 d{};
        d.dwSize = sizeof(d);
        d.dwFlags = session.flags;
        d.guidInstance = LobbyGuid(lobby);
        memcpy(&d.guidApplication, session.application, 16);
        d.dwMaxPlayers = session.maxPlayers;
        d.dwCurrentPlayers = session.currentPlayers;
        d.lpszSessionNameA = session.name;
        d.dwUser1 = session.user[0];
        d.dwUser2 = session.user[1];
        d.dwUser3 = session.user[2];
        d.dwUser4 = session.user[3];
        return d;
    }
    void ReadDescription(const DPSESSIONDESC2 &d) {
        session.flags = d.dwFlags;
        session.maxPlayers = std::min<DWORD>(dt::MaxPlayers, d.dwMaxPlayers ? d.dwMaxPlayers : 8);
        memcpy(session.application, &d.guidApplication, 16);
        session.user[0] = d.dwUser1;
        session.user[1] = d.dwUser2;
        session.user[2] = d.dwUser3;
        session.user[3] = d.dwUser4;
        auto name = SafeName(d.lpszSessionNameA, sizeof(session.name) - 1);
        memset(session.name, 0, sizeof(session.name));
        memcpy(session.name, name.data(), name.size());
    }

  public:
    Provider() : request(this, &Provider::OnRequest), failure(this, &Provider::OnFailure) {
        std::string config=LogPath(); config.replace(config.size()-3,3,"ini");
        diagnostics=GetPrivateProfileIntA("Diagnostics","Latency",1,config.c_str())!=0;
        Log("DirectPlay4A adapter created");
    }
    ~Provider() { Close(); }
    HRESULT WINAPI QueryInterface(REFIID id, void **out) override {
        if (!out)
            return E_POINTER;
        *out = nullptr;
        if (id == IID_IUnknown || id == IID_IDirectPlay4A || id == IID_IDirectPlay3A ||
            id == IID_IDirectPlay2A) {
            *out = static_cast<IDirectPlay4 *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG WINAPI AddRef() override { return InterlockedIncrement(&refs); }
    ULONG WINAPI Release() override {
        NET_LOCK;
        LONG n = InterlockedDecrement(&refs);
        if (!n)
            delete this;
        return n;
    }
#include "DirectPlayMethods.inc"
};

HRESULT WINAPI Provider::InitializeConnection(void *, DWORD) {
    NET_LOCK;
    return InitSteam() ? DP_OK : DPERR_UNAVAILABLE;
}
HRESULT WINAPI Provider::Initialize(LPGUID) {
    NET_LOCK;
    return InitializeConnection(nullptr, 0);
}
HRESULT WINAPI Provider::EnumConnections(LPCGUID, LPDPENUMCONNECTIONSCALLBACK cb, void *context,
                                         DWORD) {
    NET_LOCK;
    if (!cb)
        return DPERR_INVALIDPARAM;
    DPNAME n{};
    n.dwSize = sizeof(n);
    n.lpszShortNameA = const_cast<char *>("Steam - Join codes");
    GUID blob = SteamProvider;
    cb(&SteamProvider, &blob, sizeof(blob), &n, DPCONNECTION_DIRECTPLAY, context);
    return DP_OK;
}
HRESULT WINAPI Provider::Open(DPSESSIONDESC2 *desc, DWORD flags) {
    NET_LOCK;
    Log("Open flags=%lu", flags);
    if (!desc || desc->dwSize != sizeof(*desc))
        return DPERR_INVALIDPARAM;
    if (!InitSteam())
        return DPERR_UNAVAILABLE;
    if (opened)
        return DPERR_ALREADYINITIALIZED;
    if (flags & DPOPEN_CREATE) {
        if (lobby)
            Close();
        if (desc->lpszPasswordA && *desc->lpszPasswordA)
            return DPERR_UNSUPPORTED;
        ReadDescription(*desc);
        LobbyCreated_t created{};
        if (!Await(SteamMatchmaking()->CreateLobby(k_ELobbyTypeInvisible, session.maxPlayers),
                   created) ||
            created.m_eResult != k_EResultOK)
            return DPERR_CANTCREATESESSION;
        lobby = created.m_ulSteamIDLobby;
        self = owner = SteamUser()->GetSteamID().ConvertToUint64();
        hosting = true;
        lost = false;
        SteamMatchmaking()->SetLobbyData(CSteamID(lobby), "dt_protocol", "dt-steam-1");
        Publish();
        desc->guidInstance = LobbyGuid(lobby);
        opened = true;
        Log("Host session created");
        if (!GetEnvironmentVariableA("DTNET_TEST", nullptr, 0)) {
            uint64_t unused = 0;
            dt::JoinDialog(true, dt::Code(lobby), unused);
        }
        return DP_OK;
    }
    if (flags & DPOPEN_JOIN) {
        HRESULT r = Join(GuidLobby(desc->guidInstance));
        if (SUCCEEDED(r)) {
            if (memcmp(session.application, &desc->guidApplication, 16)) {
                Close();
                return DPERR_UNKNOWNAPPLICATION;
            }
            opened = true;
        }
        return r;
    }
    return DPERR_INVALIDFLAGS;
}
HRESULT WINAPI Provider::Close() {
    NET_LOCK;
    if (lobby && SteamReady) {
        if (hosting)
            SteamMatchmaking()->SetLobbyJoinable(CSteamID(lobby), false);
        for (auto &kv : players) {
            if (kv.second.wire.owner != self) {
                SteamNetworkingIdentity ident{};
                ident.SetSteamID64(kv.second.wire.owner);
                SteamNetworkingMessages()->CloseChannelWithUser(ident, dt::Channel);
            }
        }
        SteamMatchmaking()->LeaveLobby(CSteamID(lobby));
        Log("Session closed");
    }
    lobby = owner = self = 0;
    local = 0;
    next = 2;
    hosting = opened = lost = askedCode = false;
    eventHandle = nullptr;
    players.clear();
    queue.clear();
    queuedBytes = 0;
    session = {};
    latency = {};
    return DP_OK;
}
HRESULT WINAPI Provider::EnumSessions(DPSESSIONDESC2 *desc, DWORD, LPDPENUMSESSIONSCALLBACK2 cb,
                                      void *context, DWORD flags) {
    NET_LOCK;
    if (flags & DPENUMSESSIONS_STOPASYNC)
        return DP_OK;
    if (!desc || !cb)
        return DPERR_INVALIDPARAM;
    if (!InitSteam())
        return DPERR_UNAVAILABLE;
    if (!lobby && !askedCode) {
        askedCode = true;
        uint64_t id = 0;
        if (!dt::JoinDialog(false, "", id))
            return DP_OK; // Host selection: keep this provider initialized.
        HRESULT r = Join(id);
        if (FAILED(r)) {
            MessageBoxW(GetActiveWindow(),
                        L"Could not join this game. Check that the host is still in the lobby and "
                        L"both players have the same multiplayer build.",
                        L"Unable to join", MB_OK | MB_ICONINFORMATION);
            askedCode = false;
            return r;
        }
    }
    Pump();
    if (lobby && !lost) {
        if (memcmp(session.application, &desc->guidApplication, 16))
            return DPERR_UNKNOWNAPPLICATION;
        auto d = Description();
        DWORD timeout = 0;
        cb(&d, &timeout, 0, context);
    }
    return DP_OK;
}
HRESULT WINAPI Provider::CreatePlayer(DPID *id, DPNAME *name, HANDLE ev, void *data, DWORD size,
                                      DWORD flags) {
    NET_LOCK;
    if (!id || !name || size > dt::MaxPlayerData || (size && !data))
        return DPERR_INVALIDPARAM;
    *id = DPID_UNKNOWN;
    if (!opened || lost)
        return DPERR_NOCONNECTION;
    if (local)
        return DPERR_CANTCREATEPLAYER;
    PlayerState p;
    p.wire.owner = self;
    p.wire.flags = flags;
    p.wire.size = size;
    auto s = SafeName(name->lpszShortNameA, sizeof(p.wire.name) - 1),
         l = SafeName(name->lpszLongNameA, sizeof(p.wire.longName) - 1);
    memcpy(p.wire.name, s.data(), s.size());
    memcpy(p.wire.longName, l.data(), l.size());
    if (size)
        p.data.assign((uint8_t *)data, (uint8_t *)data + size);
    eventHandle = ev;
    if (hosting)
        AcceptCreate(self, p);
    else {
        auto b = Serialize(p);
        if (!WireSend(owner, dt::Create, b.data(), b.size()))
            return DPERR_NOCONNECTION;
        DWORD start = GetTickCount();
        while (!local && !lost && GetTickCount() - start < 15000) {
            Pump();
            Sleep(5);
        }
    }
    if (!local)
        return DPERR_CANTCREATEPLAYER;
    *id = local;
    return DP_OK;
}
HRESULT WINAPI Provider::DestroyPlayer(DPID id) {
    NET_LOCK;
    if (!players.count(id) || players[id].wire.owner != self)
        return DPERR_INVALIDPLAYER;
    if (hosting)
        Remove(id);
    else {
        if (!WireSend(owner, dt::RemoveRequest, &id, sizeof(id)))
            return DPERR_NOCONNECTION;
        Remove(id);
    }
    if (id == local)
        local = 0;
    return DP_OK;
}
HRESULT WINAPI Provider::EnumPlayers(LPGUID, LPDPENUMPLAYERSCALLBACK2 cb, void *ctx, DWORD flags) {
    NET_LOCK;
    if (!cb)
        return DPERR_INVALIDPARAM;
    Pump();
    for (auto &kv : players) {
        auto &p = kv.second;
        bool mine = p.wire.owner == self;
        if ((flags & DPENUMPLAYERS_LOCAL) && !mine)
            continue;
        if ((flags & DPENUMPLAYERS_REMOTE) && mine)
            continue;
        DPNAME n{};
        n.dwSize = sizeof(n);
        n.lpszShortNameA = p.wire.name;
        n.lpszLongNameA = p.wire.longName;
        DWORD f = p.wire.flags | (mine ? DPPLAYER_LOCAL : 0);
        if (!cb(kv.first, DPPLAYERTYPE_PLAYER, &n, f, ctx))
            break;
    }
    return DP_OK;
}
HRESULT WINAPI Provider::GetCaps(DPCAPS *c, DWORD) {
    NET_LOCK;
    if (!c || c->dwSize != sizeof(*c))
        return DPERR_INVALIDPARAM;
    *c = {};
    c->dwSize = sizeof(*c);
    c->dwFlags =
        DPCAPS_GUARANTEEDSUPPORTED | DPCAPS_GUARANTEEDOPTIMIZED | (hosting ? DPCAPS_ISHOST : 0);
    c->dwMaxBufferSize = dt::MaxData;
    c->dwMaxQueueSize = 8 * 1024 * 1024;
    c->dwMaxPlayers = dt::MaxPlayers;
    c->dwMaxLocalPlayers = 1;
    c->dwHundredBaud = 100000;
    c->dwTimeout = 15000;
    return DP_OK;
}
HRESULT WINAPI Provider::GetPlayerCaps(DPID id, DPCAPS *c, DWORD f) {
    NET_LOCK;
    return players.count(id) ? GetCaps(c, f) : DPERR_INVALIDPLAYER;
}
HRESULT WINAPI Provider::GetPlayerFlags(DPID id, DWORD *f) {
    NET_LOCK;
    if (!f)
        return DPERR_INVALIDPARAM;
    auto it = players.find(id);
    if (it == players.end())
        return DPERR_INVALIDPLAYER;
    *f = it->second.wire.flags | (it->second.wire.owner == self ? DPPLAYER_LOCAL : 0);
    return DP_OK;
}
HRESULT WINAPI Provider::GetSessionDesc(void *data, DWORD *size) {
    NET_LOCK;
    if (!lobby)
        return DPERR_NOSESSIONS;
    Pump();
    auto d = Description();
    DWORD need = sizeof(d) + static_cast<DWORD>(strlen(session.name)) + 1;
    if (!size)
        return DPERR_INVALIDPARAM;
    if (!data || *size < need) {
        *size = need;
        return DPERR_BUFFERTOOSMALL;
    }
    d.lpszSessionNameA = (char *)data + sizeof(d);
    memcpy(data, &d, sizeof(d));
    strcpy_s(d.lpszSessionNameA, need - sizeof(d), session.name);
    *size = need;
    return DP_OK;
}
HRESULT WINAPI Provider::SetSessionDesc(DPSESSIONDESC2 *d, DWORD) {
    NET_LOCK;
    if (!d || d->dwSize != sizeof(*d))
        return DPERR_INVALIDPARAM;
    if (!hosting)
        return DPERR_ACCESSDENIED;
    if (d->dwMaxPlayers && d->dwMaxPlayers < players.size())
        return DPERR_INVALIDPARAM;
    ReadDescription(*d);
    Publish();
    return DP_OK;
}
HRESULT WINAPI Provider::GetPlayerData(DPID id, void *data, DWORD *size, DWORD flags) {
    NET_LOCK;
    auto it = players.find(id);
    if (it == players.end())
        return DPERR_INVALIDPLAYER;
    auto &b = (flags & DPGET_LOCAL) ? it->second.localData : it->second.data;
    return CopyOut(b.data(), static_cast<DWORD>(b.size()), data, size);
}
HRESULT WINAPI Provider::SetPlayerData(DPID id, void *data, DWORD size, DWORD flags) {
    NET_LOCK;
    auto it = players.find(id);
    if (it == players.end())
        return DPERR_INVALIDPLAYER;
    if (size > dt::MaxPlayerData || (size && !data))
        return DPERR_INVALIDPARAM;
    if (!(flags & DPSET_LOCAL) && it->second.wire.owner != self)
        return DPERR_ACCESSDENIED;
    auto p = it->second;
    auto &b = (flags & DPSET_LOCAL) ? p.localData : p.data;
    b.clear();
    if (size)
        b.assign((uint8_t *)data, (uint8_t *)data + size);
    if (flags & DPSET_LOCAL) {
        it->second = p;
        return DP_OK;
    }
    p.wire.size = size;
    if (hosting) {
        Upsert(p);
        auto bytes = Serialize(p);
        Broadcast(dt::PlayerInfo, bytes.data(), bytes.size());
        return DP_OK;
    }
    auto bytes = Serialize(p);
    return WireSend(owner, dt::UpdatePlayer, bytes.data(), bytes.size()) ? DP_OK
                                                                         : DPERR_NOCONNECTION;
}
HRESULT WINAPI Provider::GetPlayerName(DPID id, void *data, DWORD *size) {
    NET_LOCK;
    auto it = players.find(id);
    if (it == players.end())
        return DPERR_INVALIDPLAYER;
    if (!size)
        return DPERR_INVALIDPARAM;
    auto &p = it->second.wire;
    DWORD a = static_cast<DWORD>(strlen(p.name) + 1),
          b = static_cast<DWORD>(strlen(p.longName) + 1), need = sizeof(DPNAME) + a + b;
    if (!data || *size < need) {
        *size = need;
        return DPERR_BUFFERTOOSMALL;
    }
    DPNAME n{};
    n.dwSize = sizeof(n);
    n.lpszShortNameA = (char *)data + sizeof(n);
    n.lpszLongNameA = n.lpszShortNameA + a;
    memcpy(data, &n, sizeof(n));
    memcpy(n.lpszShortNameA, p.name, a);
    memcpy(n.lpszLongNameA, p.longName, b);
    *size = need;
    return DP_OK;
}
HRESULT WINAPI Provider::SetPlayerName(DPID id, DPNAME *n, DWORD) {
    NET_LOCK;
    auto it = players.find(id);
    if (it == players.end())
        return DPERR_INVALIDPLAYER;
    if (!n)
        return DPERR_INVALIDPARAM;
    if (it->second.wire.owner != self)
        return DPERR_ACCESSDENIED;
    auto p = it->second;
    memset(p.wire.name, 0, sizeof(p.wire.name));
    memset(p.wire.longName, 0, sizeof(p.wire.longName));
    auto s = SafeName(n->lpszShortNameA, sizeof(p.wire.name) - 1),
         l = SafeName(n->lpszLongNameA, sizeof(p.wire.longName) - 1);
    memcpy(p.wire.name, s.data(), s.size());
    memcpy(p.wire.longName, l.data(), l.size());
    auto b = Serialize(p);
    if (hosting) {
        Upsert(p);
        Broadcast(dt::PlayerInfo, b.data(), b.size());
        return DP_OK;
    }
    return WireSend(owner, dt::UpdatePlayer, b.data(), b.size()) ? DP_OK : DPERR_NOCONNECTION;
}
HRESULT WINAPI Provider::Send(DPID from, DPID to, DWORD flags, void *data, DWORD size) {
    NET_LOCK;
    Pump();
    if (!opened || lost)
        return DPERR_NOCONNECTION;
    if (size > dt::MaxData)
        return DPERR_SENDTOOBIG;
    if (size && !data)
        return DPERR_INVALIDPARAM;
    if (!players.count(from) || players[from].wire.owner != self || (to && !players.count(to)))
        return DPERR_INVALIDPLAYER;
    if (flags & (DPSEND_SIGNED | DPSEND_ENCRYPTED))
        return DPERR_UNSUPPORTED;
    if (diagnostics) ++latency.sent;
    if (to == from) {
        Event e;
        e.from = from;
        e.to = to;
        if (size)
            e.bytes.assign((uint8_t *)data, (uint8_t *)data + size);
        Push(e);
        return DP_OK;
    }
    dt::DataHeader h{from, to};
    std::vector<uint8_t> b(sizeof(h) + size);
    memcpy(b.data(), &h, sizeof(h));
    if (size)
        memcpy(b.data() + sizeof(h), data, size);
    if (hosting) {
        Route(self, b.data(), b.size());
        return DP_OK;
    }
    return WireSend(owner, dt::Data, b.data(), b.size()) ? DP_OK : DPERR_NOCONNECTION;
}
HRESULT WINAPI Provider::SendEx(DPID f, DPID t, DWORD flags, void *b, DWORD n, DWORD, DWORD, void *,
                                DWORD *id) {
    NET_LOCK;
    if (id)
        *id = 0;
    return Send(f, t, flags, b, n);
}
HRESULT WINAPI Provider::CancelMessage(DWORD, DWORD) {
    NET_LOCK;
    return DPERR_CANCELFAILED;
}
HRESULT WINAPI Provider::CancelPriority(DWORD, DWORD, DWORD) {
    NET_LOCK;
    return DPERR_CANCELFAILED;
}
HRESULT WINAPI Provider::GetMessageCount(DPID id, DWORD *n) {
    NET_LOCK;
    if (!n)
        return DPERR_INVALIDPARAM;
    Pump();
    *n = 0;
    for (auto &e : queue)
        if (!id || e.to == id || (!e.to && id == local))
            ++*n;
    return DP_OK;
}
HRESULT WINAPI Provider::GetMessageQueue(DPID f, DPID t, DWORD flags, DWORD *count, DWORD *bytes) {
    NET_LOCK;
    if (!count && !bytes)
        return DPERR_INVALIDPARAM;
    Pump();
    DWORD n = 0, b = 0;
    if (flags & DPMESSAGEQUEUE_RECEIVE)
        for (auto &e : queue)
            if ((!f || e.from == f) && (!t || e.to == t)) {
                ++n;
                b += static_cast<DWORD>(e.bytes.size());
            }
    if (count)
        *count = n;
    if (bytes)
        *bytes = b;
    return DP_OK;
}
HRESULT WINAPI Provider::Receive(DPID *from, DPID *to, DWORD flags, void *data, DWORD *size) {
    NET_LOCK;
    if (!from || !to || !size)
        return DPERR_INVALIDPARAM;
    Pump();
    auto it = queue.begin();
    for (; it != queue.end(); ++it) {
        DPID dest = it->to ? it->to : local;
        if ((flags & DPRECEIVE_FROMPLAYER) && it->from != *from)
            continue;
        if ((flags & DPRECEIVE_TOPLAYER) && dest != *to)
            continue;
        break;
    }
    if (it == queue.end())
        return DPERR_NOMESSAGES;
    auto &e = *it;
    DWORD need = 0;
    size_t header = 0;
    auto &p = e.player;
    switch (e.type) {
    case 0:
        need = static_cast<DWORD>(e.bytes.size());
        break;
    case DPSYS_CREATEPLAYERORGROUP:
        header = sizeof(DPMSG_CREATEPLAYERORGROUP);
        break;
    case DPSYS_DESTROYPLAYERORGROUP:
        header = sizeof(DPMSG_DESTROYPLAYERORGROUP);
        break;
    case DPSYS_SETPLAYERORGROUPNAME:
        header = sizeof(DPMSG_SETPLAYERORGROUPNAME);
        break;
    case DPSYS_SETPLAYERORGROUPDATA:
        header = sizeof(DPMSG_SETPLAYERORGROUPDATA);
        break;
    default:
        need = sizeof(DPMSG_GENERIC);
        break;
    }
    if (header)
        need = static_cast<DWORD>(header + strlen(p.wire.name) + 1 + strlen(p.wire.longName) + 1 +
                                  p.data.size());
    if (!data || *size < need) {
        *size = need;
        return DPERR_BUFFERTOOSMALL;
    }
    memset(data, 0, need);
    if (!e.type) {
        if (need)
            memcpy(data, e.bytes.data(), need);
    } else if (!header) {
        ((DPMSG_GENERIC *)data)->dwType = e.type;
    } else {
        char *shortName = (char *)data + header;
        size_t a = strlen(p.wire.name) + 1, b = strlen(p.wire.longName) + 1;
        char *longName = shortName + a;
        void *playerData = longName + b;
        memcpy(shortName, p.wire.name, a);
        memcpy(longName, p.wire.longName, b);
        if (!p.data.empty())
            memcpy(playerData, p.data.data(), p.data.size());
        DPNAME name{};
        name.dwSize = sizeof(name);
        name.lpszShortNameA = shortName;
        name.lpszLongNameA = longName;
        DWORD pf = p.wire.flags | (p.wire.owner == self ? DPPLAYER_LOCAL : 0);
        if (e.type == DPSYS_CREATEPLAYERORGROUP) {
            auto *m = (DPMSG_CREATEPLAYERORGROUP *)data;
            m->dwType = e.type;
            m->dwPlayerType = DPPLAYERTYPE_PLAYER;
            m->dpId = p.wire.id;
            m->dwCurrentPlayers = static_cast<DWORD>(players.size());
            m->lpData = playerData;
            m->dwDataSize = static_cast<DWORD>(p.data.size());
            m->dpnName = name;
            m->dwFlags = pf;
        } else if (e.type == DPSYS_DESTROYPLAYERORGROUP) {
            auto *m = (DPMSG_DESTROYPLAYERORGROUP *)data;
            m->dwType = e.type;
            m->dwPlayerType = DPPLAYERTYPE_PLAYER;
            m->dpId = p.wire.id;
            m->lpRemoteData = playerData;
            m->dwRemoteDataSize = static_cast<DWORD>(p.data.size());
            m->dpnName = name;
            m->dwFlags = pf;
        } else if (e.type == DPSYS_SETPLAYERORGROUPNAME) {
            auto *m = (DPMSG_SETPLAYERORGROUPNAME *)data;
            m->dwType = e.type;
            m->dwPlayerType = DPPLAYERTYPE_PLAYER;
            m->dpId = p.wire.id;
            m->dpnName = name;
        } else {
            auto *m = (DPMSG_SETPLAYERORGROUPDATA *)data;
            m->dwType = e.type;
            m->dwPlayerType = DPPLAYERTYPE_PLAYER;
            m->dpId = p.wire.id;
            m->lpData = playerData;
            m->dwDataSize = static_cast<DWORD>(p.data.size());
        }
    }
    *size = need;
    *from = e.from;
    *to = e.to ? e.to : local;
    if (!(flags & DPRECEIVE_PEEK)) {
        if (diagnostics && !e.type) {
            latency.gameInbox.Add(static_cast<int64_t>(GetTickCount64()-e.queuedAt)*1000);
            ++latency.delivered;
        }
        queuedBytes -= e.bytes.size() + e.player.data.size() + sizeof(Event);
        queue.erase(it);
        if (eventHandle && queue.empty())
            ResetEvent(eventHandle);
    }
    return DP_OK;
}

extern "C" HRESULT WINAPI DTCoCreateInstance(REFCLSID cls, LPUNKNOWN outer, DWORD context,
                                             REFIID iid, void **out) {
    NET_LOCK;
    PatchProviderLabel();
    if (cls == CLSID_DirectPlay && iid == IID_IDirectPlay4A) {
        if (!out)
            return E_POINTER;
        *out = nullptr;
        if (outer)
            return CLASS_E_NOAGGREGATION;
        try {
            *out = static_cast<IDirectPlay4 *>(new Provider);
            return S_OK;
        } catch (...) {
            return E_OUTOFMEMORY;
        }
    }
    return CoCreateInstance(cls, outer, context, iid, out);
}
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        Module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
