# Third-party notices

The root MIT license covers original code and documentation written for this project. It does not replace the following rights and licenses.

| Component | Terms and distribution |
| --- | --- |
| Dragon Throne game executable, original fonts, audio and artwork | Proprietary, respective rights holders. No complete game executable, original asset collection or saved games are distributed. Binary deltas require the user's supported original files. |
| Gameplay screenshots | Illustrative captures containing proprietary game artwork; excluded from the root MIT grant. Development versions are identified in the README. |
| cnc-ddraw v7.1.0.0 | Copyright (c) 2022 github.com/FunkyFr3sh; MIT. [Full notice](licenses/cnc-ddraw-MIT.txt). Runtime provenance is in `modernization/renderers/cnc-ddraw/provenance.json`. |
| Wine `dplay.h` | Copyright (C) the Wine project; LGPL-2.1-or-later. Original notices retained in `modernization/network/vendor/directplay/dplay.h`; [full license](licenses/Wine-LGPL-2.1.txt). `DirectPlayMethods.inc` is generated from its interface declarations and carries the same attribution, not a new MIT claim. No Wine runtime is linked or distributed. |
| Noto Sans CJK TC Regular | Copyright 2014–2021 Adobe (font metadata); SIL Open Font License 1.1. [Full license](licenses/Noto-OFL.txt). Upstream revision `f8d157532fbfaeda587e826d4cd5b21a49186f7c`. |
| Dragon UI Bitmap / grayscale coverage | Modified bitmap-format derivatives of Noto Sans CJK TC; remain under OFL-1.1, not MIT. Generated data is embedded in the executable delta and replacement font deltas. Original game glyphs unsupported by Noto remain in the user's local files. No Microsoft font atlas is shipped in public releases. |
| Locally installed Microsoft JhengHei | Used through Windows GDI for the resource bar at runtime. No Microsoft font file or converted JhengHei glyph atlas is included in the public release. |
| Valve Steamworks | Copyright Valve Corporation; proprietary SDK terms. SDK headers are local build dependencies and excluded from this repository and release source snapshot. The x86 `steam_api.dll` is a Valve-signed redistributable supplied alongside the adapter, not licensed under MIT. [SDK agreement](https://partner.steamgames.com/documentation/sdk_access_agreement), [API distribution documentation](https://partner.steamgames.com/doc/sdk/api). |

Font source: [Noto CJK](https://github.com/notofonts/noto-cjk). Noto OTF SHA-256: `dce08bd4fd91aa8aa76ed8fea4b694c2dfb8550f67871e326843212ddbeb88b4`.

Steam runtime SHA-256: `67ae11d71ae6ec404090094df1e47b614d27400dc53fa450023e6fbcf347902c`. The existing private builds used SDK interface headers pinned in `modernization/network/vendor/provenance.json`; this file records provenance, not redistribution permission for those headers. Builders should obtain a compatible SDK under Valve's terms.

Python, Pillow, fontTools, pefile, capstone and the Microsoft compiler/Windows SDK are developer-installed tools; their runtimes are not included in the player ZIP. Their own licenses apply to those tools.

This project does not claim ownership of the original game or endorsement by its developers, publisher, Valve, Microsoft, Adobe or the projects credited above.
