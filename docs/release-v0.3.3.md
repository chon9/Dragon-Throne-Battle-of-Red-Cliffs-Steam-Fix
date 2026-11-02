## v0.3.3 — appointment freeze fix

Fixes the remaining freeze when pressing **「任命」(appoint)**. v0.3.2 restored previously tracked button sizes, but a button without saved geometry could still reach the native renderer with an enlarged rectangle. The renderer now checks the actual background image dimensions and separates standard button capture positions before applying the larger HUD layout.

The tester confirmed that appointment works, buttons display correctly, and the game no longer freezes in this test. The new regression fails against the previous implementation and passes with the correction.

### Download and update

Download **DragonFullFix-0.3.3-1080p-public-beta.zip**, not GitHub's automatic source archive.

1. Close the game and run **Restore.cmd** from your existing full-fix pack, including v0.3.2.
2. Extract the new `DragonFullFix` folder inside the game folder and run **Install.cmd**. Keep `modernization/history` backups.
3. Launch through Steam. Select the game's **1024 × 768** setting for native **1920 × 1080** missions. **Language.cmd** selects English or Traditional Chinese.

The package includes the earlier building redraw correction, larger resource bar and command buttons, smooth text, borderless Alt+Tab and Steam join codes. Multiplayer protocol and command timing are unchanged.

### Validation and limits

Checks cover appointment buttons without saved geometry, native capture separation, repeated list rebuilding, partial geometry updates, click alignment, panel bounds, surface locks, world-origin instructions, protocol and provider state. Package checks cover fresh installation, v0.3.2 restore-and-upgrade, damaged-file rejection, exact rollback and save/settings preservation.

This remains a beta. Broader public Noto-font and English UI validation is pending; gameplay confirmation used the tester's local font build. Command delay remains under investigation. Native 4K, offline LAN, reconnect and host migration are not provided. The 60 FPS setting is a presentation limit.

Full game executables, original asset collections, proprietary SDK headers, personal saves, logs and diagnostic captures are excluded. Project source is MIT with separate third-party licenses. Verify the download using the accompanying **.zip.sha256** file.

[Read the guide](https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix#readme) · [Support chon9 on Ko-fi](https://ko-fi.com/chon9workshop)
