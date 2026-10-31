## First public release: v0.3.1 beta

Community fixes for the Windows Steam edition of **Dragon Throne: Battle of Red Cliffs**.

### Download and install

Download **DragonFullFix-0.3.1-1080p-public-beta.zip**, not GitHub's automatic source archive. Close the game, extract `DragonFullFix` inside the game folder, and run `Install.cmd`. Launch through Steam and select the game's **1024 × 768** setting to activate native **1920 × 1080** missions.

If you already installed a private 0.2/0.3 full-fix pack, run **Restore.cmd first**, then install this version. Keep your `modernization/history` backups. Optional **Language.cmd** selects English or Traditional Chinese independently for each player.

### Included

- Native 1080p, corrected split HUD, minimaps, menus and tooltips.
- Larger unified resource bar and right-side command buttons using original panel artwork.
- Corrected command-panel header spacing and selection redraws.
- Smooth Noto-derived bitmap text, borderless cnc-ddraw and a 60 FPS presentation limit.
- Steam join-code multiplayer and command-latency diagnostics.
- Reversible installer, source snapshot, MIT project license and third-party notices.

### Why beta?

The final HUD layout was confirmed in game, and the user and a friend confirmed multiplayer with the preceding adapter. The networking protocol is unchanged. This public build replaces private JhengHei-derived font data with OFL-licensed Noto derivatives; the new font passed generation/raster tests but still needs wider in-game and English-language visual testing.

Input delay remains under investigation. Native 4K, offline LAN, host migration and reconnect are not provided by this release. The FPS setting is a display limit, not a guaranteed simulation rate.

### Verification

Local tests passed for font compositing/clipping, command paint/hit rectangles, transparent backing and selection transitions, protocol validation, timing statistics and provider state. Fresh installation, damaged-file rejection, upgrade from the private 0.3 pack, exact rollback and save/settings preservation passed against this ZIP.

The archive excludes full game executables, original game asset collections, proprietary SDK headers, personal saves and logs. It requires the supported original Steam game files. Runtime `steam_api.dll` is Valve-signed. See the README for the supported executable hash and license boundaries.

SHA-256:

```text
2b7c1e39c1ee5f176e051a5836d8b8fb1fcec1ac9c3b6b86db52e9636eb8781e
```

Please report issues with your Windows version, language, resolution and reproduction steps. For multiplayer delays, include reviewed excerpts from `DTNet.log` after a short active two-player match.

[Read the guide](https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix#readme) · [Support chon9 on Ko-fi](https://ko-fi.com/chon9workshop)
