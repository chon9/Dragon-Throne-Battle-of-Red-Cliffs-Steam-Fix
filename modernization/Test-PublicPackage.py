"""Exercise the shipped Windows PowerShell installer on an isolated game fixture."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile
import argparse

ROOT = Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('--previous',type=Path)
args=parser.parse_args()
ARCHIVE = ROOT / 'dist/DragonFullFix-0.3.3-1080p-public-beta.zip'
FIXTURE = Path(tempfile.mkdtemp(prefix='full-fix-test-', dir=ROOT / 'builds'))
with zipfile.ZipFile(ARCHIVE) as z:
    assert z.testzip() is None
    assert not any(p.endswith(('.exe', '.log', '.dmp', 'SysInfo.cfg', 'Default.sty')) for p in z.namelist())
    assert not any('/vendor/steam/' in p or p.endswith(('.lib','.otf','.ttc')) for p in z.namelist())
    z.extractall(FIXTURE)
PACK = FIXTURE / 'DragonFullFix'
MANIFEST = json.loads((PACK / 'manifest.json').read_text())


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def run(action, success=True, error=None, package=PACK):
    result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                             '-File', str(package / 'Install.ps1'), '-Action', action,
                             '-GameDirectory', str(FIXTURE)], capture_output=True, text=True)
    assert (result.returncode == 0) == success, result.stdout + result.stderr
    if error:
        assert error in result.stderr, result.stdout + result.stderr
    print(action, 'PASS' if success else 'correctly refused', flush=True)


for name in ('dragonthrone.exe',):
    shutil.copy2(ROOT / 'baseline' / name, FIXTURE / name)
(FIXTURE / 'DATA').mkdir()
for name in ('16Point.fnt', '20Point.fnt', '24Point.fnt'):
    shutil.copy2(ROOT / 'baseline/fonts' / name, FIXTURE / 'DATA' / name)
(FIXTURE / 'ddraw.ini').write_text('; friend custom configuration\n')
(FIXTURE / 'ddraw.dll').write_bytes(b'original renderer sentinel')
(FIXTURE / 'Save').mkdir()
(FIXTURE / 'Save/do-not-touch.sav').write_bytes(b'personal save sentinel')
(FIXTURE / 'SysInfo.cfg').write_bytes(b'personal settings sentinel')
original = {p.relative_to(FIXTURE).as_posix(): digest(p) for p in FIXTURE.rglob('*')
            if p.is_file() and PACK not in p.parents}
# Unknown executable must stop before even creating installation state.
exe = FIXTURE / 'dragonthrone.exe'
good = exe.read_bytes()
exe.write_bytes(b'X' + good[1:])
run('install', False, 'Unsupported or modified game file')
assert not (FIXTURE / 'modernization').exists()
exe.write_bytes(good)
# A damaged shipped payload must be refused before mutation.
payload = PACK / MANIFEST['files'][-1]['payload']
data = payload.read_bytes()
payload.write_bytes(data + b'bad')
run('install', False, 'Package verification failed')
assert not (FIXTURE / 'modernization').exists()
payload.write_bytes(data)
run('install')
for entry in MANIFEST['files']:
    assert digest(FIXTURE / entry['name']) == entry['after'], entry['name']
run('install', False, 'already installed')
# Another mod's changed DLL must not be overwritten by Restore.
dll = FIXTURE / 'DTNet.dll'
data = dll.read_bytes()
dll.write_bytes(data + b'changed')
run('restore', False, 'Changed file')
assert digest(exe) == MANIFEST['files'][0]['after']
dll.write_bytes(data)
run('restore')
for name, expected in original.items():
    assert digest(FIXTURE / name) == expected, name
for name in ('DTNet.dll', 'steam_api.dll'):
    assert not (FIXTURE / name).exists(), name
assert not (FIXTURE / 'modernization/installed-full-fix.json').exists()
# Exercise the documented previous release -> restore -> current release path.
# The new Restore.cmd must
# also work after a friend extracts over their old package folder.
if args.previous:
    old = FIXTURE / 'old-package'
    with zipfile.ZipFile(args.previous) as z:z.extractall(old)
    run('install', package=old / 'DragonFullFix')
    run('install', False, 'already installed')
    run('restore')
    run('install')
    for entry in MANIFEST['files']:
        assert digest(FIXTURE / entry['name']) == entry['after']
    run('restore')
    for name, expected in original.items():
        assert digest(FIXTURE / name) == expected, name
print('Exact installed file match, exact original restore, saved data preservation: PASS')
print('Fixture retained:', FIXTURE)
(ROOT / 'dist/verification.json').write_text(json.dumps({
    'version': MANIFEST['version'], 'archive_sha256': digest(ARCHIVE), 'installer': 'Windows PowerShell 5.1',
    'checks': ['ZIP integrity', 'no full game executable or personal files',
               'unknown executable rejected before mutation', 'damaged payload rejected before mutation',
               'all installed files match public build manifest', 'repeat install rejected',
               'changed runtime protected on restore', 'byte-exact restore',
               'save and settings sentinels preserved', 'new DLLs removed on restore',
               'previous pack upgrade and byte-exact restore' if args.previous else 'previous pack upgrade not run'],
    'language': 'English assets and current Traditional Chinese registry value checked read-only; live switch not run',
    'user_confirmed': ['two-PC multiplayer on previous adapter', 'revision 3 bottom-right panel',
                       'building duplication gone and unit selection correct on local origin fix',
                       'continued gameplay and working buttons on local permanent freeze fix',
                       'appointment works, buttons correct and no freeze on local permanent appointment fix'],
    'pending': ['public Noto fonts in-game visual check', 'friend English UI visual check', 'measured command latency', 'native 4K'],
}, indent=2))
