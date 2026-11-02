param(
    [ValidateSet('English','TraditionalChinese','Restore')][string]$Language,
    [string]$GameDirectory=(Split-Path -Parent $PSScriptRoot),
    [switch]$Elevated,
    [switch]$CheckOnly,
    [switch]$NoPause
)
$ErrorActionPreference='Stop'
$env:PSModulePath=(Join-Path $PSHOME 'Modules')+';'+$env:PSModulePath
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if (!(Test-Path -LiteralPath (Join-Path $game 'dragonthrone.exe'))) { throw 'Extract DragonFullFix inside your game folder first.' }
if (!$Language) {
    Write-Host '1. English'
    Write-Host '2. Traditional Chinese'
    Write-Host '3. Restore language from before the first switch'
    switch (Read-Host 'Choose 1, 2, or 3') {
        '1' { $Language='English' }
        '2' { $Language='TraditionalChinese' }
        '3' { $Language='Restore' }
        default { throw 'No language selected. Nothing changed.' }
    }
}
$resource=if ($Language -eq 'English') { 'english' } else { 'chinesetaiwan' }
if ($Language -ne 'Restore') {
    foreach ($ext in @('tib','txd','txs')) {
        if (!(Test-Path -LiteralPath (Join-Path $game "DATA\$resource.$ext"))) { throw "Missing $resource.$ext. Install this language using Steam first." }
    }
}
$keyPath='SOFTWARE\Object Software (Beijing) Co., Ltd.\Fate of the Dragon Expansion\1.00.000'
$registry=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,[Microsoft.Win32.RegistryView]::Registry32)
try {
    $key=$registry.OpenSubKey($keyPath,$false)
    if (!$key) { throw 'Steam has not set up this game yet. Launch it once from Steam, close it, and retry.' }
    try { $current=$key.GetValue('Language',$null); $kind=if ($null -ne $current) { $key.GetValueKind('Language').ToString() } else { $null } } finally { $key.Dispose() }
} finally { $registry.Dispose() }
if ($CheckOnly) { Write-Output "Current language: $current. $Language resources found. No changes made."; return }
foreach ($process in @(Get-Process dragonthrone -ErrorAction SilentlyContinue)) {
    if (!$process.Path -or $process.Path -eq (Join-Path $game 'dragonthrone.exe')) { throw 'Save and close the game before switching language.' }
}
$principal=New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    if ($Elevated) { throw 'Administrator access was not granted.' }
    Write-Host 'Windows will request administrator access to change this legacy game language setting.'
    $arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$PSCommandPath+'" -Language '+$Language+' -GameDirectory "'+$game+'" -Elevated'
    if ($NoPause) { $arguments+=' -NoPause' }
    $child=Start-Process powershell.exe -ArgumentList $arguments -Verb RunAs -Wait -PassThru -WindowStyle Hidden
    if ($child.ExitCode -ne 0) { throw 'Language switch did not complete.' }
    return
}
$statePath=Join-Path $game 'modernization\full-fix-language-backup.json'
if ($Language -eq 'Restore') {
    if (!(Test-Path -LiteralPath $statePath)) { throw 'No previous language was saved by this switcher.' }
    $saved=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    $value=$saved.value
    $valueKind=$saved.kind
} else {
    if (!(Test-Path -LiteralPath $statePath)) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $statePath) -Force | Out-Null
        [pscustomobject]@{value=$current;kind=$kind} | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding UTF8
    }
    $value=if ($Language -eq 'English') { 'ENGLISH' } else { 'CHINESETAIWAN' }
    $valueKind='String'
}
$registry=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,[Microsoft.Win32.RegistryView]::Registry32)
try {
    $key=$registry.OpenSubKey($keyPath,$true)
    try {
        if ($null -eq $value) { $key.DeleteValue('Language',$false) }
        else { $key.SetValue('Language',$value,[Microsoft.Win32.RegistryValueKind]$valueKind) }
        if ($key.GetValue('Language',$null) -ne $value) { throw 'Language read-back verification failed.' }
    } finally { $key.Dispose() }
} finally { $registry.Dispose() }
if ($Language -eq 'Restore') { Remove-Item -LiteralPath $statePath }
Write-Output "Game text language set to: $value. Launch from Steam."
Write-Output 'If Steam resets it later, choose the same language under Steam game Properties > General > Language.'
# The parent CMD owns the interactive console and pause; the elevated helper stays hidden.
