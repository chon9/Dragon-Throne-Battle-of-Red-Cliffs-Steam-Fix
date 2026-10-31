"""Build OFL-licensed Noto-derived glyphs; preserve unsupported local game glyphs."""
import hashlib
import json
import pathlib
import struct
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont

ROOT = pathlib.Path(__file__).resolve().parent
FONT = ROOT / 'vendor/fonts/NotoSansCJKtc-Regular.otf'


def build():
    backup = ROOT / 'baseline/fonts'
    backup.mkdir(exist_ok=True)
    output = ROOT / 'builds/readable-fonts'
    output.mkdir(exist_ok=True)
    coverage = TTFont(str(FONT), fontNumber=0).getBestCmap()
    records = []
    preview = Image.new('RGB', (800, 210), '#eee7d5')
    draw = ImageDraw.Draw(preview)
    sample = '任務目標  保存進度  遊戲設置  回到遊戲  60/100'
    for row, (name, size) in enumerate([('16Point.fnt', 18), ('20Point.fnt', 19), ('24Point.fnt', 22)]):
        live = ROOT.parent / 'DATA' / name
        saved = backup / name
        if not saved.exists():
            saved.write_bytes(live.read_bytes())
        original = saved.read_bytes()
        data = bytearray(original)
        w, h = struct.unpack_from('<HH', data, 100)
        stride, record = data[104:106]
        assert record == stride * h and len(data) == 180 + 65536 * (record + 2)
        metrics = 180 + 65536 * record
        font = ImageFont.truetype(str(FONT), size, index=0)
        changed = 0
        for code in coverage:
            if not 32 <= code < 65536 or 0xd800 <= code < 0xe000:
                continue
            char = chr(code)
            # Keep the engine's fixed cell allocation; update its advance table.
            box = font.getbbox(char, anchor='ls')
            advance = min(w, max(1, round(font.getlength(char))))
            glyph = Image.new('L', (stride * 8, h), 255)
            gd = ImageDraw.Draw(glyph)
            gd.text((max(0, -box[0]), h - 4), char, font=font, anchor='ls', fill=0)
            bits = glyph.point(lambda value: 255 if value >= 144 else 0, mode='1').tobytes()
            assert len(bits) == record
            data[180 + code * record:180 + (code + 1) * record] = bits
            data[metrics + code * 2:metrics + code * 2 + 2] = bytes([advance, min(h, max(1, box[3] - box[1]))])
            changed += 1
        data[4:36] = b'Dragon UI Bitmap'.ljust(32, b'\0')
        data[106:108] = bytes([round(font.getlength('0')), size])
        (output / name).write_bytes(data)
        # Preview the actual serialized glyphs, using their serialized advances.
        x = 12
        for char in sample:
            code = ord(char)
            glyph = Image.frombytes('1', (stride * 8, h), bytes(data[180 + code * record:180 + (code + 1) * record]))
            advance = data[metrics + code * 2]
            preview.paste(glyph.crop((0, 0, advance, h)).convert('RGB'), (x, row * 65 + 25))
            x += advance + 1
        draw.text((12, row * 65 + 5), name + ' / Dragon UI Bitmap ' + str(size) + 'px', fill='#222222')
        records.append({'name': name, 'original_sha256': hashlib.sha256(original).hexdigest(),
                        'sha256': hashlib.sha256(data).hexdigest(), 'size': size, 'replaced_glyphs': changed})
    (output / 'manifest.json').write_text(json.dumps({'font': str(FONT), 'files': records}, indent=2))
    preview.save(ROOT / 'reports/readable-font-preview.png')
    print(json.dumps(records, indent=2))


if __name__ == '__main__':
    build()
