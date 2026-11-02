DRAGON THRONE - FULL FIX PACK 0.3.3 (1080p PUBLIC BETA)

WHAT IS INCLUDED
Native 1920x1080 gameplay with corrected HUD panels, minimaps, menus and tooltips;
the fixed left HUD edge and working right command buttons; larger smooth text;
cnc-ddraw borderless mode with a 60 FPS limit; and Steam join-code multiplayer.
English / Traditional Chinese selector, backups, and Restore.cmd are included.
This version adds a larger unified resource bar with selected-city stocks,
enlarged right-side commands within the original artwork, and clean selection
changes. v0.3.2 fixes duplicated buildings near units and a freeze when the
command list is rebuilt with enlarged buttons. Both fixes were confirmed in
local gameplay, including continued play and working command buttons.
v0.3.3 also fixes the remaining freeze on appointment (Ren Ming), confirmed
in local play with correct buttons and no freeze.
The public build uses OFL-licensed Noto-derived Dragon UI Bitmap glyphs instead
of the private Windows-derived bitmap fonts. Font raster tests passed; broader
in-game visual validation of this font change is pending. See the source README
and THIRD_PARTY_NOTICES.md for licenses and test coverage.

Project: https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix
Optional support: https://ko-fi.com/chon9workshop

UPDATING FROM AN EARLIER PACK (INCLUDING PUBLIC v0.3.2)
Close the game. Run Restore.cmd from your existing DragonFullFix folder first,
then extract this ZIP into the game folder and run Install.cmd. Keep the
modernization/history folder: it contains the originals needed for restoration.
The new Restore.cmd can also restore an unmodified older full-pack installation if you
already extracted over the old package. Saves and language choice are preserved.
Do not copy payload files manually or install over an active older patch.

REQUIREMENTS
Each player needs their own Steam copy of Dragon Throne: Battle of Red Cliffs,
Windows, and an online Steam account. This pack targets the original Steam
executable SHA256 5f31d5ac6080a54649580ef6ce30cdb748e72919a54475a0342b8c3f8bf58444.
Different builds or modified executable/font files are refused before changes.
No Python, compiler, VPN, IPX driver, or DirectPlay component is needed to play.
Allow about 250 MB free for patch staging and backups.

INSTALL
1. Close the game. Start with the original Steam build, without another HD mod.
2. In Steam, right-click the game > Manage > Browse local files.
3. Extract the ZIP so the DragonFullFix folder is INSIDE that game folder:
       ...\Dragon Throne - Battle of Red Cliffs\dragonthrone.exe
       ...\Dragon Throne - Battle of Red Cliffs\DragonFullFix\Install.cmd
   Avoid an extra nested folder. Do not manually copy files from payload.
4. Double-click DragonFullFix\Install.cmd and wait for "All fixes installed".
   It saves your previous files under modernization\history\full-fix-... .
5. Optional: run Language.cmd and choose English or Traditional Chinese.
6. Start from Steam (or Launch-Steam.cmd). In game settings select 1024 x 768.
   That legacy option now activates native 1920 x 1080 during missions.
   The main menu may still use its original aspect ratio. Alt+Tab is supported.

LANGUAGE
The game already includes English and Traditional Chinese resources. The pack
does not replace them or force the host's language onto the friend. Installation
keeps your current language. Language.cmd lets each player choose independently.
Close the game before switching. Windows asks for administrator access because
the original game reads its language from the 32-bit machine registry. Only the
game's Language value is changed; the first previous value is saved. Option 3
restores it. The switcher does not change Steam account/client language.
If Steam later resets the selection, select the matching language in the game's
Steam Properties > General > Language. If Windows elevation is declined, use
that Steam setting instead. This selects existing text resources, not a new
voice translation. Smooth fonts include Latin and Traditional Chinese glyphs.
English resource selection is checked; a full English UI/match remains to be
visually tested, including long English tooltip lines.

PLAY TOGETHER
Both players should use this package for matching UI and diagnostics. The
network protocol is unchanged and remains compatible with the previous pack.

Host: Multiplayer > Steam - Join codes. In the small dialog select Host a game,
then Create game in the normal multiplayer screen. Copy the displayed code,
send it privately to your friend, click Continue, and stay in the lobby.
Do not use the old session password field. Anyone with the code can try joining.

Friend: Multiplayer > Steam - Join codes. Paste the code into the small dialog
and select Join. Select the resulting session in the game's list and join it.
Use the join code, not the old IP address shown by the game.

Steam networking supports relay routing without manual router forwarding.
Steam must be online even when both PCs are on the same LAN. Offline LAN is
not implemented. The host must remain connected; migration and automatic
reconnection are not implemented.

MULTIPLAYER STATUS AND DELAY CHECK
The host and friend confirmed multiplayer works with the previous adapter.
This update adds timing diagnostics; it does not claim to fix command delay.
During a two-player session, play normally for a minute and try move, build and
train commands. DTNet.log beside dragonthrone.exe records connection and queue
timings every ten seconds, without chat, join codes or command payloads.
Keep that log for troubleshooting if commands still feel delayed.
Measured command latency, long matches and recovery after disconnect remain
unverified. The new larger commands should also be checked in your next match.

RESTORE
Close the game and double-click Restore.cmd. It restores exactly the files
saved before installing this pack, and removes DLLs that were newly added.
It preserves saves and your language choice. Use Language.cmd option 3 if you
also want the prior language back. Keep modernization\history until restoration
is complete. Changed files are protected: restoration stops if another mod or
Steam update changed an installed file. Do not layer the older separate native
or multiplayer installers over this pack.

STATUS AND CONTENTS
The tested profile is 1080p, not a validated native 4K build. On another display
the renderer may scale the 1080p image. 60 FPS is the configured display limit,
not a guarantee of 60 unique simulation frames under all workloads.
This is an experimental combined fix, not a promise to fix every original bug.
Save games, personal settings/progress, logs and full game files are not bundled.
The executable and font changes are compressed patches against your own files.
The existing Steam audio files are preserved.

Runtime: cnc-ddraw v7.1.0.0 (MIT, license/provenance included), custom DTNet.dll
(source included), and Valve-signed x86 steam_api.dll. SDK/header provenance
and their notices are in THIRD_PARTY_NOTICES.md and licenses/. Proprietary SDK
headers are not distributed; developers must supply them separately. Project
source is included for inspection and future modification. Source is not needed
to play. Install.cmd uses Windows PowerShell only for this run; it does not
change the machine's execution policy or add firewall rules/services/drivers.
