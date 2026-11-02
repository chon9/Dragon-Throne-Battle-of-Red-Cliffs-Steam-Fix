## v0.4.0 — guided installer and automatic updates

Your friend can now update an older combined fix pack with **one CMD wizard**. It verifies the new package and original backups, reverses the installed patch automatically, and applies the new version while preserving saves and personal game settings.

### Download and update

Download **DragonFullFix-0.4.0-1080p-public-beta.zip**, not GitHub's automatic source archive.

1. Save and close the game.
2. Extract the ZIP inside the game folder, replacing the old `DragonFullFix` package folder. **Keep `modernization/history` and the installation record.**
3. Run **Install.cmd** or **Update.cmd**. No separate manual Restore step is needed for an existing combined full-fix pack.
4. Choose the output resolution, language and opening-video option, then wait for **Setup complete**.
5. Launch through Steam. Select the game's **1024 × 768** option for native **1920 × 1080** missions.

### Choices

- **Output:** automatic desktop borderless (default), or a borderless window at 1920×1080, 2560×1440 or 3840×2160. Only sizes that fit the primary monitor are shown. The HUD still renders at 1080p; larger output is scaled, **not native 4K**. A desktop of at least 1920×1080 is required.
- **Language:** keep current (default), English or Traditional Chinese. An explicit change may need Windows administrator approval for the game's registry setting. If that step fails, the patch stays installed and the wizard reports how to retry it with Language.cmd.
- **Skip opening videos:** **Off by default**, so videos play. Choose On to avoid buggy startup playback and go straight to the menu. The two opening videos are renamed and preserved; campaign videos are untouched. Choose Off later, or use Restore.cmd, to re-enable them.

Run the wizard again to change preferences. Enter always selects the defaults. This installs the downloaded ZIP; it is not a background download service.

### Backups and recovery

The updater stages verified outputs before changing game files. It attempts to restore the previous working installation if an operation fails. If the updater was interrupted, run **Recover.cmd**, then run the wizard again. **Restore.cmd** returns the game files to their state before the first combined pack, without removing saves or changing the selected language.

Allow at least 1 GB free; retained history grows with each update. Missing/corrupt backups, files changed by other mods, and separately installed development patches block automatic updates rather than overwriting unknown changes.

### Validation and limits

Isolated Windows PowerShell tests cover fresh installation, automatic updates from the private **0.2 and 0.3** friend packs and public **0.3.3**, exact original restoration, save/settings preservation, video On/Off changes, all output configurations, damaged-payload/backup rejection, failed-update rollback and recovery after terminating the updater. Prompt tests cover both languages with a registry stub; a live elevated language switch is not claimed as tested.

This release keeps the v0.3.3 gameplay runtime and network protocol, including the building-duplication and appointment-freeze fixes. Opening-video skipping was confirmed locally to reach the menu directly. Scaled 1440p/4K visuals, broader public-font/English UI coverage and measured command latency remain pending. Native 4K, offline LAN, reconnect and host migration are not implemented. The 60 FPS setting remains a presentation limit.

The source and release archive are scanned for secrets before publishing. Full game binaries, original asset collections, proprietary SDK headers, saves, logs and diagnostic captures are excluded. Project code is MIT with separate third-party licenses; check the accompanying **.zip.sha256** file to verify the download.

[Read the guide](https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix#readme) · [Support chon9 on Ko-fi](https://ko-fi.com/chon9workshop)
