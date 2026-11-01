param(
    [ValidateSet('install','restore')][string]$Action='install',
    [string]$GameDirectory=(Split-Path -Parent $PSScriptRoot)
)
$ErrorActionPreference='Stop'
$env:PSModulePath=(Join-Path $PSHOME 'Modules')+';'+$env:PSModulePath
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$modern=Join-Path $game 'modernization'
$statePath=Join-Path $modern 'installed-full-fix.json'
function Hash($p) { (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant() }
function Under($root,$relative) {
    $prefix=[IO.Path]::GetFullPath($root).TrimEnd('\')+'\'
    $path=[IO.Path]::GetFullPath((Join-Path $root $relative))
    if (!$path.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe file path.' }
    return $path
}
function Put($source,$destination) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}
if (!(Test-Path -LiteralPath (Join-Path $game 'dragonthrone.exe'))) { throw 'Extract DragonFullFix inside the game folder, beside dragonthrone.exe.' }
foreach ($process in @(Get-Process dragonthrone -ErrorAction SilentlyContinue)) {
    if (!$process.Path -or $process.Path -eq (Join-Path $game 'dragonthrone.exe')) { throw 'Save and close the game first.' }
}
if ($Action -eq 'restore') {
    if (!(Test-Path -LiteralPath $statePath)) { throw 'No full-fix installation is recorded here.' }
    $state=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    $backup=Under $modern $state.backup
    foreach ($entry in $state.files) {
        if ((Hash (Under $game $entry.name)) -ne $entry.after) { throw "Changed file: $($entry.name). Restore stopped to preserve your changes." }
        if ($entry.before -and (Hash (Under $backup $entry.name)) -ne $entry.before) { throw 'Backup verification failed.' }
    }
    foreach ($entry in $state.files) {
        $target=Under $game $entry.name
        if ($entry.before) { Put (Under $backup $entry.name) $target }
        else { Remove-Item -LiteralPath $target }
    }
    Remove-Item -LiteralPath $statePath
    Write-Output 'Restored all files to their state before this package was installed. Saves and language choice are preserved.'
    return
}
if (Test-Path -LiteralPath $statePath) { throw 'This package is already installed. Use Restore.cmd before reinstalling.' }
foreach ($name in @('installed-native.json','installed-network.json','installed-modern-hud.json','installed-world-origin.json','installed-hud-hang-fix.json')) {
    if (Test-Path -LiteralPath (Join-Path $modern $name)) { throw 'An earlier separate patch is installed. Restore it with its original installer before using this combined package.' }
}
$manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
foreach ($entry in $manifest.files) {
    $target=Under $game $entry.name
    $payload=Under $PSScriptRoot $entry.payload
    if ((Hash $payload) -ne $entry.payloadSha256) { throw "Package verification failed: $($entry.name)" }
    if ($entry.kind -eq 'delta' -and (Hash $target) -ne $entry.before) { throw "Unsupported or modified game file: $($entry.name). This package needs the original supported Steam build. No changes made." }
}
Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.IO.Compression;
public static class DragonFullPatch {
 public static void Apply(string original,string patch,string output) {
  using(var file=File.OpenRead(patch))
  using(var zip=new GZipStream(file,CompressionMode.Decompress))
  using(var r=new BinaryReader(zip)) {
   if(r.ReadUInt32()!=0x31504644) throw new InvalidDataException("Patch header");
   int length=r.ReadInt32(), count=r.ReadInt32();
   if(length<1 || length>128*1024*1024 || count<0 || count>1000000) throw new InvalidDataException("Patch bounds");
   byte[] bytes=File.ReadAllBytes(original); Array.Resize(ref bytes,length);
   int end=0;
   for(int i=0;i<count;i++) {
    int offset=r.ReadInt32(), size=r.ReadInt32();
    if(offset<end || size<1 || offset>length || size>length-offset) throw new InvalidDataException("Patch range");
    int read=0;
    while(read<size) { int n=r.Read(bytes,offset+read,size-read); if(n==0) throw new EndOfStreamException(); read+=n; }
    end=offset+size;
   }
   if(r.Read()!=-1) throw new InvalidDataException("Trailing patch bytes");
   File.WriteAllBytes(output,bytes);
  }
 }
}
'@
$relative='history\full-fix-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$backup=Under $modern $relative
$stage=Join-Path $backup '_stage'
New-Item -ItemType Directory -Path $stage -Force | Out-Null
$records=@()
# Prepare and verify every output before changing any game files.
foreach ($entry in $manifest.files) {
    $target=Under $game $entry.name
    $output=Under $stage $entry.name
    $payload=Under $PSScriptRoot $entry.payload
    New-Item -ItemType Directory -Path (Split-Path -Parent $output) -Force | Out-Null
    if ($entry.kind -eq 'delta') { [DragonFullPatch]::Apply($target,$payload,$output) }
    else { Put $payload $output }
    if ((Hash $output) -ne $entry.after) { throw "Output verification failed: $($entry.name). Game files unchanged." }
    $before=$null
    if (Test-Path -LiteralPath $target) {
        $before=Hash $target
        Put $target (Under $backup $entry.name)
        if ((Hash (Under $backup $entry.name)) -ne $before) { throw 'Backup verification failed.' }
    }
    $records+= [pscustomobject]@{name=$entry.name;before=$before;after=$entry.after}
}
try {
    foreach ($entry in $records) {
        Put (Under $stage $entry.name) (Under $game $entry.name)
        if ((Hash (Under $game $entry.name)) -ne $entry.after) { throw 'Installed file verification failed.' }
    }
    [pscustomobject]@{version=$manifest.version;backup=$relative;files=$records} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $statePath -Encoding UTF8
} catch {
    foreach ($entry in $records) {
        $target=Under $game $entry.name
        if ($entry.before) { Put (Under $backup $entry.name) $target }
        elseif (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target }
    }
    throw
}
# Only remove the known staging files, retaining all original backups.
foreach ($entry in $records) { Remove-Item -LiteralPath (Under $stage $entry.name) }
Write-Output 'All fixes installed: 1080p HUD, smooth text, borderless 60 FPS profile, and Steam join codes.'
Write-Output 'Launch through Steam. Select the in-game 1024 x 768 option to activate native 1920 x 1080.'
Write-Output 'Use Language.cmd to select English or Traditional Chinese. See README.txt for the friend test.'
