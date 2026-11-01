"""Exact-build camera-to-screen origin corrections; no simulation changes."""
# Address, original instruction, replacement, world-pointer register, result register.
# All sites have just read camera Y from world+0x8e. The original add -20
# assumed a 20px top bar; SUB [world+0x0c] uses the actual viewport origin.
PATCHES = (
    (0x4196de, '83c0ec', '2b450c', 'ebp', 'eax'),  # building overlap clip
    (0x484726, '83c0ec', '2b470c', 'edi', 'eax'),  # general sprite clip
    (0x4b6501, '83c2ec', '2b560c', 'esi', 'edx'),  # shared overlay origin
    (0x4b653d, '83c0ec', '2b460c', 'esi', 'eax'),  # entity render origin
    (0x4b65f6, '83c7ec', '2b780c', 'eax', 'edi'),  # effect render origin
)

def apply(patch):
    for address,before,after,_,_ in PATCHES:
        patch(address,bytes.fromhex(before),bytes.fromhex(after),
              'Use actual world Y origin for sprite/building overlap rendering')
