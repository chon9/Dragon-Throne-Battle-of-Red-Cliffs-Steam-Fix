"""Build the public patch-only player archive from locally supplied dependencies."""
from pathlib import Path
import gzip
import hashlib
import importlib.util
import json
import shutil
import struct
import subprocess
import zipfile
import pefile

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
VERSION = '0.3.2'
FONT_SHA = 'dce08bd4fd91aa8aa76ed8fea4b694c2dfb8550f67871e326843212ddbeb88b4'
STEAM_SHA = '67ae11d71ae6ec404090094df1e47b614d27400dc53fa450023e6fbcf347902c'
DDRAW_SHA = '85e0f7d530dfda134793a57cb3e76b0287dcc96892ee57162dd68f47283b03a9'

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def module(name, path):
    spec=importlib.util.spec_from_file_location(name,path)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result)
    return result

def main():
    assert sha(ROOT/'vendor/fonts/NotoSansCJKtc-Regular.otf')==FONT_SHA
    fonts=json.loads((ROOT/'builds/readable-fonts/manifest.json').read_text())
    assert Path(fonts['font']).name=='NotoSansCJKtc-Regular.otf'
    for entry in fonts['files']:
        assert sha(ROOT/'builds/readable-fonts'/entry['name'])==entry['sha256']
    # Require a Noto-built coverage manifest, preventing reuse of private atlases.
    coverage=json.loads((ROOT/'builds/smooth-fonts/manifest.json').read_text())
    assert coverage['source_sha256']==FONT_SHA
    assert sha(ROOT/'builds/smooth-fonts/coverage.bin')==coverage['sha256']
    module('wide',ROOT/'Build-Widescreen.py').build(1920,1080,True,True)
    exe=bytearray((ROOT/'builds/native-1920x1080-smooth-modern/dragonthrone.exe').read_bytes())
    pe=pefile.PE(data=exe)
    entry=next(e for e in pe.DIRECTORY_ENTRY_IMPORT if e.dll==b'ole32.dll')
    assert {i.name for i in entry.imports}=={b'CoInitialize',b'CoUninitialize',b'CoCreateInstance'}
    offset=pe.get_offset_from_rva(entry.struct.Name)
    assert exe[offset:offset+10]==b'ole32.dll\0'
    exe[offset:offset+10]=b'DTNet.dll\0'
    checksum=pe.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum')
    exe[checksum:checksum+4]=bytes(4)
    exe[checksum:checksum+4]=pefile.PE(data=exe).generate_checksum().to_bytes(4,'little')
    staged=ROOT/'builds/public';staged.mkdir(exist_ok=True)
    (staged/'dragonthrone.exe').write_bytes(exe)
    # A fresh directory prevents stale SDK headers or private assets entering a rebuild.
    out=ROOT/'dist'/f'v{VERSION}'/'DragonFullFix'
    out.mkdir(parents=True,exist_ok=False)
    (out/'payload').mkdir()
    manifest={'version':VERSION+'-public-beta','resolution':'1920x1080',
              'font':'Dragon UI Bitmap (Noto Sans CJK TC derivative; OFL-1.1)',
              'status':'Building redraw and command-panel freeze fixes user confirmed on local build; public font visual coverage pending; latency diagnostics only','files':[]}

    def delta(name, original, updated):
        before,after=original.read_bytes(),updated.read_bytes()
        ranges=[];start=last=None
        for i in range(min(len(before),len(after))):
            if before[i]!=after[i]:
                if start is None:start=i
                last=i
            elif start is not None and i-last>32:
                ranges.append((start,last+1));start=None
        if start is not None:ranges.append((start,last+1))
        if len(after)>len(before):ranges.append((len(before),len(after)))
        data=bytearray(struct.pack('<4sII',b'DFP1',len(after),len(ranges)))
        check=bytearray(before[:len(after)]);check.extend(bytes(max(0,len(after)-len(check))))
        for a,b in ranges:
            data.extend(struct.pack('<II',a,b-a));data.extend(after[a:b]);check[a:b]=after[a:b]
        assert check==after
        path='payload/'+Path(name).name+'.patch.gz'
        (out/path).write_bytes(gzip.compress(data,compresslevel=9,mtime=0))
        manifest['files'].append({'name':name,'kind':'delta','before':sha(original),'after':sha(updated),
                                  'payload':path,'payloadSha256':sha(out/path)})

    delta('dragonthrone.exe',ROOT/'baseline/dragonthrone.exe',staged/'dragonthrone.exe')
    for name in ('16Point.fnt','20Point.fnt','24Point.fnt'):
        delta('DATA/'+name,ROOT/'baseline/fonts'/name,ROOT/'builds/readable-fonts'/name)
    assert sha(ROOT/'network/build/steam_api.dll')==STEAM_SHA
    assert sha(ROOT/'renderers/cnc-ddraw/ddraw.dll')==DDRAW_SHA
    for name,source in [('ddraw.dll',ROOT/'renderers/cnc-ddraw/ddraw.dll'),
                        ('ddraw.ini',ROOT/'profiles/cnc-borderless.ini'),
                        ('DTNet.dll',ROOT/'network/build/DTNet.dll'),('steam_api.dll',ROOT/'network/build/steam_api.dll')]:
        path='payload/'+name;shutil.copy2(source,out/path)
        manifest['files'].append({'name':name,'kind':'copy','payload':path,'payloadSha256':sha(source),'after':sha(source)})
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    for source,name in [('FullFix-Install.ps1','Install.ps1'),('FullFix-Language.ps1','Language.ps1'),('FullFix-README.txt','README.txt')]:
        shutil.copy2(ROOT/source,out/name)
    for action in ('Install','Restore'):
        (out/(action+'.cmd')).write_text('@echo off\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install.ps1" -Action '+action.lower()+' -GameDirectory "%~dp0.."\nif errorlevel 1 echo Operation failed. Read the error above.\npause\n')
    (out/'Language.cmd').write_text('@echo off\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Language.ps1" -GameDirectory "%~dp0.."\npause\n')
    (out/'Launch-Steam.cmd').write_text('@echo off\nstart "" "steam://rungameid/2633500"\n')
    for name in ('LICENSE','THIRD_PARTY_NOTICES.md'):
        shutil.copy2(REPO/name,out/name)
    shutil.copytree(REPO/'licenses',out/'licenses')
    # git's allowlist includes project code/docs and intentionally excludes local SDK/assets.
    paths=subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard'],cwd=REPO).decode().split('\0')
    for name in sorted(set(filter(None,paths))):
        assert '/vendor/steam/' not in name and '/baseline/' not in name
        assert Path(name).suffix.lower() not in ('.exe','.dll','.lib','.fnt','.otf','.log','.dmp')
        dest=out/'source'/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(REPO/name,dest)
    archive=ROOT/'dist'/f'DragonFullFix-{VERSION}-1080p-public-beta.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file():z.write(p,p.relative_to(out.parent))
    archive.with_suffix('.zip.sha256').write_text(sha(archive)+'  '+archive.name+'\n')
    print(json.dumps({'archive':str(archive),'bytes':archive.stat().st_size,'sha256':sha(archive)},indent=2))

if __name__=='__main__':main()
