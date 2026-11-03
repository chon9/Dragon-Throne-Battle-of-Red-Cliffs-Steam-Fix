## Classic HUD and bottom-gap fix

- Restores the original resource bar/popup and command-button sizes. Runtime HUD replacement is removed from DTNet; the experimental enlarged HUD and F8 remapping are not included.
- Fixes units and drag-selection outlines disappearing in the open space between the bottom panels when the cursor crosses a panel. The user confirmed both stay visible and selection works.
- Keeps native 1080p widescreen, smoother text, borderless Alt+Tab, minimaps/tooltips, building-origin corrections and Steam join-code multiplayer.
- Keeps the guided updater: resolution output, English/Traditional Chinese, opening-video skip (Off by default), automatic old-pack reversal, backups and recovery.

## Install or update

Download **DragonFullFix-0.4.1-1080p-public-beta.zip**, extract it beside `dragonthrone.exe`, replacing the previous `DragonFullFix` package folder, then run **Install.cmd** or **Update.cmd**. Keep `modernization/history` and `installed-full-fix.json`. The wizard reverses a recorded older combined pack automatically; manual restoration is not required.

Launch through Steam and use the game's 1024×768 setting for native 1920×1080 missions. Higher output choices scale the 1080p image; they are not native 4K rendering. Each player can select their own language.

## Validation and limits

The release checks cover package integrity, clean install and exact rollback, automatic v0.4.0 upgrade, failed/interrupted update recovery, protocol/provider logic, absence of runtime HUD hooks, font/origin regressions, and 19,220 world/HUD boundary points through the generated x86 input guards. Local gameplay confirmed the bottom-gap correction.

This remains a beta. A friend's v0.4.0 freeze was not captured, and longer multiplayer sessions with this build remain to be tested. Multiplayer command delay is not claimed fixed. Scaled 1440p/4K visuals and long English text still need broader testing.

The ZIP contains patch deltas and required redistributables, not a full game. Use your own supported Steam copy. GitHub's automatic source archives are for developers.

[Support on Ko-fi](https://ko-fi.com/chon9workshop)
