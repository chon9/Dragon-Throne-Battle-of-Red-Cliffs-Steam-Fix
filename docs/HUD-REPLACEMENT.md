# Classic HUD in v0.4.1

v0.4.0 used a runtime HUD replacement that resized original control geometry.
Earlier captured freezes involved oversized rectangles reaching the native
bitmap decoder. A later freeze on a friend's PC was not captured, so that
specific cause is unconfirmed.

v0.4.1 removes the ModernUI include and installation call from DTNet and builds
with `modern_hud=False`. It retains the original 20px top bar, resource popup,
command-button sizes, widescreen docking, text improvements and networking.
The separate renderer-HUD experiment was withdrawn after local testing; it is
not shipped, and no F8 remapping remains. Historical ModernUI source/tests are
retained only for reference and are not installed by the DLL.

The real-DLL regression creates dummy legacy vtables and a DTMODERN1 marker in
its own process. Constructing DirectPlay leaves those vtables untouched. The
v0.4.0 DLL fails the same guard. This verifies removal of that mechanism, not
long-session stability on every machine.

## Open space between the bottom panels

The old widescreen patch shortened the shared world rectangle when the cursor
was above either bottom panel. Sprite updates and drag-selection drawing also
read it, making units and selection outlines disappear in the visible gap.

The correction keeps the world rectangle full-height. Two native input checks
exclude points under the left and right panels, then delegate to the original
bounds and popup checks. It neither replaces UI controls nor installs mouse
polling hooks. The original terrain draw entry is unchanged.

The exact generated x86 code was executed at 19,220 boundary points through both
input paths. Tests check unchanged world fields, panel exclusion, native popup
continuation and fallback for legacy display modes/other controls. The user
confirmed units and drag selection remain visible in the gap while crossing
the right panel. Friend long-session testing is still pending.
