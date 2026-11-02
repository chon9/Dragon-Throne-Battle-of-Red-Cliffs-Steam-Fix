# Dragon Throne: Battle of Red Cliffs — Steam Fix

A community patch for the Windows Steam edition: native 1080p gameplay, clearer HUD and text, borderless Alt+Tab, and multiplayer through Steam join codes.

[Download the release](https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix/releases) · [Report a bug](https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix/issues) · [Support on Ko-fi](https://ko-fi.com/chon9workshop)

**Requires your own Steam copy of the game.** This repository contains patch code, not the game's source code or a standalone game. It is an unofficial community project and is not affiliated with the game's developers, publisher, or Valve.

## Features

- Native **1920 × 1080** mission rendering with separated bottom panels and working minimaps, menu entries and tooltips.
- A larger resource bar displaying the viewed/selected owned city's stocks, plus population and player gold.
- Larger right-side command buttons inside the original panel artwork; corrected placement and selection redraws.
- Fixes for duplicated buildings near units and the command-panel redraw freeze after rebuilding the button list (v0.3.2), including the remaining「任命」appointment case (v0.3.3).
- Larger, antialiased text. Public releases use **Dragon UI Bitmap**, derived from OFL-licensed Noto Sans CJK TC. The top bar uses the locally installed Windows font at runtime.
- **cnc-ddraw borderless mode** with a 60 FPS presentation limit and easier Alt+Tab.
- **Steam join-code multiplayer** without VPN software, IPX drivers or manual router forwarding.
- Guided **Install.cmd / Update.cmd**: automatically reverse an older full-fix pack, choose display output, English / Traditional Chinese, and whether to skip opening videos.
- Verified staging, original backups, failed-update rollback and **Recover.cmd** for interrupted operations.

## Screenshots

![Native widescreen mission with separated HUD panels](docs/screenshots/widescreen.png)

*Existing screenshot from the earlier 1080p build. The public release adds the larger resource bar and command panel described above; bundled font shapes also differ.*

![Modern HUD development preview](docs/screenshots/modern-hud-preview.png)

*Modern HUD development screenshot supplied during testing. Taken before the final bottom-right header-spacing correction; that correction was subsequently confirmed by the tester. These are gameplay captures, not mockups.*

![Steam join-code host and join dialog](docs/screenshots/steam-multiplayer.png)

*Steam multiplayer provider and join-code dialog. Both players need Steam online.*

## Install

1. Save and close the game. **You do not need to manually restore an earlier combined full-fix pack.**
2. Download **DragonFullFix-0.4.0-1080p-public-beta.zip** from [Releases](https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix/releases). GitHub's automatic **Source code** archives are for developers, not installation.
3. In Steam, choose **Manage → Browse local files**. Extract the ZIP so the folder layout is:

   ```text
   Dragon Throne - Battle of Red Cliffs/
     dragonthrone.exe
     DragonFullFix/
       Install.cmd
       Update.cmd
       Restore.cmd
       Recover.cmd
       Language.cmd
   ```

4. Run **DragonFullFix/Install.cmd** (or **Update.cmd**, which opens the same wizard). Choose output resolution, language, and the opening-video option. Press Enter for **automatic desktop output**, **keep current language**, and **skip videos Off**.
5. Wait for **Setup complete**. The installer verifies the new payload and old backups, automatically reverses the recorded older patch, and installs the new one. Do not copy files out of `payload` manually.
6. Launch through **Steam**. Select the game's **1024 × 768** setting: this legacy option now selects **1920 × 1080 during missions**. The main menu retains its legacy layout.

The installer supports one exact original Steam executable, SHA-256:

```text
5f31d5ac6080a54649580ef6ce30cdb748e72919a54475a0342b8c3f8bf58444
```

Modified or different executable/font files are rejected before installation. Restore other HD/native/network mods using their own installers first. If Steam updates the game, wait for a compatible patch rather than bypassing hash checks. Allow at least 1 GB free for staging and rollback snapshots; retained update history grows with each installation. Players do not need Python or a compiler.

### Updating and uninstalling

Close the game, extract the new ZIP into the same game folder, replacing the old `DragonFullFix` package folder, then run **Install.cmd** or **Update.cmd**. Keep `modernization/history` and `modernization/installed-full-fix.json`: they identify and preserve your original files. Automatic upgrade is tested from the private **0.2 / 0.3** friend packs and public **0.3.3**. Earlier public packs use the same record format. This is a local package updater; it does not download releases in the background.

The updater stages and verifies new files **before** reversing the old patch. A failed operation attempts to restore the previous installed version and its preferences. If the updater was interrupted, run **Recover.cmd**, then rerun the wizard. Changed game files, missing/corrupt backups and unrelated layered mods are protected and block automatic updates; keep the diagnostic message and backups instead of bypassing the checks.

Run **Restore.cmd** to remove the full-fix pack and return to the files saved before the first combined installation. It also re-enables opening videos disabled by this installer. Saves, personal game settings and the language choice are preserved. Language has a separate backup; use option 3 in **Language.cmd** to restore it.

### Resolution and opening videos

- **Automatic desktop** (default): borderless fullscreen at your current desktop size.
- **1920 × 1080 / 2560 × 1440 / 3840 × 2160**: a borderless window at the chosen output size. The wizard only shows sizes that fit the primary monitor; it does not change Windows display resolution.
- Missions and the corrected HUD still render internally at **1920 × 1080**. Larger outputs scale that image; this is **not native 1440p/4K rendering**. A desktop of at least 1920 × 1080 is required. Scaled output needs broader visual testing. The renderer's [pinned configuration/source](https://github.com/FunkyFr3sh/cnc-ddraw/blob/v7.1.0.0/src/dd.c) defines these output-size limits.
- **Skip opening videos: Off by default** means opening videos play. Choose **On** to go straight to the menu and avoid the buggy startup playback. Only `DATA/Fod.avi` and `DATA/gpmom.avi` are renamed to `.disabled-by-dragonfix`; no video is deleted, and campaign videos are left alone. Choose Off on a subsequent run, or use Restore.cmd, to put them back.

Run the wizard again to change preferences; Enter always selects the defaults, including videos Off, rather than silently retaining the previous choices. Advanced noninteractive use:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\DragonFullFix\Setup.ps1 -GameDirectory . -NonInteractive -Resolution Desktop -Language English -SkipVideos On
```

### Language

Each player chooses their language independently. The game already supplies the translated resources; this patch does not add translated voices. The selector needs administrator access only for the game's legacy 32-bit registry language value. Keeping the current language requires no elevation. An explicit language selection runs after the file update; if elevation is declined or the language switch fails, the patch stays installed and setup reports the incomplete language step. Retry with Language.cmd or use Steam's game language setting. The switcher does not change Steam's client language. Long English text still needs broader visual testing.

## Play with a friend

Both players need their own copy and online Steam accounts. Using the same release is recommended.

**Host:** Multiplayer → **Steam - Join codes** → **Host a game**, then create a game in the normal screen. Share the displayed code privately, click Continue, and remain in the lobby.

**Friend:** Choose the same provider, paste the code, click Join, then select the session in the game's list. Use the code, not the legacy IP address field. Do not use the old session password field; anyone who has the code can attempt to join.

Steam can provide relay routing without router setup. Steam online is also required on the same LAN. Offline LAN, host migration and automatic reconnect are not implemented.

## Status and known limits

| Area | Status |
| --- | --- |
| Windows 11 / Steam / 1080p | Tested locally |
| HUD, menus, minimaps and tooltips | Iteratively tested; final command-panel layout confirmed by user |
| Building duplication / command-panel freeze | Corrections confirmed in local gameplay; regression tests added |
| Two-PC multiplayer | User and friend confirmed play on the preceding adapter; protocol unchanged |
| Public Noto-derived fonts | Build and raster tests; broader in-game validation pending |
| Command delay | Reported for move/build/train; diagnostics added, **not claimed fixed** |
| Resolution choices | 1080p internal HUD; desktop / 1080p / 1440p / 4K output configuration tested; scaled visuals pending |
| Native 4K / other aspect ratios | Not implemented or validated |
| 60 FPS | Presentation limit, not a guarantee of 60 simulation updates or minimum performance |

The release remains **beta** because broader public-font validation and other coverage gaps remain. v0.4.0 adds guided updates and video selection while retaining the v0.3.3 gameplay fixes; the network protocol and command timing are unchanged.

For multiplayer delay, play normally for at least a minute with both players connected, then attach relevant `DTNet.log` excerpts to an issue. The adapter records polling, queue and connection timing every ten seconds; it does not log chat, join codes or command payloads. Review logs before posting. Disconnect recovery and long, high-population matches need more testing.

## Development

See [BUILDING.md](BUILDING.md) for dependencies, source layout, local baseline preparation and tests. Proprietary Steamworks SDK headers/import libraries and original game files are deliberately excluded from Git. Fetch/build dependencies locally; the release ZIP contains only the permitted Steam runtime, patch payloads, custom adapter and renderer.

[CONTRIBUTING.md](CONTRIBUTING.md) describes useful bug reports and patch contributions. A configurable mod API is a future goal, not an implemented feature.

## License and credits

Original project code is [MIT licensed](LICENSE), copyright 2026 chon9. This does **not** relicense the game, its artwork, screenshots, fonts or third-party dependencies. Read [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for the exact boundaries, Wine header attribution, Noto's OFL, cnc-ddraw's MIT notice and Valve's runtime terms.

Thanks to [cnc-ddraw](https://github.com/FunkyFr3sh/cnc-ddraw), [Noto CJK](https://github.com/notofonts/noto-cjk), [Wine](https://gitlab.winehq.org/wine/wine), and the community's research and testing. The [existing HD mod](https://github.com/game-patchs/FateOfTheDragonExpansionEx-HD) and [RedirectPlay](https://github.com/Sh0wdown/RedirectPlay) were research references; their mod DLLs are not bundled here.

## Support

If this project helps you enjoy the game again, you can [support chon9's workshop on Ko-fi](https://ko-fi.com/chon9workshop). Donations are optional; the patch and source remain freely available.
