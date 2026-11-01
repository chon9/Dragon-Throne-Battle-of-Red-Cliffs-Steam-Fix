# Building and testing

The player ZIP needs only Windows PowerShell. The following instructions are for developers. They build a separate output; they do not overwrite your installed game.

## Dependencies

- Windows, Python 3.9+ and `python -m pip install -r requirements.txt`.
- Visual Studio C++ x86 tools and a Windows SDK. Defaults match the tested MSVC **14.29.30133** / SDK **10.0.16299.0** environment. Set `DT_MSVC_ROOT`, `DT_MSVC_BIN` (Hostx64/x86), `DT_WINDOWS_SDK_ROOT`, and `DT_WINDOWS_SDK_VERSION` for another installation. Other compilers/SDK revisions have not been validated.
- A legally obtained compatible **Steamworks SDK**, with `SteamNetworkingMessages002` and the interfaces recorded in `modernization/network/vendor/provenance.json`. Copy `public/steam/*.h` to `modernization/network/vendor/steam/`; copy the x86 `redistributable_bin/steam_api.lib` and `steam_api.dll` to `modernization/network/vendor/redistributable_bin/`. These paths are ignored by Git. The release script pins the previously tested Valve-signed runtime hash; changing SDK/runtime versions requires a separate compatibility review.
- Run `python modernization/Fetch-Public-Dependencies.py` for pinned Noto and cnc-ddraw downloads. Wine's DirectPlay declarations and license are included in source.

Do not commit SDK headers, import libraries, game binaries, original assets or generated bitmap atlases.

## Original local baseline

Copy the **unmodified** supported Steam `dragonthrone.exe` into `modernization/baseline/dragonthrone.exe`. Copy `DATA/16Point.fnt`, `20Point.fnt`, and `24Point.fnt` into `modernization/baseline/fonts/`. These directories are ignored by Git. The executable SHA-256 must be:

```text
5f31d5ac6080a54649580ef6ce30cdb748e72919a54475a0342b8c3f8bf58444
```

Use a separate clean Steam copy or a verified original backup. Never substitute a previously patched executable as the baseline.

## Build

From the repository root:

```powershell
New-Item -ItemType Directory -Force modernization/builds,modernization/reports | Out-Null
python modernization/Build-ReadableFonts.py
python modernization/Build-SmoothFonts.py
python modernization/network/Build.py
python modernization/Build-SmoothFonts.py --test
python modernization/Release.py
python modernization/Test-PublicPackage.py
```

Check each command succeeds before continuing. Font generation may take several minutes. `Release.py` verifies the Noto source/atlas hashes, generates the modern 1080p executable, redirects the three COM imports to DTNet, and creates compressed deltas plus a reversible installer in `modernization/dist/`. It refuses an existing version staging directory to avoid stale source/dependency files; use a fresh checkout or a new release version for a rebuild.

`Test-PublicPackage.py` uses a separate temporary game fixture, verifies all installed output hashes, rejects altered inputs, checks exact rollback, and preserves save/settings sentinels. Pass `--previous path/to/DragonFullFix-0.3.1-1080p-public-beta.zip` to additionally test upgrading from the preceding public release through Restore and Install.

## Source map

| File | Purpose |
| --- | --- |
| `modernization/Build-Widescreen.py` | Guarded exact-build binary patches, world viewport, split HUD, input bounds and font hook placement |
| `modernization/WorldOriginPatch.py` | Five exact-byte corrections aligning sprites and building overlap with the actual viewport origin |
| `modernization/SmoothFont.cpp` | Grayscale glyph coverage composited into RGB565 |
| `modernization/network/ModernUI.h` | Resource row, retained resource icons, command reflow and paint/hit geometry |
| `modernization/network/DTNet.cpp` | DirectPlay4 adapter using Steam lobbies and reliable messages |
| `modernization/network/Protocol.h` | Session/player packet validation and join codes |
| `modernization/network/LatencyStats.h` | Diagnostic timing windows; no simulation scheduling changes |
| `modernization/FullFix-Install.ps1` | Payload/hash checks, staging, backups and rollback |
| `modernization/FullFix-Language.ps1` | Existing English/Traditional Chinese resource selection |

## What tests establish

The C++ build runs protocol validation, timing statistics, action geometry/transparent backing, command-list rebuild freeze regressions, isolated provider state tests, and execution of all five x86 world-origin replacements. The font test covers RGB565 compositing, clipping, transparent pixels and balanced surface locks. These are local regression tests, not proof of live multiplayer timing, every UI state, English text fit or 4K compatibility.

The game still owns its simulation, game data and artwork. Hard-coded addresses and 1080p geometry intentionally support one executable only. A configurable mod API, offline LAN and reconnect support are future work.
