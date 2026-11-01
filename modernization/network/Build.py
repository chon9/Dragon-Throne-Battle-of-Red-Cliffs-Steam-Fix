import os
import pathlib
import subprocess
import shutil

ROOT = pathlib.Path(__file__).resolve().parent
VC = pathlib.Path(os.environ.get('DT_MSVC_ROOT','C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133'))
SDK = pathlib.Path(os.environ.get('DT_WINDOWS_SDK_ROOT','C:/Program Files (x86)/Windows Kits/10'))
VERSION = os.environ.get('DT_WINDOWS_SDK_VERSION','10.0.16299.0')
OUT = ROOT / 'build'
OUT.mkdir(exist_ok=True)
env = dict(os.environ)
env['INCLUDE'] = ';'.join(map(str, [VC / 'include'] + [SDK / 'Include' / VERSION / n for n in ['ucrt','shared','um']]))
env['LIB'] = ';'.join(map(str, [VC / 'lib/x86'] + [SDK / 'Lib' / VERSION / n / 'x86' for n in ['ucrt','um']]))
STEAM = ROOT/'vendor/redistributable_bin'
steam_dll = STEAM/'steam_api.dll'
if not steam_dll.exists(): steam_dll=ROOT.parent/'steam_api.dll'
shutil.copy2(steam_dll,OUT/'steam_api.dll')
subprocess.run(['python',str(ROOT/'Generate-Interface.py')],check=True)
for name in ['SteamProbe','Test-Protocol','Test-Latency','Test-ModernUI','DTNet','Test-Adapter','Test-ProviderCore']:
    if not (ROOT/(name+'.cpp')).exists(): continue
    dll=name=='DTNet'
    command=[str(VC/'bin/Hostx64/x86/cl.exe'),'/nologo','/utf-8','/D_CRT_SECURE_NO_WARNINGS','/std:c++17','/O2','/MT','/EHsc','/W4','/wd4100',
             '/Fo'+str(OUT/(name+'.obj')), '/Fe'+str(OUT/(name+('.dll' if dll else '.exe'))), str(ROOT/(name+'.cpp'))]
    if dll: command+=['/LD']
    command+=['/link','/LIBPATH:'+str(VC/'lib/x86'),'kernel32.lib','user32.lib','gdi32.lib','ole32.lib','uuid.lib',str(STEAM/'steam_api.lib')]
    if dll: command+=['/DEF:'+str(ROOT/'DTNet.def')]
    subprocess.run(command,env=env,cwd=OUT,check=True)
subprocess.run([str(OUT/'Test-Protocol.exe')],check=True,cwd=OUT)
subprocess.run([str(OUT/'Test-Latency.exe')],check=True,cwd=OUT)
subprocess.run([str(OUT/'Test-ModernUI.exe')],check=True,cwd=OUT)
subprocess.run([str(OUT/'Test-ProviderCore.exe')],check=True,cwd=OUT)
subprocess.run([str(VC/'bin/Hostx64/x86/cl.exe'),'/nologo','/std:c++17','/O2','/MT','/EHsc',
                '/Fo'+str(OUT/'Test-WorldOrigin.obj'),'/Fe'+str(OUT/'Test-WorldOrigin.exe'),
                str(ROOT.parent/'Test-WorldOrigin.cpp'),'/link','kernel32.lib'],env=env,cwd=OUT,check=True)
subprocess.run([str(OUT/'Test-WorldOrigin.exe')],check=True,cwd=OUT)
