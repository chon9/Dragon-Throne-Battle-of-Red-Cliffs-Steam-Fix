param(
    [ValidateSet('install','update','restore','recover')][string]$Action='install',
    [string]$GameDirectory=(Split-Path -Parent $PSScriptRoot),
    [ValidateSet('Desktop','1920x1080','2560x1440','3840x2160')][string]$Resolution='Desktop',
    [ValidateSet('Off','On')][string]$SkipVideos='Off'
)
$ErrorActionPreference='Stop'
$env:PSModulePath=(Join-Path $PSHOME 'Modules')+';'+$env:PSModulePath
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$modern=Join-Path $game 'modernization'
$stateName='modernization/installed-full-fix.json'
$statePath=Join-Path $game $stateName
$journalPath=Join-Path $modern 'full-fix-transaction.json'
$allowed=@('dragonthrone.exe','DATA/16Point.fnt','DATA/20Point.fnt','DATA/24Point.fnt','ddraw.dll','ddraw.ini','DTNet.dll','steam_api.dll')
$videoNames=@('DATA/Fod.avi','DATA/gpmom.avi')
$suffix='.disabled-by-dragonfix'
function Hash($p) { if (Test-Path -LiteralPath $p -PathType Leaf) { (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant() } }
function Under($root,$relative) {
    if ([IO.Path]::IsPathRooted($relative)) { throw 'Unsafe absolute path.' }
    $prefix=[IO.Path]::GetFullPath($root).TrimEnd('\')+'\'
    $path=[IO.Path]::GetFullPath((Join-Path $root $relative))
    if (!$path.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe file path.' }
    # Do not let a junction or symlink redirect an installer write outside this folder.
    $walk=$path
    while ($walk -and $walk.Length -ge $prefix.Length) {
        if ((Test-Path -LiteralPath $walk) -and ((Get-Item -LiteralPath $walk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Linked installer paths are not supported.' }
        $walk=Split-Path -Parent $walk
    }
    return $path
}
function Put($source,$destination) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}
function Set-File($source,$destination) {
    if ($source) { Put $source $destination }
    elseif (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination }
}
function Check-Entries($entries,$names) {
    $seen=@{}
    foreach ($e in $entries) {
        if ($e.name -notin $names -or $seen.ContainsKey($e.name)) { throw 'Unexpected or duplicate file record.' }
        $seen[$e.name]=$true
        foreach ($h in @($e.before,$e.after,$e.sha256)) {
            if ($h -and $h -notmatch '^[a-fA-F0-9]{64}$') { throw 'Invalid file hash.' }
        }
    }
}
function Backup-Path($relative) {
    if ($relative -notmatch '^history[\\/]full-fix-[a-zA-Z0-9_-]+([\\/]_undo)?$') { throw 'Unsafe backup location.' }
    Under $modern $relative
}
function Undo-Transaction($j,[bool]$checkCurrent) {
    $undo=Backup-Path $j.backup
    Check-Entries $j.files ($allowed+@($stateName))
    foreach ($e in $j.files) {
        if ($e.before -and (Hash (Under $undo $e.name)) -ne $e.before) { throw 'Recovery backup verification failed.' }
        $current=Hash (Under $game $e.name)
        if ($checkCurrent -and $current -ne $e.before -and $current -ne $e.after -and $current -ne $e.original) { throw "Changed file blocks recovery: $($e.name). Keep the backups for manual recovery." }
    }
    $seen=@{}
    foreach ($m in $j.moves) {
        if ($m.name -notin $videoNames -or $seen.ContainsKey($m.name) -or $m.direction -notin @('disable','enable') -or $m.sha256 -notmatch '^[a-fA-F0-9]{64}$') { throw 'Invalid video recovery record.' }
        $seen[$m.name]=$true
        $from=Under $game $m.name
        $to=Under $game ($m.name+$suffix)
        if ($m.direction -eq 'enable') { $from,$to=$to,$from }
        if ((Test-Path -LiteralPath $from) -and (Test-Path -LiteralPath $to)) { throw 'Both video filenames exist. Recovery stopped to preserve them.' }
        if ((Hash $from) -ne $m.sha256 -and (Hash $to) -ne $m.sha256) { throw 'Video recovery verification failed.' }
    }
    foreach ($m in $j.moves) {
        $from=Under $game $m.name; $to=Under $game ($m.name+$suffix)
        if ($m.direction -eq 'enable') { $from,$to=$to,$from }
        if (Test-Path -LiteralPath $to) { Rename-Item -LiteralPath $to -NewName (Split-Path -Leaf $from) }
    }
    foreach ($e in $j.files) {
        $source=if ($e.before) { Under $undo $e.name } else { $null }
        Set-File $source (Under $game $e.name)
        if ((Hash (Under $game $e.name)) -ne $e.before) { throw 'Recovery read-back verification failed.' }
    }
    Remove-Item -LiteralPath $journalPath
}
if (!(Test-Path -LiteralPath (Join-Path $game 'dragonthrone.exe'))) { throw 'Extract DragonFullFix inside the game folder, beside dragonthrone.exe.' }
foreach ($process in @(Get-Process dragonthrone -ErrorAction SilentlyContinue)) {
    if (!$process.Path -or $process.Path -eq (Join-Path $game 'dragonthrone.exe')) { throw 'Save and close the game first.' }
}
if ($Action -eq 'recover') {
    if (!(Test-Path -LiteralPath $journalPath)) { throw 'No interrupted update is recorded.' }
    Undo-Transaction (Get-Content -LiteralPath $journalPath -Raw | ConvertFrom-Json) $true
    Write-Output 'Recovered the installation as it was before the interrupted operation. Run Install.cmd again.'
    return
}
if (Test-Path -LiteralPath $journalPath) { throw 'An interrupted operation is recorded. Run Recover.cmd before installing or restoring.' }
$old=$null; $oldFiles=@{}; $oldBackup=$null
if (Test-Path -LiteralPath $statePath) {
    if ($Action -eq 'install') { throw 'This package is already installed. Use Install.cmd for the guided automatic update.' }
    $old=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    Check-Entries $old.files $allowed
    if (@($old.files).Count -eq 0) { throw 'Empty installation record.' }
    $oldBackup=Backup-Path $old.backup
    foreach ($e in $old.files) {
        if ((Hash (Under $game $e.name)) -ne $e.after) { throw "Changed file: $($e.name). Stopped to preserve your changes." }
        if ($e.before -and (Hash (Under $oldBackup $e.name)) -ne $e.before) { throw 'Backup verification failed.' }
        $oldFiles[$e.name]=$e
    }
} elseif ($Action -eq 'restore') { throw 'No full-fix installation is recorded here.' }
foreach ($name in @('installed-native.json','installed-network.json','installed-modern-hud.json','installed-world-origin.json','installed-hud-hang-fix.json','installed-appointment-fix.json','installed-startup-videos.json','installed-renderer-hud.json','installed-classic-hud.json')) {
    if (Test-Path -LiteralPath (Join-Path $modern $name)) { throw 'An earlier separate patch is installed. Restore it with its original installer before using this combined package.' }
}
function Baseline($name) {
    if ($oldFiles.ContainsKey($name)) {
        if ($oldFiles[$name].before) { return (Under $oldBackup $name) }
        return $null
    }
    $p=Under $game $name
    if (Test-Path -LiteralPath $p -PathType Leaf) { return $p }
    return $null
}
# Plan video renames, verifying ownership before touching either filename.
$oldVideos=@{}
if ($old -and $old.videos) {
    Check-Entries $old.videos $videoNames
    foreach ($v in $old.videos) { $oldVideos[$v.name]=$v.sha256 }
}
$moves=@(); $videos=@()
foreach ($name in $videoNames) {
    $source=Under $game $name; $disabled=Under $game ($name+$suffix)
    $wasSkipped=$oldVideos.ContainsKey($name)
    if ($wasSkipped) {
        if ((Test-Path -LiteralPath $source) -or (Hash $disabled) -ne $oldVideos[$name]) { throw 'Changed opening video: restore stopped to preserve your files.' }
    } elseif (Test-Path -LiteralPath $disabled) { throw 'Unrecorded disabled video found. Restore it with its original video switcher first.' }
    $wantSkip=$Action -ne 'restore' -and $SkipVideos -eq 'On'
    if ($wantSkip) {
        $h=if ($wasSkipped) { $oldVideos[$name] } else { Hash $source }
        if (!$h) { throw "Cannot skip missing opening video: $name" }
        $videos+=[pscustomobject]@{name=$name;sha256=$h}
        if (!$wasSkipped) { $moves+=[pscustomobject]@{name=$name;sha256=$h;direction='disable'} }
    } elseif ($wasSkipped) { $moves+=[pscustomobject]@{name=$name;sha256=$oldVideos[$name];direction='enable'} }
}
$manifest=$null
if ($Action -ne 'restore') {
    $manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
    Check-Entries $manifest.files $allowed
    if (@($manifest.files).Count -ne $allowed.Count) { throw 'Incomplete package manifest.' }
    foreach ($e in $manifest.files) {
        if ($e.kind -notin @('delta','copy')) { throw 'Unknown payload format.' }
        if (!$e.after -or !$e.payloadSha256 -or (Hash (Under $PSScriptRoot $e.payload)) -ne $e.payloadSha256) { throw "Package verification failed: $($e.name)" }
        if ($e.kind -eq 'delta') {
            $base=Baseline $e.name
            if (!$base -or !$e.before -or (Hash $base) -ne $e.before) { throw "Unsupported or modified game file: $($e.name). Original supported Steam files are required. No changes made." }
        }
    }
}
if (-not ('DragonFullPatch' -as [type])) {
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
}
$relative='history/full-fix-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+([guid]::NewGuid().ToString('N').Substring(0,8))
$backup=Backup-Path $relative
$stage=Join-Path $backup '_stage'
$undo=Join-Path $backup '_undo'
New-Item -ItemType Directory -Path $stage -Force | Out-Null
$records=@(); $writes=@(); $allNames=@($allowed | Where-Object { $Action -ne 'restore' -or $oldFiles.ContainsKey($_) })
foreach ($name in $allNames) {
    $base=Baseline $name; $before=$null
    if ($base) {
        $before=Hash $base
        Put $base (Under $backup $name)
        if ((Hash (Under $backup $name)) -ne $before) { throw 'Original backup verification failed.' }
    }
    $output=$null; $after=$before
    if ($Action -ne 'restore') {
        $e=$manifest.files | Where-Object { $_.name -eq $name }
        $output=Under $stage $name
        New-Item -ItemType Directory -Path (Split-Path -Parent $output) -Force | Out-Null
        if ($e.kind -eq 'delta') { [DragonFullPatch]::Apply($base,(Under $PSScriptRoot $e.payload),$output) }
        else { Put (Under $PSScriptRoot $e.payload) $output }
        if ((Hash $output) -ne $e.after) { throw "Output verification failed: $name. Game files unchanged." }
        if ($name -eq 'ddraw.ini' -and $Resolution -ne 'Desktop') {
            $width,$height=$Resolution.Split('x')
            $ini=Get-Content -LiteralPath $output -Raw
            $ini=$ini -replace '(?m)^width=0\r?$',("width="+$width) -replace '(?m)^height=0\r?$',("height="+$height) -replace '(?m)^fullscreen=true\r?$','fullscreen=false'
            [IO.File]::WriteAllText($output,$ini,[Text.Encoding]::ASCII)
        }
        $after=Hash $output
        $records+=[pscustomobject]@{name=$name;before=$before;after=$after}
    } elseif ($before) { $output=Under $backup $name }
    $writes+=[pscustomobject]@{name=$name;source=$output;original=$before;after=$after}
}
$newState=$null
if ($Action -ne 'restore') {
    $newState=Under $stage $stateName
    New-Item -ItemType Directory -Path (Split-Path -Parent $newState) -Force | Out-Null
    [pscustomobject]@{version=$manifest.version;backup=$relative;files=$records;videos=$videos;options=@{resolution=$Resolution;skipVideos=$SkipVideos}} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $newState -Encoding UTF8
}
$writes+=[pscustomobject]@{name=$stateName;source=$newState;original=$null;after=$(if ($newState) { Hash $newState } else { $null })}
$snapshots=@()
foreach ($w in $writes) {
    $target=Under $game $w.name; $current=Hash $target
    if ($current) {
        Put $target (Under $undo $w.name)
        if ((Hash (Under $undo $w.name)) -ne $current) { throw 'Rollback snapshot verification failed.' }
    }
    $snapshots+=[pscustomobject]@{name=$w.name;before=$current;original=$w.original;after=$w.after}
}
$journal=[pscustomobject]@{version=1;backup=($relative+'/_undo');files=$snapshots;moves=$moves}
$journal | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath ($journalPath+'.tmp') -Encoding UTF8
Move-Item -LiteralPath ($journalPath+'.tmp') -Destination $journalPath
try {
    if ($old -and $Action -ne 'restore') {
        Write-Output "Restoring the recorded originals from $($old.version)..."
        foreach ($e in $old.files) {
            $source=if ($e.before) { Under $oldBackup $e.name } else { $null }
            Set-File $source (Under $game $e.name)
            if ((Hash (Under $game $e.name)) -ne $e.before) { throw 'Old patch reversal failed.' }
        }
    }
    Write-Output 'Applying verified files and preferences...'
    foreach ($w in $writes | Where-Object { $_.name -ne $stateName }) {
        Set-File $w.source (Under $game $w.name)
        if ((Hash (Under $game $w.name)) -ne $w.after) { throw 'Installed file verification failed.' }
    }
    foreach ($m in $moves) {
        $from=Under $game $m.name; $to=Under $game ($m.name+$suffix)
        if ($m.direction -eq 'enable') { $from,$to=$to,$from }
        Rename-Item -LiteralPath $from -NewName (Split-Path -Leaf $to)
        if ((Hash $to) -ne $m.sha256) { throw 'Video rename verification failed.' }
    }
    Set-File $newState $statePath
    if ((Hash $statePath) -ne $writes[-1].after) { throw 'Installation record verification failed.' }
    Remove-Item -LiteralPath $journalPath
} catch {
    $failure=$_
    try { Undo-Transaction $journal $false; Write-Output 'The previous installation was restored after the failed operation.' }
    catch { Write-Warning "Rollback could not finish. Keep modernization/history and run Recover.cmd after closing the game. $($_.Exception.Message)" }
    throw $failure
}
foreach ($w in $writes) {
    $p=Under $stage $w.name
    if (Test-Path -LiteralPath $p -PathType Leaf) { Remove-Item -LiteralPath $p }
}
if ($Action -eq 'restore') { Write-Output 'Restored original files and opening videos. Saves, settings and language choice are preserved.' }
else {
    Write-Output "All fixes installed: $($manifest.version). Output: $Resolution. Skip opening videos: $SkipVideos."
    Write-Output 'Launch through Steam. The in-game 1024 x 768 option selects native 1920 x 1080 missions.'
    Write-Output 'Other output sizes scale this 1080p image; they do not enable a native 4K HUD.'
}
