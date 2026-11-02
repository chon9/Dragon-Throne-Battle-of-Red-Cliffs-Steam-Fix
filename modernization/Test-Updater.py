"""Test the shipped updater in isolated fixtures; never change the live game/registry."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parent
p = argparse.ArgumentParser()
p.add_argument('--package', type=Path, help='Unpacked test candidate, before building final ZIP')
p.add_argument('--previous', type=Path, action='append', default=[])
args = p.parse_args()
archive = ROOT / 'dist/DragonFullFix-0.4.0-1080p-public-beta.zip'
fixture = Path(tempfile.mkdtemp(prefix='updater-test-', dir=ROOT / 'builds'))
pack = fixture / 'DragonFullFix'
checks = []
if args.package:
    shutil.copytree(args.package, pack)
else:
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert not any(n.endswith(('.exe', '.log', '.dmp', '.otf', '.ttc', '.lib', 'SysInfo.cfg', 'Default.sty')) for n in z.namelist())
        assert not any('/vendor/steam/' in n for n in z.namelist())
        z.extractall(fixture)
    checks.append('ZIP integrity; no full executables, personal files or proprietary SDK/assets')
manifest = json.loads((pack / 'manifest.json').read_text())
state = fixture / 'modernization/installed-full-fix.json'
journal = fixture / 'modernization/full-fix-transaction.json'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None


def ps(script='Install.ps1', options=(), success=True, error=None, package=pack, stdin=None):
    r = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                        '-File', str(package / script), '-GameDirectory', str(fixture), *options],
                       input=stdin, capture_output=True, text=True)
    assert (r.returncode == 0) == success, r.stdout + r.stderr
    if error:
        assert error in r.stdout + r.stderr, r.stdout + r.stderr
    return r


def engine(action, **kwargs):
    return ps(options=('-Action', action), **kwargs)


def setup(*options, **kwargs):
    return ps('Setup.ps1', ('-NonInteractive', *options), **kwargs)


def passed(message):
    checks.append(message)
    print('PASS:', message, flush=True)


(fixture / 'DATA').mkdir()
shutil.copy2(ROOT / 'baseline/dragonthrone.exe', fixture / 'dragonthrone.exe')
for name in ('16Point.fnt', '20Point.fnt', '24Point.fnt'):
    shutil.copy2(ROOT / 'baseline/fonts' / name, fixture / 'DATA' / name)
for name, data in {'ddraw.ini': b'; custom renderer settings\n', 'ddraw.dll': b'previous renderer',
                   'SysInfo.cfg': b'personal settings', 'Save/keep.sav': b'personal save',
                   'DATA/Fod.avi': b'opening video 1', 'DATA/gpmom.avi': b'opening video 2',
                   'DATA/campaign.avi': b'campaign video'}.items():
    dest = fixture / name; dest.parent.mkdir(exist_ok=True); dest.write_bytes(data)
original = {n: sha(fixture / n) for n in [e['name'] for e in manifest['files']] +
            ['SysInfo.cfg', 'Save/keep.sav', 'DATA/Fod.avi', 'DATA/gpmom.avi', 'DATA/campaign.avi']}


def assert_original():
    for n, h in original.items():
        assert sha(fixture / n) == h, n
    assert not state.exists() and not journal.exists()
    assert not list((fixture / 'DATA').glob('*.disabled-by-dragonfix'))


def assert_installed(resolution='Desktop', skip='Off'):
    s = json.loads(state.read_text(encoding='utf-8-sig'))
    assert s['version'] == manifest['version']
    assert s['options'] == {'resolution': resolution, 'skipVideos': skip}
    for e in s['files']:
        assert sha(fixture / e['name']) == e['after'], e['name']
    for e in manifest['files']:
        if e['name'] != 'ddraw.ini' or resolution == 'Desktop':
            assert sha(fixture / e['name']) == e['after'], e['name']
    ini = (fixture / 'ddraw.ini').read_text()
    assert 'windowed=true' in ini and 'border=false' in ini and 'adjmouse=true' in ini
    if resolution == 'Desktop':
        assert 'fullscreen=true' in ini and 'width=0' in ini
    else:
        w, h = resolution.split('x')
        assert f'width={w}' in ini and f'height={h}' in ini and 'fullscreen=false' in ini
    for n in ('DATA/Fod.avi', 'DATA/gpmom.avi'):
        assert sha(fixture / (n + ('.disabled-by-dragonfix' if skip == 'On' else ''))) == original[n]
        assert not (fixture / (n if skip == 'On' else n + '.disabled-by-dragonfix')).exists()
    for n in ('Save/keep.sav', 'SysInfo.cfg', 'DATA/campaign.avi'):
        assert sha(fixture / n) == original[n]
    assert not journal.exists()


exe = fixture / 'dragonthrone.exe'; data = exe.read_bytes()
exe.write_bytes(b'bad')
setup(success=False, error='Unsupported or modified game file')
exe.write_bytes(data)
assert_original()
passed('Unknown executable refused before mutation')
payload = pack / manifest['files'][-1]['payload']; good = payload.read_bytes()
payload.write_bytes(good + b'bad')
setup(success=False, error='Package verification failed')
payload.write_bytes(good)
assert_original()
passed('Damaged fresh-install payload refused before mutation')
setup(); assert_installed()
passed('Guided defaults install exactly the manifest; videos default Off; saves preserved')
engine('install', success=False, error='already installed')
setup('-Resolution', '2560x1440', '-SkipVideos', 'On'); assert_installed('2560x1440', 'On')
setup('-Resolution', '3840x2160', '-SkipVideos', 'On'); assert_installed('3840x2160', 'On')
setup('-Resolution', '1920x1080'); assert_installed('1920x1080')
passed('1080p/1440p/4K output settings and repeated skip On/Off changes are reversible')
engine('restore'); assert_original()
setup('-SkipVideos', 'On'); assert_installed(skip='On')
# Steam verification may recreate an original filename beside the disabled copy.
video = fixture / 'DATA/Fod.avi'
video.write_bytes(b'new Steam download')
setup(success=False, error='Changed opening video')
engine('restore', success=False, error='Changed opening video')
assert video.read_bytes() == b'new Steam download'
video.unlink()
engine('restore'); assert_original()
passed('Restore removes new DLLs and re-enables opening videos byte-for-byte; duplicate video names are protected')

# Real old packs use their own shipped installer; the new wizard must reverse them automatically.
for index, previous in enumerate(args.previous):
    old_root = fixture / f'old-{index}'
    with zipfile.ZipFile(previous) as z:
        z.extractall(old_root)
    old_pack = old_root / 'DragonFullFix'
    engine('install', package=old_pack)
    old_files = {e['name']: sha(fixture / e['name']) for e in manifest['files']}
    old_state = state.read_bytes()
    payload.write_bytes(good + b'bad')
    setup(success=False, error='Package verification failed')
    payload.write_bytes(good)
    assert state.read_bytes() == old_state
    assert all(sha(fixture / n) == h for n, h in old_files.items())
    # Broken original backups and modifications by other mods must block an update.
    s = json.loads(old_state.decode('utf-8-sig'))
    bad_record = dict(s, backup='../outside-fixture')
    state.write_text(json.dumps(bad_record))
    setup(success=False, error='Unsafe backup location')
    state.write_bytes(old_state)
    original_exe = fixture / 'modernization' / s['backup'].replace('\\', '/') / 'dragonthrone.exe'
    saved = original_exe.read_bytes(); original_exe.write_bytes(b'broken backup')
    setup(success=False, error='Backup verification failed')
    original_exe.write_bytes(saved)
    dll = fixture / 'DTNet.dll'; saved = dll.read_bytes(); dll.write_bytes(saved + b'other mod')
    setup(success=False, error='Changed file')
    engine('restore', success=False, error='Changed file')
    dll.write_bytes(saved)
    assert state.read_bytes() == old_state
    setup('-SkipVideos', 'On'); assert_installed(skip='On')
    engine('restore'); assert_original()
    passed(f'Automatic upgrade and exact original restore: {previous.name}; damaged payload/backup and changed runtime protected')

# Fault injection changes only the isolated copy of the script, never production code.
setup()
before_files = {e['name']: sha(fixture / e['name']) for e in manifest['files']}
before_state = state.read_bytes()
installer = pack / 'Install.ps1'; source = installer.read_text(encoding='utf-8-sig')
needle = '    Set-File $newState $statePath'
assert source.count(needle) == 1
installer.write_text(source.replace(needle, "    throw 'Injected late write failure'\n" + needle), encoding='utf-8')
setup('-SkipVideos', 'On', success=False, error='Injected late write failure')
installer.write_text(source, encoding='utf-8')
assert state.read_bytes() == before_state
assert all(sha(fixture / n) == h for n, h in before_files.items())
assert_installed()
passed('Late installation failure restores the previous patched files, record and video filenames')
installer.write_text(source.replace(needle, '    Stop-Process -Id $PID -Force\n' + needle), encoding='utf-8')
setup('-SkipVideos', 'On', success=False)
installer.write_text(source, encoding='utf-8')
assert journal.exists()
setup(success=False, error='Run Recover.cmd')
engine('recover')
assert state.read_bytes() == before_state
assert all(sha(fixture / n) == h for n, h in before_files.items())
assert_installed()
engine('restore'); assert_original()
passed('Killed updater leaves a recovery journal; Recover restores the previous installation and videos')

# Exercise prompts and language routing with a fixture-only registry stub. Live HKLM is never changed.
language_script = pack / 'Language.ps1'; real_language = language_script.read_bytes()
language_script.write_text('param($Language,$GameDirectory,[switch]$CheckOnly,[switch]$NoPause)\n'
                           '"$Language|$CheckOnly|$NoPause" | Add-Content -LiteralPath (Join-Path $GameDirectory "language-test.txt")\n', encoding='utf-8')
for number, language in [('2', 'English'), ('3', 'TraditionalChinese')]:
    ps('Setup.ps1', stdin=f'invalid\n1\n{number}\n1\n')
    assert_installed()
    lines = (fixture / 'language-test.txt').read_text().splitlines()
    assert lines[-2:] == [f'{language}|True|False', f'{language}|False|True'], lines
    engine('restore'); assert_original()
language_script.write_bytes(real_language)
passed('Interactive invalid-choice retry, English/Traditional Chinese preflight and switch routing (registry stub only)')
ps('Setup.ps1', stdin='\n\n\n'); assert_installed()
engine('restore'); assert_original()
passed('Pressing Enter through all prompts uses desktop output, current language and videos Off')
report = {'version': manifest['version'], 'archive_sha256': sha(archive) if not args.package else None,
          'installer': 'Windows PowerShell 5.1', 'checks': checks,
          'pending': ['Visual validation of scaled 1440p/4K output', 'Live elevated language switch; routing tested with registry stub',
                      'Public Noto fonts and long English UI visual coverage', 'Measured command latency', 'Native 4K HUD'],
          'notes': 'No live installation, registry, save or Steam settings changed by these fixture tests.'}
(ROOT / 'dist/updater-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('Fixture retained:', fixture)
