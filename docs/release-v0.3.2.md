## v0.3.2 — building redraw and freeze fixes

This beta fixes two issues found during play:

- **Duplicated buildings near units:** sprites, effects and building overlap now use the actual viewport origin beneath the taller resource bar. The tester confirmed that duplication stopped and unit selection stayed correct.
- **Picture freezing after command-list changes:** the larger button click dimensions could survive a native layout change and reach the old bitmap decoder. The fix restores the affected geometry before drawing. The tester confirmed continued play and working buttons with the permanent correction.

Native 1080p, borderless Alt+Tab, the larger resource bar, smooth text, English / Traditional Chinese selection, and Steam join codes remain included. The multiplayer protocol is unchanged; this release does not claim to fix command delay.

### Install or update

Download **DragonFullFix-0.3.2-1080p-public-beta.zip**, not GitHub's automatic source archive.

1. Close the game. If an earlier full-fix pack is installed, run **Restore.cmd** first, including when updating from v0.3.1.
2. Extract `DragonFullFix` inside the game folder and run **Install.cmd**. Keep `modernization/history` backups.
3. Optionally run **Language.cmd** to choose English or Traditional Chinese.
4. Launch through Steam. The game's **1024 × 768** setting selects native **1920 × 1080** missions.

### Validation and limits

The command-list regression reproduced the freeze's invalid dimensions before the fix and passes afterward. Tests cover repeated list rebuilding, partial native geometry changes, drawing/click alignment, surface locks, all five x86 world-origin replacements, protocol validation, timing statistics and provider state. The release package is tested for fresh installation, v0.3.1 restore-and-upgrade, corrupted-file rejection, exact rollback and save/settings preservation.

This remains a beta: broader public Noto-font and English UI testing is pending. The two gameplay corrections were validated on the tester's local build. Native 4K, offline LAN, reconnect and host migration are not provided. The 60 FPS setting is a presentation limit, not a guaranteed simulation rate.

The ZIP contains patch payloads, permitted runtime components and project source. Full game executables, original asset collections, proprietary SDK headers, personal saves, diagnostic dumps and logs are excluded. Original project code is MIT; third-party licenses remain separate.

Verify the ZIP against its accompanying **.zip.sha256** file. Please include reproduction steps, language and resolution when reporting a bug.

[Read the guide](https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix#readme) · [Support chon9 on Ko-fi](https://ko-fi.com/chon9workshop)
