"""Build a reversible, exact-build native widescreen prototype (no installation).

Keeps the legacy 1024-wide UI coordinate system for asset selection. Expands
the actual render target and world viewport, docks the intact HUD at the bottom,
and gives input clipping the physical canvas dimensions. No gameplay changes.
Build dependency: pefile and capstone (already present in this workspace).
"""
import argparse
import hashlib
import json
import pathlib
import struct
import pefile
import capstone
import importlib.util

ROOT = pathlib.Path(__file__).resolve().parent
ORIGINAL = '5f31d5ac6080a54649580ef6ce30cdb748e72919a54475a0342b8c3f8bf58444'


def u32(n):
    return struct.pack('<I', n & 0xffffffff)


def align(n, a):
    return (n + a - 1) // a * a


def build(width, height, smooth_fonts=False, modern_hud=False):
    if not (1024 <= width <= 3840 and 768 <= height <= 2160):
        raise ValueError('Supported prototype range: 1024..3840 by 768..2160')
    original = (ROOT / 'baseline/dragonthrone.exe').read_bytes()
    top_height = 68 if modern_hud else 20
    if hashlib.sha256(original).hexdigest() != ORIGINAL:
        raise ValueError('Baseline executable hash mismatch')
    pe = pefile.PE(data=original)
    if pe.OPTIONAL_HEADER.ImageBase != 0x400000 or pe.OPTIONAL_HEADER.DllCharacteristics & 0x40:
        raise ValueError('Unexpected executable relocation model')
    image = bytearray(original)
    edits = []
    touched = set()
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    dis.skipdata = True

    def patch(va, before, after, reason):
        if len(before) != len(after):
            raise ValueError('Patch must preserve original span length')
        offset = pe.get_offset_from_rva(va - 0x400000)
        if bytes(image[offset:offset + len(before)]) != before:
            raise ValueError('Unexpected instruction at ' + hex(va))
        span = set(range(offset, offset + len(before)))
        if touched.intersection(span):
            raise ValueError('Overlapping edits at ' + hex(va))
        touched.update(span)
        image[offset:offset + len(before)] = after
        edits.append({'va': hex(va), 'offset': offset, 'before': before.hex(), 'after': after.hex(), 'reason': reason})

    def immediate(va, old, new, reason):
        patch(va, u32(old), u32(new), reason)

    origin_spec = importlib.util.spec_from_file_location('world_origin', ROOT / 'WorldOriginPatch.py')
    origin = importlib.util.module_from_spec(origin_spec)
    origin_spec.loader.exec_module(origin)
    origin.apply(patch)

    def instruction_immediate(va, old, new, reason):
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        decoder.detail = True
        ins = next(decoder.disasm(pe.get_data(va - 0x400000, 15), va))
        if ins.imm_size != 4:
            raise ValueError('Expected a 32-bit immediate at ' + hex(va))
        immediate(va + ins.imm_offset, old, new, reason)

    # Independent RX code and RW state sections; the original fixed-base image
    # has no relocation directory and is not ASLR-enabled.
    sa, fa = pe.OPTIONAL_HEADER.SectionAlignment, pe.OPTIONAL_HEADER.FileAlignment
    last = pe.sections[-1]
    code_rva = align(last.VirtualAddress + max(last.Misc_VirtualSize, last.SizeOfRawData), sa)
    code_capacity = sa * 4
    state_rva = code_rva + code_capacity
    code_va, state_va = 0x400000 + code_rva, 0x400000 + state_rva
    physical_w, physical_h = state_va + 16, state_va + 20
    code = bytearray()

    def jump(source, target, opcode=b'\xe9'):
        return opcode + u32(target - source - 5)

    def add_stub(data):
        while len(code) % 16:
            code.append(0x90)
        address = code_va + len(code)
        code.extend(data)
        return address

    # Track the dimensions chosen for every display mode, including menus.
    replay = pe.get_data(0x4893dc - 0x400000, 9)
    assert replay == bytes.fromhex('6a006a006866040000')
    stub = b'\x89\x35' + u32(physical_w) + b'\x89\x3d' + u32(physical_h) + replay
    target = add_stub(stub)
    code.extend(jump(target + len(stub), 0x4893e5))
    patch(0x4893dc, replay, jump(0x4893dc, target) + b'\x90' * 4, 'Record physical display dimensions without changing logical UI selectors')
    immediate(0x4893bd, 1024, width, 'Mode 7 render width')
    immediate(0x4893c2, 768, height, 'Mode 7 render height')
    immediate(0x488ec8, 1024, width, 'Window enforcement width comparison')
    immediate(0x488ed9, 768, height, 'Window enforcement height comparison')
    immediate(0x488ee2, 768, height, 'Window enforcement height')
    immediate(0x488ee7, 1024, width, 'Window enforcement width')

    # Preserve original HUD asset dimensions. All map rectangles are derived
    # from this origin by the original setup and share the click translation.
    immediate(0x568c31, 561, height - 207, 'Dock intact 1024x207 HUD at physical bottom')

    # Extend panel clipping, while keeping the original bitmap dimensions.
    def hook(va, length, payload, reason):
        before = pe.get_data(va - 0x400000, length)
        target = add_stub(payload)
        code.extend(jump(target + len(payload), va + length))
        patch(va, before, jump(va, target) + b'\x90' * (length - 5), reason)

    hook(0x568c35, 10, b'\xc7\x46\x10' + u32(width) + b'\xc7\x46\x14' + u32(207),
         'Full-width HUD clipping and input rectangle')
    immediate(0x568c42, 1024 * 207, width * 207, 'Full-width HUD area')
    hook(0x56d1b9, 6, b'\xc7\x46\x10' + u32(width) + b'\x8d\x46\x50',
         'Full-width top bar clipping rectangle')
    immediate(0x56d1d5, 1024 * 20, width * top_height, 'Full-width top bar area')
    if modern_hud:
        immediate(0x56d1ce, 20, top_height, 'Reserve a readable persistent city-resource row')

    # The minimap keeps its original bitmap source coordinates. Only its
    # destination rectangle moves; setup derives drawing and input from it.
    delta = width - 1024
    instruction_immediate(0x568f5d, 863, 863 + delta, 'Anchor primary minimap destination to the right edge')
    instruction_immediate(0x57c2b6, 751, 751 + delta, 'Place action grid next to the right minimap, including its input origin')
    instruction_immediate(0x56d364, 969, 969 + delta, 'Anchor top settings button to the right')
    instruction_immediate(0x56d384, 999, 999 + delta, 'Anchor top exit button to the right')
    for va in [0x5693f7, 0x56950d, 0x56973a]:
        instruction_immediate(va, 187, 187 + delta, 'Move right-map status icon drawing and hit testing together')

    def background_compositor(parts):
        # Same nine-argument thiscall as the game's bitmap blit. Non-1024
        # resources and the original 800 mode take the original method.
        data = bytearray(b'\x81\x79\x0c' + u32(1024))
        branches = []
        branches.append(len(data)); data.extend(b'\x0f\x85' + b'\0' * 4)
        data.extend(b'\x81\x7c\x24\x10' + u32(1024))
        branches.append(len(data)); data.extend(b'\x0f\x85' + b'\0' * 4)
        data.extend(b'\x60\x8b\xec')  # pushad; mov ebp,esp
        for destination_x, source_x, part_width in parts:
            # At EBP+36: target, dx, dy, width, height, sx, sy, flags, clip.
            data.extend(b'\xff\x75\x44\xff\x75\x40\xff\x75\x3c')
            data.extend(b'\x68' + u32(source_x) + b'\xff\x75\x34\x68' + u32(part_width))
            data.extend(b'\xff\x75\x2c\x8b\x45\x28\x05' + u32(destination_x) + b'\x50\xff\x75\x24')
            data.extend(b'\x8b\x4d\x18\x8b\x11\xff\x52\x0c')
        data.extend(b'\x61\xc2\x24\x00')
        fallback = len(data)
        data.extend(b'\x8b\x11\xff\x62\x0c')
        for offset in branches:
            data[offset + 2:offset + 6] = u32(fallback - offset - 6)
        return add_stub(data)

    # Tile a plain part of the original artwork; never stretch icons/text.
    bottom_parts = [(0, 0, 750)]
    hook(0x569463, 6, b'\x8b\x47\x10\xba' + u32(740),
         'Clip information-panel artwork at its closing frame instead of spilling into the world')
    # The information subpanel adds 40 legacy pixels under the old action
    # grid. Once that grid moves right, this previously hidden strip shows.
    # Cap this subpanel alone; never narrow the shared target or action grid.
    payload = bytearray(b'\x50\x81\x3d' + u32(0x8e02ff) + u32(1024))
    skip_mode = len(payload); payload.extend(b'\x75\0')
    payload.extend(b'\xb8' + u32(740) + b'\x2b\x86' + u32(0x719) + b'\x39\xc1')
    skip_small = len(payload); payload.extend(b'\x7e\0')
    payload.extend(b'\x89\xc1')
    finish = len(payload)
    payload.extend(b'\x58' + pe.get_data(0x57ca08 - 0x400000, 6))
    for offset in (skip_mode, skip_small):
        payload[offset + 1] = finish - offset - 2
    hook(0x57ca08, 6, payload, 'Remove the information subpanel extension previously hidden under the action grid')
    # Leave the middle open to the world instead of inventing a filler panel.
    bottom_parts.append((width - 284, 740, 284))
    top_parts = [(0, 0, 1024)]
    for x in range(1024, width, 128):
        top_parts.append((x, 500, min(128, width - x)))
    bottom_blit = background_compositor(bottom_parts)
    top_blit = background_compositor(top_parts)
    for va, target, pushes in [(0x569373, bottom_blit, b'\x50\x57'),
                               (0x5693a7, bottom_blit, b'\x50\x57'),
                               (0x56e537, top_blit, b'\x50\x56')]:
        while len(code) % 16:
            code.append(0x90)
        stub_address = code_va + len(code)
        payload = pushes + jump(stub_address + 2, target, b'\xe8')
        hook(va, 5, payload, 'Compose native-width UI background from original artwork')

    world_height = height - top_height
    # Replace the large-mode allocation+rectangle block as one coherent unit.
    before = pe.get_data(0x5d4a60 - 0x400000, 0x30)
    block = b'\x68' + u32(world_height) + b'\x68' + u32(width) + b'\x8b\xce'
    while len(code) % 16:
        code.append(0x90)
    target = code_va + len(code)
    block += jump(target + len(block), 0x5d43a0, b'\xe8')
    for offset, value in [(8, 0), (12, top_height), (16, width), (20, world_height), (24, width * world_height)]:
        block += b'\xc7\x46' + bytes([offset]) + u32(value)
    block += jump(target + len(block), 0x5d4ac6)
    code.extend(block)
    patch(0x5d4a60, before, jump(0x5d4a60, target) + b'\x90' * (len(before) - 5), 'Allocate and describe native world viewport together')

    # World drawing covers the open middle below the HUD line. Legacy input
    # code still reads a rectangle: shorten that rectangle under either HUD
    # section, and retain full height over the opening. Drawing always restores
    # full height for the duration of the renderer call.
    replay = pe.get_data(0x5d5fd0 - 0x400000, 5)
    assert replay == bytes.fromhex('5355568bf1')
    trampoline = add_stub(replay)
    code.extend(jump(trampoline + 5, 0x5d5fd5))
    while len(code) % 16:
        code.append(0x90)
    target = code_va + len(code)
    wrapper = bytearray(b'\x81\x79\x10' + u32(width))
    fallback_branch = len(wrapper); wrapper.extend(b'\x0f\x85' + b'\0' * 4)
    wrapper.extend(b'\x55\x8b\xec\x56\x8b\xf1\xc7\x46\x14' + u32(world_height))
    wrapper.extend(b'\xff\x75\x08\x8b\xce')
    wrapper.extend(jump(target + len(wrapper), trampoline, b'\xe8'))
    wrapper.extend(b'\xc7\x46\x14' + u32(height - 207 - top_height) + b'\xa1' + u32(0x8b9d60))
    wrapper.extend(b'\x3d' + u32(750))
    left_branch = len(wrapper); wrapper.extend(b'\x0f\x8c' + b'\0' * 4)
    wrapper.extend(b'\x3d' + u32(width - (454 if modern_hud else 284)))
    right_branch = len(wrapper); wrapper.extend(b'\x0f\x8d' + b'\0' * 4)
    wrapper.extend(b'\xc7\x46\x14' + u32(world_height))
    finish = len(wrapper)
    wrapper.extend(b'\x8b\x46\x10\x0f\xaf\x46\x14\x89\x46\x18\x5e\x5d\xc2\x04\x00')
    fallback = len(wrapper)
    wrapper.extend(jump(target + len(wrapper), trampoline))
    for offset, destination in [(fallback_branch, fallback), (left_branch, finish), (right_branch, finish)]:
        wrapper[offset + 2:offset + 6] = u32(destination - offset - 6)
    code.extend(wrapper)
    patch(0x5d5fd0, replay, jump(0x5d5fd0, target), 'Draw world behind separated HUD sections; limit world input under panels')
    # The world now redraws behind the panels, so invalidate their children
    # together before painting. This also preserves selected-unit details.
    while len(code) % 16:
        code.append(0x90)
    target = code_va + len(code)
    # Preserve the minimap's two-stage cache update. Resetting it to stage 2
    # on every frame prevents stage 1 from ever presenting either map.
    payload = b'\x60\x8b\x46\x7c\xff\x70\x4c\x8b\xce'
    payload += jump(target + len(payload), 0x5690b0, b'\xe8')
    payload += b'\x5a\x8b\x46\x7c\x89\x50\x4c\x61\x8b\x06\xff\x50\x24'
    hook(0x56932e, 5, payload, 'Refresh the complete HUD over the expanded world')

    # Stage 2 updates both map caches but originally presents neither until
    # the following frame. The expanded world erases that retained frame.
    # Finish stage 1 immediately for BOTH maps, then restore the stage value
    # so the existing decrement and dirty-state logic remain unchanged.
    while len(code) % 16:
        code.append(0x90)
    target = code_va + len(code)
    payload = bytearray(b'\x83\x7e\x4c\x02')
    skip_present = len(payload); payload.extend(b'\x0f\x85' + b'\0' * 4)
    payload.extend(b'\x60\xc7\x46\x4c' + u32(1) + b'\x57\x8b\xce')
    payload.extend(jump(target + len(payload), 0x574430, b'\xe8'))
    payload.extend(b'\x83\xbe\xf2\x02\x00\x00\xff')
    skip_secondary = len(payload); payload.extend(b'\x0f\x84' + b'\0' * 4)
    payload.extend(b'\x57\x8b\xce')
    payload.extend(jump(target + len(payload), 0x574750, b'\xe8'))
    restore_stage = len(payload)
    payload.extend(b'\xc7\x46\x4c' + u32(2) + b'\x61')
    replay_offset = len(payload)
    replay = pe.get_data(0x574a24 - 0x400000, 6)
    assert replay == bytes.fromhex('8b166a008bce')
    payload.extend(replay)
    for offset, destination in [(skip_present, replay_offset), (skip_secondary, restore_stage)]:
        payload[offset + 2:offset + 6] = u32(destination - offset - 6)
    hook(0x574a24, 6, payload, 'Present both minimaps on cache-refresh frames without a blank intermediate frame')

    # Only input/tooltip clamps and edge scrolling use the physical canvas;
    # widget resource selectors deliberately continue seeing 800 or 1024.
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        for ins in dis.disasm(section.get_data(), 0x400000 + section.VirtualAddress):
            if not (0x48a500 <= ins.address < 0x48b800 or 0x5d6200 <= ins.address < 0x5d6400):
                continue
            for old, new in [(0x8e02ff, physical_w), (0x8e0303, physical_h)]:
                index = bytes(ins.bytes).find(u32(old))
                if index >= 0:
                    immediate(ins.address + index, old, new, 'Physical input/tooltip/edge-scroll bounds')

    # These exact UI-only GetSystemMetrics(0) callers choose legacy resources.
    # POP consumes the stdcall argument; MOV loads logical width without flags.
    for va in [0x50ce23, 0x56d19d, 0x57c087, 0x57c28b, 0x57c9ea]:
        patch(va, bytes.fromhex('ff1540746800'), b'\x58\xa1' + u32(0x8e02ff), 'Choose legacy UI assets despite native display width')
    immediate(0x50ce46, 528, height - 240, 'Keep message strip above bottom HUD')
    immediate(0x564d1f, 1024, width, 'Center dialog in native world width')
    immediate(0x564d2e, 541, height - 227, 'Center dialog above HUD height')
    immediate(0x564b04, 1024, width, 'Initialize dialog content and frame at the same native X origin')
    immediate(0x564b13, 541, height - 227, 'Initialize dialog content and frame at the same native Y origin')
    # Child-specific builders can overwrite their content origin with legacy
    # coordinates. Re-anchor the content before its final layout pass.
    payload = bytes.fromhex('8b4e588b460883c00a89416c8b460c83c00a8941706a00')
    hook(0x564da6, 5, payload, 'Align popup content origin before the child layout pass')
    instruction_immediate(0x48a32b, 1024, width, 'Expand independent tooltip font clipping width')
    instruction_immediate(0x48a333, 768, height, 'Expand independent tooltip font clipping height')
    instruction_immediate(0x48a33e, 1024 * 768, width * height, 'Expand tooltip font clipping area')
    # Every new hover resets this independently of the constructor.
    instruction_immediate(0x48a5b3, 1024, width, 'Retain native tooltip font clipping when hover text changes')
    instruction_immediate(0x48a5bb, 768, height, 'Retain native tooltip font height when hover text changes')
    instruction_immediate(0x48a5c3, 1024 * 768, width * height, 'Retain native tooltip clip area on each hover')
    # Resource/cost tooltips have a separate initialization path. Without an
    # optional description it inherits the last tooltip's clip rectangle;
    # with a description it used another literal 1024x768 rectangle.
    instruction_immediate(0x48b52b, 1024, width, 'Native text clip width for resource/cost tooltips')
    instruction_immediate(0x48b533, 768, height, 'Native text clip height for resource/cost tooltips')
    instruction_immediate(0x48b53b, 1024 * 768, width * height, 'Native resource/cost tooltip clip area')
    payload = bytearray(b'\x50\x51\x8b\x86\x86\x04\x00\x00')
    payload.extend(b'\xc7\x40\x21' + u32(0) + b'\xc7\x40\x25' + u32(0))
    payload.extend(b'\x8b\x0d' + u32(physical_w) + b'\x89\x48\x29')
    payload.extend(b'\x8b\x0d' + u32(physical_h) + b'\x89\x48\x2d')
    payload.extend(b'\x0f\xaf\x48\x29\x89\x48\x31\x59\x58')
    replay = pe.get_data(0x48b489 - 0x400000, 8)
    assert replay == bytes.fromhex('8b6c24548b542450')
    payload.extend(replay)
    hook(0x48b489, 8, payload, 'Reset resource/cost text clipping even when no description is supplied')

    font_data = None
    font_rva = state_rva + sa
    if smooth_fonts:
        spec = importlib.util.spec_from_file_location('smooth_fonts', ROOT / 'Build-SmoothFonts.py')
        smooth = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(smooth)
        atlas_path = ROOT / 'builds/smooth-fonts/coverage.bin'
        font_data = atlas_path.read_bytes() if atlas_path.exists() else smooth.build_atlas()
        before = pe.get_data(0x65ff40 - 0x400000, 5)
        assert before == bytes.fromhex('538b5c2428')
        original_font = add_stub(before)
        code.extend(jump(original_font + 5, 0x65ff45))
        if len(code) > 0x1000:
            raise ValueError('UI hooks overlap reserved font-code page')
        font_code, font_entry = smooth.compile_hook(code_va + 0x1000, 0x400000 + font_rva, original_font)
        code.extend(b'\x90' * (0x1000 - len(code)))
        code.extend(font_code)
        patch(0x65ff40, before, jump(0x65ff40, font_entry),
              'Blend grayscale glyph coverage directly onto RGB565 targets; preserve original clipping and advances')

    if len(code) > code_capacity:
        raise ValueError('Code section exceeds allocated virtual span')
    state = b'DTWNATIVE1\0'.ljust(16, b'\0') + u32(800) + u32(600) + u32(width) + u32(height)
    if modern_hud:
        state += b'DTMODERN1\0'
    section_table = pe.sections[-1].get_file_offset() + 40
    payloads = [
        (b'.wcode', code_rva, code, 0x60000020),
        (b'.wdata', state_rva, state, 0xc0000040),
    ]
    if font_data is not None:
        payloads.append((b'.wfont', font_rva, font_data, 0x40000040))
    if section_table + 40 * len(payloads) > pe.OPTIONAL_HEADER.SizeOfHeaders:
        raise ValueError('Insufficient header space')
    for index, (name, rva, payload, flags) in enumerate(payloads):
        raw = align(len(image), fa)
        raw_size = align(len(payload), fa)
        image.extend(b'\0' * (raw + raw_size - len(image)))
        image[raw:raw + len(payload)] = payload
        virtual_size = code_capacity if name == b'.wcode' else len(payload)
        header = struct.pack('<8sIIIIIIHHI', name, virtual_size, rva, raw_size, raw, 0, 0, 0, 0, flags)
        image[section_table + index * 40:section_table + (index + 1) * 40] = header
    struct.pack_into('<H', image, pe.FILE_HEADER.get_field_absolute_offset('NumberOfSections'), len(pe.sections) + len(payloads))
    image_end = align(font_rva + len(font_data), sa) if font_data is not None else state_rva + sa
    struct.pack_into('<I', image, pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfImage'), image_end)
    struct.pack_into('<I', image, pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfCode'), pe.OPTIONAL_HEADER.SizeOfCode + align(len(code), fa))
    struct.pack_into('<I', image, pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfInitializedData'), pe.OPTIONAL_HEADER.SizeOfInitializedData + align(len(state), fa) + (align(len(font_data), fa) if font_data is not None else 0))
    struct.pack_into('<I', image, pe.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum'), 0)
    check = pefile.PE(data=bytes(image))
    struct.pack_into('<I', image, pe.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum'), check.generate_checksum())
    destination = ROOT / 'builds' / ('native-' + str(width) + 'x' + str(height) + ('-smooth' if smooth_fonts else '') + ('-modern' if modern_hud else ''))
    destination.mkdir(parents=True, exist_ok=True)
    (destination / 'dragonthrone.exe').write_bytes(image)
    manifest = {'status': 'experimental-unverified', 'original_sha256': ORIGINAL,
                'patched_sha256': hashlib.sha256(image).hexdigest(), 'width': width, 'height': height,
                'font_renderer': 'grayscale-rgb565' if smooth_fonts else 'original-bitmap',
                'modern_hud': modern_hud, 'top_height': top_height,
                'physical_width_address': hex(physical_w), 'physical_height_address': hex(physical_h),
                'hud_policy': 'Left information panel; right action grid and minimap; world visible in the middle; right top controls',
                'edits': edits}
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    (destination / 'stubs.txt').write_text('\n'.join(hex(i.address) + ' ' + i.mnemonic + ' ' + i.op_str for i in dis.disasm(bytes(code), code_va)), encoding='utf-8')
    print(destination, '\nPatches:', len(edits), '\nSHA256:', manifest['patched_sha256'])
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--width', type=int, default=1920)
    parser.add_argument('--height', type=int, default=1080)
    parser.add_argument('--smooth-fonts', action='store_true')
    parser.add_argument('--modern-hud', action='store_true')
    args = parser.parse_args()
    build(args.width, args.height, args.smooth_fonts, args.modern_hud)
