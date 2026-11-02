param(
    [string]$GameDirectory=(Split-Path -Parent $PSScriptRoot),
    [switch]$NonInteractive,
    [ValidateSet('Desktop','1920x1080','2560x1440','3840x2160')][string]$Resolution='Desktop',
    [ValidateSet('Keep','English','TraditionalChinese')][string]$Language='Keep',
    [ValidateSet('Off','On')][string]$SkipVideos='Off'
)
$ErrorActionPreference='Stop'
function Choose($prompt,$choices,$default) {
    do {
        $answer=Read-Host $prompt
        if (!$answer) { return $default }
        if ($choices.ContainsKey($answer)) { return $choices[$answer] }
        Write-Host 'Please enter one of the listed numbers.'
    } while ($true)
}
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if (!(Test-Path -LiteralPath (Join-Path $game 'dragonthrone.exe'))) { throw 'Extract DragonFullFix beside dragonthrone.exe, then run Install.cmd.' }
Write-Host 'Dragon Throne - install / automatic update'
Write-Host 'Close the game first. Saves and progress are preserved.'
if (Test-Path -LiteralPath (Join-Path $game 'modernization/installed-full-fix.json')) {
    $old=Get-Content -LiteralPath (Join-Path $game 'modernization/installed-full-fix.json') -Raw | ConvertFrom-Json
    Write-Host "Detected installed pack: $($old.version). Its original backups will be restored automatically before applying this pack."
}
if (!$NonInteractive) {
    Write-Host ''
    Write-Host 'OUTPUT RESOLUTION (the game and modern HUD render at 1920 x 1080)'
    Write-Host '1. Automatic desktop size, borderless fullscreen [default]'
    $choices=@{'1'='Desktop'}
    $bounds=$null
    try {
        Add-Type -TypeDefinition 'using System.Runtime.InteropServices; public static class DragonSetupDpi { [DllImport("user32.dll")] public static extern bool SetProcessDPIAware(); }'
        [DragonSetupDpi]::SetProcessDPIAware() | Out-Null
        Add-Type -AssemblyName System.Windows.Forms; $bounds=[Windows.Forms.Screen]::PrimaryScreen.Bounds
    } catch { }
    if ($bounds -and ($bounds.Width -lt 1920 -or $bounds.Height -lt 1080)) { throw 'This modern HUD pack needs a desktop of at least 1920 x 1080. Smaller native layouts are not supported yet. No changes made.' }
    $n=2
    foreach ($size in @('1920x1080','2560x1440','3840x2160')) {
        $w,$h=$size.Split('x')
        if (!$bounds -or ([int]$w -le $bounds.Width -and [int]$h -le $bounds.Height)) {
            Write-Host "$n. $size borderless window (scaled output)"
            $choices[[string]$n]=$size; $n++
        }
    }
    Write-Host 'Automatic fits your desktop, including 1440p/4K displays. Fixed sizes above your current monitor are hidden.'
    $Resolution=Choose 'Choose output [1]' $choices 'Desktop'
    Write-Host ''
    Write-Host 'LANGUAGE'
    Write-Host '1. Keep current language [default]'
    Write-Host '2. English'
    Write-Host '3. Traditional Chinese'
    $Language=Choose 'Choose language [1]' @{'1'='Keep';'2'='English';'3'='TraditionalChinese'} 'Keep'
    Write-Host ''
    Write-Host 'SKIP OPENING VIDEOS'
    Write-Host '1. Off - play the opening videos [default]'
    Write-Host '2. On - go straight to the menu (avoids buggy opening playback)'
    $SkipVideos=Choose 'Choose video option [1]' @{'1'='Off';'2'='On'} 'Off'
}
# Check language assets and registry availability before changing the game files.
if ($Language -ne 'Keep') {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Language.ps1') -Language $Language -GameDirectory $game -CheckOnly
    if ($LASTEXITCODE -ne 0) { throw 'Language preflight failed. No patch files were changed.' }
}
Write-Host "Installing with output $Resolution, language $Language, skip videos $SkipVideos..."
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Install.ps1') -Action update -GameDirectory $game -Resolution $Resolution -SkipVideos $SkipVideos
if ($LASTEXITCODE -ne 0) { throw 'Installation did not complete. Read the error above; keep the history folder.' }
if ($Language -ne 'Keep') {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Language.ps1') -Language $Language -GameDirectory $game -NoPause
    if ($LASTEXITCODE -ne 0) {
        throw 'The patch is installed, but the language switch did not complete. Run Language.cmd to retry, or use Steam game Properties > Language.'
    }
}
Write-Host 'Setup complete. Launch with Steam or Launch-Steam.cmd.'
Write-Host 'You can run Install.cmd again to change these preferences. Restore.cmd removes the patch.'
