#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

// Uses only the game's real app ID and the signed Valve client API. Creates
// one private, empty lobby to test availability, then leaves it immediately.
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    SetEnvironmentVariableW(L"SteamAppId", L"2633500");
    SetEnvironmentVariableW(L"SteamGameId", L"2633500");
    HMODULE dll = LoadLibraryExW(argv[1], 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!dll) { printf("{\"load_error\":%lu}\n", GetLastError()); return 3; }
#define PROC(type,name,symbol) type name = (type)GetProcAddress(dll,symbol); if (!name) return 4
    typedef bool (__cdecl *Init)();
    typedef void (__cdecl *Void)();
    typedef void* (__cdecl *Interface)();
    typedef bool (__cdecl *BoolInterface)(void*);
    typedef unsigned (__cdecl *GetId)(void*);
    typedef void (__cdecl *StartRelay)(void*);
    typedef int (__cdecl *RelayStatus)(void*,void*);
    typedef uint64_t (__cdecl *CreateLobby)(void*,int,int);
    typedef void (__cdecl *LeaveLobby)(void*,uint64_t);
    typedef bool (__cdecl *Completed)(void*,uint64_t,bool*);
    typedef bool (__cdecl *CallResult)(void*,uint64_t,void*,int,int,bool*);
    PROC(Init,init,"SteamAPI_Init"); PROC(Void,shutdown,"SteamAPI_Shutdown");
    PROC(Void,callbacks,"SteamAPI_RunCallbacks");
    PROC(Interface,utils,"SteamAPI_SteamUtils_v010");
    PROC(Interface,user,"SteamAPI_SteamUser_v021");
    PROC(Interface,match,"SteamAPI_SteamMatchmaking_v009");
    PROC(Interface,network,"SteamAPI_SteamNetworkingUtils_SteamAPI_v004");
    PROC(GetId,appId,"SteamAPI_ISteamUtils_GetAppID");
    PROC(BoolInterface,logged,"SteamAPI_ISteamUser_BLoggedOn");
    PROC(StartRelay,relayInit,"SteamAPI_ISteamNetworkingUtils_InitRelayNetworkAccess");
    PROC(RelayStatus,relayStatus,"SteamAPI_ISteamNetworkingUtils_GetRelayNetworkStatus");
    PROC(CreateLobby,create,"SteamAPI_ISteamMatchmaking_CreateLobby");
    PROC(LeaveLobby,leave,"SteamAPI_ISteamMatchmaking_LeaveLobby");
    PROC(Completed,complete,"SteamAPI_ISteamUtils_IsAPICallCompleted");
    PROC(CallResult,result,"SteamAPI_ISteamUtils_GetAPICallResult");
    if (!init()) { puts("{\"steam_init\":false}"); return 5; }
    unsigned app = appId(utils());
    if (app != 2633500) { shutdown(); printf("{\"unexpected_app\":%u}\n",app); return 6; }
    bool online = logged(user());
    relayInit(network());
    uint64_t request = create(match(),0,2);
    struct LobbyCreated { int result; uint64_t lobby; } lobby = {};
    static_assert(sizeof(LobbyCreated)==16, "Steam callback layout");
    bool failed = false, ready = false, obtained = false;
    DWORD start = GetTickCount();
    int relay = 0;
    while (GetTickCount() - start < 15000) {
        callbacks();
        if (!ready) ready = complete(utils(),request,&failed);
        if (ready && !obtained && !failed) {
            obtained = result(utils(),request,&lobby,sizeof(lobby),513,&failed);
            if (obtained && lobby.result == 1) leave(match(),lobby.lobby);
        }
        relay = relayStatus(network(),0);
        if (ready && (relay == 100 || relay < 0)) break;
        Sleep(50);
    }
    printf("{\"steam_init\":true,\"app_id\":%u,\"logged_on\":%s,\"private_lobby_result\":%d,\"callback_received\":%s,\"relay_availability\":%d}\n",app,online?"true":"false",lobby.result,obtained?"true":"false",relay);
    shutdown();
    return obtained && lobby.result == 1 ? 0 : 7;
}
