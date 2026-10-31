// Generated from vendor/directplay/dplay.h; retain its license and provenance.
HRESULT WINAPI AddPlayerToGroup(DPID, DPID) override { return Unsupported("AddPlayerToGroup"); }
HRESULT WINAPI Close() override ;
HRESULT WINAPI CreateGroup(LPDPID, LPDPNAME, LPVOID, DWORD, DWORD) override { return Unsupported("CreateGroup"); }
HRESULT WINAPI CreatePlayer(LPDPID lpidPlayer, LPDPNAME lpPlayerName, HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) override ;
HRESULT WINAPI DeletePlayerFromGroup(DPID, DPID) override { return Unsupported("DeletePlayerFromGroup"); }
HRESULT WINAPI DestroyGroup(DPID) override { return Unsupported("DestroyGroup"); }
HRESULT WINAPI DestroyPlayer(DPID idPlayer) override ;
HRESULT WINAPI EnumGroupPlayers(DPID, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override { return Unsupported("EnumGroupPlayers"); }
HRESULT WINAPI EnumGroups(LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override { return Unsupported("EnumGroups"); }
HRESULT WINAPI EnumPlayers(LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) override ;
HRESULT WINAPI EnumSessions(LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2, LPVOID lpContext, DWORD dwFlags) override ;
HRESULT WINAPI GetCaps(LPDPCAPS lpDPCaps, DWORD dwFlags) override ;
HRESULT WINAPI GetGroupData(DPID, LPVOID, LPDWORD, DWORD) override { return Unsupported("GetGroupData"); }
HRESULT WINAPI GetGroupName(DPID, LPVOID, LPDWORD) override { return Unsupported("GetGroupName"); }
HRESULT WINAPI GetMessageCount(DPID idPlayer, LPDWORD lpdwCount) override ;
HRESULT WINAPI GetPlayerAddress(DPID, LPVOID, LPDWORD) override { return Unsupported("GetPlayerAddress"); }
HRESULT WINAPI GetPlayerCaps(DPID idPlayer, LPDPCAPS lpPlayerCaps, DWORD dwFlags) override ;
HRESULT WINAPI GetPlayerData(DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags) override ;
HRESULT WINAPI GetPlayerName(DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize) override ;
HRESULT WINAPI GetSessionDesc(LPVOID lpData, LPDWORD lpdwDataSize) override ;
HRESULT WINAPI Initialize(LPGUID lpGUID) override ;
HRESULT WINAPI Open(LPDPSESSIONDESC2 lpsd, DWORD dwFlags) override ;
HRESULT WINAPI Receive(LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize) override ;
HRESULT WINAPI Send(DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize) override ;
HRESULT WINAPI SetGroupData(DPID, LPVOID, DWORD, DWORD) override { return Unsupported("SetGroupData"); }
HRESULT WINAPI SetGroupName(DPID, LPDPNAME, DWORD) override { return Unsupported("SetGroupName"); }
HRESULT WINAPI SetPlayerData(DPID idPlayer, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) override ;
HRESULT WINAPI SetPlayerName(DPID idPlayer, LPDPNAME lpPlayerName, DWORD dwFlags) override ;
HRESULT WINAPI SetSessionDesc(LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags) override ;
HRESULT WINAPI AddGroupToGroup(DPID, DPID) override { return Unsupported("AddGroupToGroup"); }
HRESULT WINAPI CreateGroupInGroup(DPID, LPDPID, LPDPNAME, LPVOID, DWORD, DWORD) override { return Unsupported("CreateGroupInGroup"); }
HRESULT WINAPI DeleteGroupFromGroup(DPID, DPID) override { return Unsupported("DeleteGroupFromGroup"); }
HRESULT WINAPI EnumConnections(LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags) override ;
HRESULT WINAPI EnumGroupsInGroup(DPID, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override { return Unsupported("EnumGroupsInGroup"); }
HRESULT WINAPI GetGroupConnectionSettings(DWORD, DPID, LPVOID, LPDWORD) override { return Unsupported("GetGroupConnectionSettings"); }
HRESULT WINAPI InitializeConnection(LPVOID lpConnection, DWORD dwFlags) override ;
HRESULT WINAPI SecureOpen(LPCDPSESSIONDESC2, DWORD, LPCDPSECURITYDESC, LPCDPCREDENTIALS) override { return Unsupported("SecureOpen"); }
HRESULT WINAPI SendChatMessage(DPID, DPID, DWORD, LPDPCHAT) override { return Unsupported("SendChatMessage"); }
HRESULT WINAPI SetGroupConnectionSettings(DWORD, DPID, LPDPLCONNECTION) override { return Unsupported("SetGroupConnectionSettings"); }
HRESULT WINAPI StartSession(DWORD, DPID) override { return Unsupported("StartSession"); }
HRESULT WINAPI GetGroupFlags(DPID, LPDWORD) override { return Unsupported("GetGroupFlags"); }
HRESULT WINAPI GetGroupParent(DPID, LPDPID) override { return Unsupported("GetGroupParent"); }
HRESULT WINAPI GetPlayerAccount(DPID, DWORD, LPVOID, LPDWORD) override { return Unsupported("GetPlayerAccount"); }
HRESULT WINAPI GetPlayerFlags(DPID idPlayer, LPDWORD lpdwFlags) override ;
HRESULT WINAPI GetGroupOwner(DPID, LPDPID) override { return Unsupported("GetGroupOwner"); }
HRESULT WINAPI SetGroupOwner(DPID, DPID) override { return Unsupported("SetGroupOwner"); }
HRESULT WINAPI SendEx(DPID , DPID , DWORD , LPVOID , DWORD , DWORD , DWORD , LPVOID , LPDWORD  ) override ;
HRESULT WINAPI GetMessageQueue(DPID , DPID , DWORD , LPDWORD , LPDWORD  ) override ;
HRESULT WINAPI CancelMessage(DWORD , DWORD  ) override ;
HRESULT WINAPI CancelPriority(DWORD , DWORD , DWORD  ) override ;
