"""Generate grayscale glyph coverage and a freestanding x86 text hook."""
import hashlib
import argparse
import pathlib
import struct
import subprocess
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
import pefile
import os
import json

ROOT = pathlib.Path(__file__).resolve().parent
OUTPUT = ROOT / 'builds/smooth-fonts'
MSVC = pathlib.Path(os.environ.get('DT_MSVC_BIN', 'C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x86'))


def build_atlas():
    OUTPUT.mkdir(exist_ok=True)
    face = ROOT / 'vendor/fonts/NotoSansCJKtc-Regular.otf'
    coverage = TTFont(str(face), fontNumber=0).getBestCmap()
    data = bytearray(3 * 65536 * 4)
    for index, size in enumerate([18, 19, 22]):
        h = 24 if index == 2 else 20
        font = ImageFont.truetype(str(face), size, index=0)
        for code in sorted(coverage):
            if not 32 <= code < 65536 or 0xd800 <= code < 0xe000:
                continue
            glyph = Image.new('L', (24, h), 0)
            box = font.getbbox(chr(code), anchor='ls')
            ImageDraw.Draw(glyph).text((max(0, -box[0]), h - 4), chr(code), font=font, anchor='ls', fill=255)
            struct.pack_into('<I', data, (index * 65536 + code) * 4, len(data))
            data.extend(glyph.tobytes())
    (OUTPUT / 'coverage.bin').write_bytes(data)
    (OUTPUT / 'manifest.json').write_text(json.dumps({'source_sha256': hashlib.sha256(face.read_bytes()).hexdigest(),
                                                   'sha256': hashlib.sha256(data).hexdigest()}, indent=2))
    print('Coverage bytes:', len(data), 'SHA256:', hashlib.sha256(data).hexdigest())
    return bytes(data)


def compile_hook(code_va, atlas_va, original_va):
    OUTPUT.mkdir(exist_ok=True)
    obj = OUTPUT / 'SmoothFont.obj'
    exe = OUTPUT / 'SmoothFont.exe'
    subprocess.run([str(MSVC / 'cl.exe'), '/nologo', '/c', '/O2', '/GS-', '/GR-', '/Zl',
                    '/D' + 'FONT_ATLAS=' + hex(atlas_va), '/D' + 'FONT_ORIGINAL=' + hex(original_va),
                    '/Fo' + str(obj), str(ROOT / 'SmoothFont.cpp')], check=True, capture_output=True)
    # The linker places .text at RVA 0x1000; the payload is copied to code_va.
    subprocess.run([str(MSVC / 'link.exe'), '/nologo', '/nodefaultlib', '/fixed:no', '/dynamicbase:no',
                    '/machine:x86', '/safeseh:no', '/subsystem:windows', '/entry:FontEntry',
                    '/base:0x400000', '/merge:.rdata=.text', '/out:' + str(exe), str(obj)],
                   check=True, capture_output=True)
    pe = pefile.PE(str(exe))
    assert not getattr(pe, 'DIRECTORY_ENTRY_IMPORT', None)
    pe.relocate_image(code_va - 0x1000)
    # pefile returns early for fully position-independent code (no relocations).
    pe.OPTIONAL_HEADER.ImageBase = code_va - 0x1000
    text = next(s for s in pe.sections if s.Name.rstrip(b'\0') == b'.text')
    assert text.VirtualAddress + pe.OPTIONAL_HEADER.ImageBase == code_va
    assert all(s.Name.rstrip(b'\0') in (b'.text', b'.reloc') for s in pe.sections)
    entry = pe.OPTIONAL_HEADER.ImageBase + pe.OPTIONAL_HEADER.AddressOfEntryPoint
    assert code_va <= entry < code_va + text.Misc_VirtualSize
    return text.get_data()[:text.Misc_VirtualSize], entry


def test_hook():
    OUTPUT.mkdir(exist_ok=True)
    obj, exe = OUTPUT / 'Test.obj', OUTPUT / 'Test.exe'
    subprocess.run([str(MSVC / 'cl.exe'), '/nologo', '/c', '/O2', '/GS-', '/GR-', '/Zl',
                    '/Fo' + str(obj), str(ROOT / 'Test-SmoothFont.cpp')], check=True)
    subprocess.run([str(MSVC / 'link.exe'), '/nologo', '/nodefaultlib', '/machine:x86',
                    '/subsystem:console', '/entry:main', '/out:' + str(exe), str(obj)], check=True)
    subprocess.run([str(exe)], check=True)
    print('PASS: RGB565 blending, clipping, transparent pixels, balanced locks, fallback paths.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--test', action='store_true')
    args = parser.parse_args()
    if args.test:
        test_hook()
    else:
        build_atlas()
