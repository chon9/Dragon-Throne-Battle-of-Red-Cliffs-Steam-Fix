#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <initguid.h>
#include <windows.h>
#ifndef DUMMYUNIONNAME1
#define DUMMYUNIONNAME1
#endif
#include "Protocol.h"
#include "vendor/directplay/dplay.h"
#include "vendor/steam/steam_api.h"
#include <cstdio>
#include <vector>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            printf("FAIL line %d: %s\n", __LINE__, #x);                                            \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
static BOOL CALLBACK Connection(LPCGUID, void *blob, DWORD size, LPCDPNAME name, DWORD, void *ctx) {
    if (size == 16 && name->lpszShortNameA && strstr(name->lpszShortNameA, "Steam"))
        ++*(int *)ctx;
    return TRUE;
}
static BOOL CALLBACK Player(DPID, DWORD, LPCDPNAME name, DWORD flags, void *ctx) {
    if (!strcmp(name->lpszShortNameA, "Test player") && (flags & DPPLAYER_LOCAL))
        ++*(int *)ctx;
    return TRUE;
}
int main() {
    SetEnvironmentVariableA("DTNET_TEST", "1");
    HMODULE dll = LoadLibraryW(L"DTNet.dll");
    CHECK(dll);
    using Factory = HRESULT(WINAPI *)(REFCLSID, LPUNKNOWN, DWORD, REFIID, void **);
    auto create = (Factory)GetProcAddress(dll, "CoCreateInstance");
    CHECK(create);
    CHECK(GetProcAddress(dll, "CoInitialize"));
    CHECK(GetProcAddress(dll, "CoUninitialize"));
    IDirectPlay4 *dp = nullptr;
    CHECK(create(CLSID_DirectPlay, 0, CLSCTX_INPROC_SERVER, IID_IDirectPlay4A, (void **)&dp) ==
          DP_OK);
    void *unknown = nullptr;
    CHECK(dp->QueryInterface(IID_IUnknown, &unknown) == S_OK);
    ((IUnknown *)unknown)->Release();
    CHECK(dp->InitializeConnection(nullptr, 0) == DP_OK);
    int connections = 0;
    CHECK(dp->EnumConnections(0, Connection, &connections, 0) == DP_OK && connections == 1);
    DPCAPS caps{};
    caps.dwSize = sizeof(caps);
    CHECK(dp->GetCaps(&caps, 0) == DP_OK);
    CHECK(caps.dwMaxBufferSize == dt::MaxData && !(caps.dwFlags & DPCAPS_ASYNCSUPPORTED));
    DPSESSIONDESC2 d{};
    d.dwSize = sizeof(d);
    d.dwMaxPlayers = 2;
    d.lpszSessionNameA = (char *)"Adapter test";
    d.guidApplication = {0x234235, 0x1111, 0x2222, {1, 2, 3, 4, 5, 6, 7, 8}};
    CHECK(dp->Open(&d, DPOPEN_CREATE) == DP_OK);
    CHECK(d.guidInstance != GUID{});
    DPNAME name{};
    name.dwSize = sizeof(name);
    name.lpszShortNameA = (char *)"Test player";
    DPID player = 0;
    HANDLE event = CreateEventW(0, TRUE, FALSE, 0);
    const char value[] = "Player data";
    CHECK(dp->CreatePlayer(&player, &name, event, (void *)value, sizeof(value),
                           DPPLAYER_SERVERPLAYER) == DP_OK &&
          player == 1);
    CHECK(WaitForSingleObject(event, 0) == WAIT_TIMEOUT);
    int players = 0;
    CHECK(dp->EnumPlayers(0, Player, &players, 0) == DP_OK && players == 1);
    DWORD bytes = 0;
    CHECK(dp->GetSessionDesc(nullptr, &bytes) == DPERR_BUFFERTOOSMALL);
    std::vector<uint8_t> buf(bytes);
    CHECK(dp->GetSessionDesc(buf.data(), &bytes) == DP_OK);
    auto *desc = (DPSESSIONDESC2 *)buf.data();
    CHECK(desc->dwCurrentPlayers == 1 && !strcmp(desc->lpszSessionNameA, "Adapter test"));
    bytes = 0;
    CHECK(dp->GetPlayerName(player, nullptr, &bytes) == DPERR_BUFFERTOOSMALL);
    buf.resize(bytes);
    CHECK(dp->GetPlayerName(player, buf.data(), &bytes) == DP_OK);
    CHECK(!strcmp(((DPNAME *)buf.data())->lpszShortNameA, "Test player"));
    DPID from = 0, to = 0;
    bytes = 0;
    CHECK(dp->Receive(&from, &to, 0, nullptr, &bytes) == DPERR_NOMESSAGES);
    CHECK(WaitForSingleObject(event, 0) == WAIT_TIMEOUT);
    const char command[] = "game command";
    CHECK(dp->Send(player, player, DPSEND_GUARANTEED, (void *)command, sizeof(command)) == DP_OK);
    bytes = sizeof(command);
    buf.resize(bytes);
    CHECK(dp->Receive(&from, &to, 0, buf.data(), &bytes) == DP_OK && from == player &&
          to == player && !memcmp(buf.data(), command, sizeof(command)));
    CHECK(dp->SetPlayerData(player, (void *)value, sizeof(value), DPSET_LOCAL) == DP_OK);
    bytes = sizeof(value);
    buf.resize(bytes);
    CHECK(dp->GetPlayerData(player, buf.data(), &bytes, DPGET_LOCAL) == DP_OK &&
          !memcmp(buf.data(), value, sizeof(value)));
    CHECK(dp->Send(999, player, 0, (void *)command, sizeof(command)) == DPERR_INVALIDPLAYER);
    CHECK(dp->Send(player, player, 0, (void *)command, dt::MaxData + 1) == DPERR_SENDTOOBIG);
    CHECK(dp->DestroyPlayer(player) == DP_OK);
    CHECK(dp->Close() == DP_OK);
    CHECK(dp->Release() == 0);
    CloseHandle(event);
    // Exercise Valve's reliable message transport locally as well as the adapter ABI.
    HSteamNetConnection a = 0, b = 0;
    CHECK(SteamNetworkingSockets()->CreateSocketPair(&a, &b, false, nullptr, nullptr));
    CHECK(SteamNetworkingSockets()->SendMessageToConnection(a, command, sizeof(command),
                                                            k_nSteamNetworkingSend_Reliable,
                                                            nullptr) == k_EResultOK);
    SteamNetworkingMessage_t *message = nullptr;
    DWORD start = GetTickCount();
    while (!message && GetTickCount() - start < 5000) {
        SteamAPI_RunCallbacks();
        SteamNetworkingSockets()->ReceiveMessagesOnConnection(b, &message, 1);
        Sleep(5);
    }
    CHECK(message && message->m_cbSize == sizeof(command) &&
          !memcmp(message->m_pData, command, sizeof(command)));
    message->Release();
    SteamNetworkingSockets()->CloseConnection(a, 0, nullptr, false);
    SteamNetworkingSockets()->CloseConnection(b, 0, nullptr, false);
    SteamAPI_Shutdown();
    puts("PASS: Steam lobby, DirectPlay4A ABI, player/session buffers, system messages, "
         "peek/event, loopback and Valve reliable socket pair");
    return 0;
}
