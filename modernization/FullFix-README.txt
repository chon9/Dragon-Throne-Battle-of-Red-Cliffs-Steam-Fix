DRAGON THRONE - FULL FIX PACK 0.4.1 (1080p PUBLIC BETA)

CLASSIC HUD AND WORLD-GAP FIX - 0.4.1
The original 20px top bar, resource popup and original-size command buttons
return. Runtime HUD replacement, the experimental enlarged renderer HUD and
F8 remapping are not included. Native 1080p gameplay, separated panels, smooth
text, borderless mode, Steam join codes and the guided updater are retained.

Units and selection outlines no longer disappear in the open bottom middle
area when the mouse crosses a HUD panel. The user confirmed this in gameplay.
World geometry remains full-height; input checks protect the actual panels.
Long-session stability on the friend's PC remains unconfirmed. This is not a
guaranteed fix for every freeze, and multiplayer command delay is not fixed.

WHAT IS INCLUDED
Native 1920x1080 gameplay with original-style HUD controls; larger smooth text;
cnc-ddraw borderless mode with a 60 FPS limit; Steam join-code multiplayer;
resolution, language and opening-video choices; backups and recovery commands.
The public fonts are OFL-licensed Noto-derived Dragon UI Bitmap. The opening
videos are skipped only if you select On; the wizard still defaults to Off.

Project: https://github.com/chon9/Dragon-Throne-Battle-of-Red-Cliffs-Steam-Fix
Optional support: https://ko-fi.com/chon9workshop

UPDATING FROM AN EARLIER COMBINED PACK
Save and close the game. Extract this ZIP into the game folder, replacing the
old DragonFullFix package folder. Run Install.cmd or Update.cmd: both open the
same guided wizard. It automatically restores the recorded original files
before applying the new patch. Do not manually run Restore.cmd first.
Keep modernization/history and installed-full-fix.json: these are required for
safe updates and restoring originals. Tested from friend packs 0.2 / 0.3 and
public 0.3.3 in the earlier updater; v0.4.0 upgrade tested for this release.
This wizard installs the downloaded pack; it does not download it.
The new payload and original backups are verified before reversing the old pack.
A failed update attempts to restore the previous installed version. If the
operation was interrupted, run Recover.cmd, then try the wizard again.
Modified files, bad backups and separate layered mods stop the update safely.
Do not delete backups or bypass errors; keep the error message for support.

REQUIREMENTS
Each player needs their own Steam copy of Dragon Throne: Battle of Red Cliffs,
Windows, and an online Steam account. This pack targets the original Steam
executable SHA256 5f31d5ac6080a54649580ef6ce30cdb748e72919a54475a0342b8c3f8bf58444.
Different builds or modified executable/font files are refused before changes.
No Python, compiler, VPN, IPX driver, or DirectPlay component is needed to play.
Allow at least 1 GB free for staging and rollback snapshots. Update history
is retained and grows over time.

INSTALL
1. Close the game. Use the original supported Steam build or an earlier combined
   full-fix pack. Other HD mods must be restored with their own installer first.
2. In Steam, right-click the game > Manage > Browse local files.
3. Extract the ZIP so the DragonFullFix folder is INSIDE that game folder:
       ...\Dragon Throne - Battle of Red Cliffs\dragonthrone.exe
       ...\Dragon Throne - Battle of Red Cliffs\DragonFullFix\Install.cmd
   Avoid an extra nested folder. Do not manually copy files from payload.
4. Double-click DragonFullFix\Install.cmd (or Update.cmd) and choose:
   - Output: automatic desktop borderless, or a fixed borderless window at
     1920x1080 / 2560x1440 / 3840x2160. Sizes above your monitor are hidden.
   - Language: keep current, English, or Traditional Chinese.
   - Skip opening videos: Off by default (videos play), or On (straight to menu).
   Enter chooses the defaults: automatic desktop, current language, skip Off.
5. Wait for "Setup complete". Backups go under modernization\history.
   Windows may ask for admin access for an explicit language change. If denied,
   the patch stays installed; use Language.cmd later to complete that step.
6. Start from Steam (or Launch-Steam.cmd). In game settings select 1024 x 768.
   That legacy option now activates native 1920 x 1080 during missions.
   The main menu may still use its original aspect ratio. Alt+Tab is supported.

LANGUAGE
The game already includes English and Traditional Chinese resources. The pack
does not replace them or force the host's language onto the friend. Installation
keeps your current language by default. The wizard or Language.cmd lets each
player choose independently.
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
Timing diagnostics are retained; this update does not claim to fix command delay.
During a two-player session, play normally for a minute and try move, build and
train commands. DTNet.log beside dragonthrone.exe records connection and queue
timings every ten seconds, without chat, join codes or command payloads.
Keep that log for troubleshooting if commands still feel delayed.
Measured command latency, long matches and recovery after disconnect remain
unverified. Check the restored original-size commands in your next match.

RESTORE
Close the game and double-click Restore.cmd. It restores exactly the files
saved before the first combined pack, and removes DLLs that were newly added.
Opening videos disabled by the wizard are also restored.
It preserves saves and your language choice. Use Language.cmd option 3 if you
also want the prior language back. Keep modernization\history until restoration
is complete. Changed files are protected: restoration stops if another mod or
Steam update changed an installed file. Do not layer the older separate native
or multiplayer installers over this pack.

RESOLUTION AND VIDEO OPTIONS
The widescreen HUD still renders at 1920x1080. 1440p and 4K output scale that image;
they do not enable a native 4K layout. Your desktop must be at least 1920x1080.
The wizard changes renderer settings, not Windows display resolution. Scaled
output settings are tested; their in-game appearance needs broader testing.
Run the wizard again to change choices. Enter always uses the defaults.
Skip On renames DATA/Fod.avi and DATA/gpmom.avi with the suffix
.disabled-by-dragonfix. The files are preserved, not deleted. Campaign videos
are untouched. Skip Off or Restore.cmd re-enables these opening videos.

STATUS AND CONTENTS
The tested native profile is 1080p, not a validated native 4K build. 60 FPS is the configured display limit,
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
