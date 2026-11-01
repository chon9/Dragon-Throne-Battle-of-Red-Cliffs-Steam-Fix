# Changelog

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
