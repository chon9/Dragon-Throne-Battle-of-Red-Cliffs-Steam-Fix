# Changelog

## 0.4.1 — classic HUD and world-gap beta

- Remove runtime replacement-HUD code from DTNet. Restore the original 20px top bar, city-resource popup and original-size command buttons. The separate renderer-HUD experiment and F8 remapping are excluded.
- Fix disappearing units and selection outlines in the open gap between the bottom panels. Keep world geometry full-height and reject clicks under actual HUD panels at the input checks instead of shrinking the shared world rectangle based on cursor position.
- The user confirmed units stay visible and drag selection works across the bottom gap and right panel.
- Retain widescreen docking, minimaps, menu/tooltips, smooth fonts, building-origin corrections, stock cnc-ddraw borderless mode, Steam join codes and the guided automatic updater.
- Add a real-DLL guard against runtime HUD hooks and execute the generated x86 hit checks at 19,220 boundary points, checking unchanged world geometry and native popup delegation.
- A friend's v0.4.0 freeze was not captured; this release removes the replacement mechanism but does not claim every freeze or multiplayer command delay is fixed. Longer sessions on the friend's PC remain to be tested.

## 0.4.0 — guided update beta

- Add Install.cmd / Update.cmd choices for borderless output size, current/English/Traditional Chinese language, and opening videos (skip defaults to Off). The HUD remains native 1080p; larger outputs are scaled.
- Automatically verify and reverse an existing combined pack before applying the new payload, preserving saves and original backups.
- Stage all outputs before mutation, restore the previous installation on failure, and provide Recover.cmd for interrupted updates.
- Reversibly skip only the two opening videos, using the game's normal missing-video path. Startup skip was confirmed locally to go straight to the menu.
- Keep the v0.3.3 gameplay runtime and network protocol. No new input-delay fix is claimed.

## 0.3.3 — appointment freeze beta

- Fix the remaining freeze when pressing「任命」(appoint). An action button without saved geometry could retain an enlarged rectangle and overrun its native background bitmap; v0.3.2 did not cover this case.
- Check native button crop sizes against their background images, and arrange standard command buttons into separate native capture positions before enlarging them.
- Add a regression for an untracked button with inherited enlarged dimensions. The tester confirmed appointment, correct buttons and no freeze with the permanent local fix.

## 0.3.2 — stability beta

- Use the viewport's actual top edge when drawing overlapping buildings, units and effects. This corrects duplicated buildings below the taller resource bar.
- Restore enlarged control geometry field by field when the native UI moves a button. Previously, rebuilding the command list could leave an 80×50 click area in the 35×35 bitmap drawing path, causing repeated access violations and a frozen picture.
- Add regression coverage for command-list rebuilding, partial native geometry changes and world-origin projection.
- Both corrections were confirmed in local gameplay: building duplication stopped, selection remained correct, and continued play with command buttons stayed responsive.

## 0.3.1 — public beta

- Publish patch sources under MIT with separate third-party notices.
- Replace privately generated JhengHei bitmap data with OFL-licensed Noto-derived Dragon UI Bitmap for public redistribution.
- Retain the confirmed 0.3 native 1080p, enlarged resource bar and corrected right-command layout.
- Include borderless cnc-ddraw, language selection, smooth text and Steam join-code multiplayer.
- Include command-latency diagnostics; command delay remains under investigation.
- Exclude proprietary SDK headers, full game binaries, original assets, personal saves and logs from the repository and source bundle.

## 0.3 — private test pack

- Unified resource bar and larger command buttons.
- Corrected header overlap, transparent capture backing and selection redraws.
- Local installation, previous-pack upgrade and byte-exact rollback checks passed.

## 0.2 — private friend test pack

- Native widescreen, menu/tooltips/minimap fixes, larger smooth text and borderless mode.
- Steam join codes and English/Traditional Chinese selector.
- Two-PC multiplayer confirmed by the user and a friend.
