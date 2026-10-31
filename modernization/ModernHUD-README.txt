MODERN HUD PROTOTYPE - development notes

This opt-in prototype layers over the confirmed working 1080p/smooth/Steam build.
Build-ModernHUD.py stages the marked executable and DTNet.dll. Install-ModernHUD.ps1
backs up both files, installs only after the game closes, and supports -Action restore.
Restore this layer before using the earlier native/network/full-fix installers.
The 0.2 friend ZIP remains the earlier tested build; it has not been overwritten.

Presentation
- 68-pixel top area; world render and input origin move together.
- Larger text, selected territory's city stocks, and existing player gold value.
- Stock is shown only for the local player's cities, as in the original UI.
- Six stock icons match the original resource order (grain, meat, wood, iron,
  prepared food, wine); no economy or resource values are changed.
- Right commands retain native handlers, enabled states and hover imagery.
  The panel reflows into up to three columns; native 35px icons become 48px,
  with 80x50 click rectangles. Original geometry is restored before each native
  paint, then the new paint and hit rectangles are applied together.
- Revision 2 replaces the duplicate old top line, loads independent resource
  icons without requiring the drop-down, reuses original panel artwork, and
  removes the old action-panel wipe animation.
- Revision 3 moves all three command rows below the original 44px header,
  keeps them above the bottom frame, and clears the native capture backing
  before each paint so transparent pixels cannot retain old enlarged icons.
  Selection-change tests cover eight, two, seven and zero visible commands,
  transparent rims, unchanged header pixels and removal of stale buttons.
- Explicit farm confirm/cancel text labels still need verified control IDs.
  Do not guess label meaning from icon color or control order.

Networking diagnostics
The wire protocol and command scheduling are unchanged. The user confirmed that
the previous adapter works between two PCs, but reports delays for move/build/train.
The new adapter logs timing summaries to DTNet.log every 10 seconds while a session
has at least two players. It records no command payloads, chat, codes or Steam IDs.
Disable the measurements with DTNet.ini beside the game executable:
    [Diagnostics]
    Latency=0

Latency local:
  poll_ms: interval between game calls that service the adapter (mean/max).
  steam_inbox_ms: locally received Steam messages waiting for the adapter to poll.
  game_inbox_ms: messages waiting in the adapter until the game consumes them.
  sends/delivered: local adapter call counts, not necessarily individual orders.
Latency player:
  ping_ms: Steam's round-trip estimate; send_queue_ms: estimated outgoing wait.
  pending_reliable/unacked: bytes pending transmission or acknowledgment.
  quality_local/remote: Steam connection quality estimates.
Lobby idling, alt-tab and pauses can raise maxima; collect while actively playing.
These are not click-to-action measurements and do not prove a delay reduction.
If network/poll/queue timings are small but commands still lag, investigate the
original game's synchronized command tick/look-ahead next. Do not alter simulation
timing independently on one peer, or drop reliable game orders to chase latency.

Checks
Protocol, isolated provider, timing statistics and action geometry tests pass.
The geometry test covers repeated reflow, paint/hit alignment, bounds, balanced
surface locks, original-artwork composition calls, and wipe-counter reset.
Installer and byte-exact rollback passed in a separate fixture.
Live first revision: larger row and action tiles render; user reported duplicate
top line, icons absent before opening the drop-down, rough switches and mismatched
panel style. Revision 2 addressed these. Revision 3 corrected the command rows
overlapping the header. User confirmed "Panel looks correct now" after testing.
Native 4K, explicit farm labels, English layout and measured two-PC latency remain
unfinished. Preserve the prior working build and do not label this prototype stable.
