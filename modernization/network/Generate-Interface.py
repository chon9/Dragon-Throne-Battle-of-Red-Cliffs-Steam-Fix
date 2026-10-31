"""Generate ABI-exact declarations from the pinned DirectPlay header."""
import pathlib, re
root=pathlib.Path(__file__).resolve().parent
s=(root/'vendor/directplay/dplay.h').read_text()
s=s[s.index('DECLARE_INTERFACE_(IDirectPlay4,'):]
s=s[:s.index('};')]
implemented=set('Close CreatePlayer DestroyPlayer EnumPlayers EnumSessions GetCaps GetMessageCount GetPlayerCaps GetPlayerData GetPlayerName GetSessionDesc Initialize Open Receive Send SetPlayerData SetPlayerName SetSessionDesc EnumConnections InitializeConnection GetPlayerFlags SendEx GetMessageQueue CancelMessage CancelPriority'.split())
out=['// Generated from vendor/directplay/dplay.h; retain its license and provenance.']
for name,args in re.findall(r'STDMETHOD\((\w+)\)\((.*?)\) PURE;',s):
    args=args.replace('THIS_ ','').replace('THIS','')
    suffix=';' if name in implemented else '{ return Unsupported("'+name+'"); }'
    # Remove unused parameter names from unsupported stubs, avoiding warnings.
    if name not in implemented:
        args=', '.join(re.sub(r'\s+\w+$','',x.strip()) for x in args.split(','))
    out.append('HRESULT WINAPI '+name+'('+args+') override '+suffix)
(root/'DirectPlayMethods.inc').write_text('\n'.join(out)+'\n')
