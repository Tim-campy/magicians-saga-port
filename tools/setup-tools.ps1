# Installs research tools only inside this checkout. Requires Windows PowerShell 5.1+.
# Official sources: github.com/1013503897/jadx-headless-mcp,
# github.com/iBotPeaches/Apktool, developer.android.com/tools/releases/platform-tools.
[CmdletBinding()]
param([string]$ProjectDirectory = (Join-Path $PSScriptRoot '..'))

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath($ProjectDirectory)
$toolRoot = Join-Path $projectRoot '.tools'
$downloadRoot = Join-Path $toolRoot 'downloads'
$javaCommand = (Get-Command java -ErrorAction Stop).Source
$null = Get-Command javac -ErrorAction Stop
$null = New-Item -ItemType Directory -Force -Path $downloadRoot
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Get-Download([string]$Url, [string]$Destination) {
    if (-not $Url.StartsWith('https://')) { throw 'HTTPS is required.' }
    Write-Host "Downloading $Url"
    $temporary = "$Destination.partial"
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $temporary -TimeoutSec 120
    if ((Get-Item -LiteralPath $temporary).Length -eq 0) { throw "Empty download: $Url" }
    Move-Item -LiteralPath $temporary -Destination $Destination -Force
}

function Assert-Sha256([string]$Path, [string]$Expected) {
    if ($Expected -notmatch '^[0-9a-fA-F]{64}$') { throw "Invalid expected SHA-256 for $Path" }
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Expected.ToLowerInvariant()) { throw "SHA-256 mismatch: $Path" }
    return $actual
}

function Expand-SafeZip([string]$Archive, [string]$Destination) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $root = [IO.Path]::GetFullPath($Destination).TrimEnd('\') + '\'
    $zip = [IO.Compression.ZipFile]::OpenRead($Archive)
    try {
        $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in $zip.Entries) {
            $name = $entry.FullName.Replace('/', '\')
            $target = [IO.Path]::GetFullPath((Join-Path $root $name))
            if (-not $target.StartsWith($root, [StringComparison]::OrdinalIgnoreCase) -or
                $name.Contains(':') -or -not $seen.Add($target)) {
                throw "Unsafe or duplicate ZIP entry: $name"
            }
            if ((($entry.ExternalAttributes -shr 16) -band 0xF000) -eq 0xA000) {
                throw "Symlink ZIP entry: $name"
            }
        }
        foreach ($entry in $zip.Entries) {
            $target = Join-Path $root $entry.FullName.Replace('/', '\')
            if ($entry.FullName.EndsWith('/')) {
                $null = New-Item -ItemType Directory -Force -Path $target
            } else {
                $null = New-Item -ItemType Directory -Force -Path ([IO.Path]::GetDirectoryName($target))
                [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target, $true)
            }
        }
    } finally { $zip.Dispose() }
}

$jadxVersion = '0.7.1'
$jadxBase = "https://github.com/1013503897/jadx-headless-mcp/releases/download/v$jadxVersion"
$jadxName = "jadx-headless-mcp-$jadxVersion-all.jar"
$jadxRoot = Join-Path $toolRoot 'jadx'
$jadxJar = Join-Path $jadxRoot $jadxName
$jadxChecksum = Join-Path $jadxRoot 'jadx-headless-mcp.sha256'
$null = New-Item -ItemType Directory -Force -Path $jadxRoot
if (-not (Test-Path -LiteralPath $jadxJar)) { Get-Download "$jadxBase/$jadxName" $jadxJar }
if (-not (Test-Path -LiteralPath $jadxChecksum)) { Get-Download "$jadxBase/jadx-headless-mcp.sha256" $jadxChecksum }
$hashMatch = [regex]::Match((Get-Content -LiteralPath $jadxChecksum -Raw), '(?im)^([a-f0-9]{64})(?:\s+\*?jadx-headless-mcp-0\.7\.1-all\.jar)?\s*$')
if (-not $hashMatch.Success) { throw 'JADX checksum file has an unexpected format.' }
$jadxHash = Assert-Sha256 $jadxJar $hashMatch.Groups[1].Value

$apktoolVersion = '2.12.1'
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/iBotPeaches/Apktool/releases/tags/v$apktoolVersion" -Headers @{ 'User-Agent' = 'MagiciansSaga-research-setup' } -TimeoutSec 60
$apktoolName = "apktool_$apktoolVersion.jar"
$asset = @($release.assets | Where-Object { $_.name -eq $apktoolName })
if ($asset.Count -ne 1 -or $asset[0].digest -notmatch '^sha256:([a-f0-9]{64})$') {
    throw 'The pinned Apktool release must supply one JAR with a SHA-256 asset digest.'
}
$apktoolExpected = $Matches[1]
$apktoolJar = Join-Path $downloadRoot $apktoolName
$apktoolUrl = "https://github.com/iBotPeaches/Apktool/releases/download/v$apktoolVersion/$apktoolName"
Get-Download $apktoolUrl $apktoolJar
$apktoolHash = Assert-Sha256 $apktoolJar $apktoolExpected

# Google publishes this moving URL. The downloaded hash and actual revision are
# recorded below; this URL alone is not a reproducible version pin.
$platformUrl = 'https://dl.google.com/android/repository/platform-tools-latest-windows.zip'
$platformZip = Join-Path $downloadRoot 'platform-tools-windows.zip'
Get-Download $platformUrl $platformZip
$platformHash = (Get-FileHash -LiteralPath $platformZip -Algorithm SHA256).Hash.ToLowerInvariant()
$platformRoot = Join-Path $toolRoot ('android-' + $platformHash.Substring(0, 12))
Expand-SafeZip $platformZip $platformRoot
$adbPath = Join-Path $platformRoot 'platform-tools/adb.exe'
if (-not (Test-Path -LiteralPath $adbPath)) { throw 'ADB missing from the downloaded archive.' }

$binRoot = Join-Path $toolRoot 'bin'
$null = New-Item -ItemType Directory -Force -Path $binRoot, $jadxRoot
Copy-Item -LiteralPath $apktoolJar -Destination (Join-Path $binRoot 'apktool.jar') -Force
Set-Content -LiteralPath (Join-Path $binRoot 'apktool.cmd') -Encoding ASCII -Value '@echo off', 'java -jar "%~dp0apktool.jar" %*', 'exit /b %errorlevel%'
$apktoolReported = & $javaCommand -jar (Join-Path $binRoot 'apktool.jar') --version
if ($LASTEXITCODE -ne 0) { throw 'Apktool validation failed.' }
$adbReported = & $adbPath version
if ($LASTEXITCODE -ne 0) { throw 'ADB validation failed.' }
$manifest = [ordered]@{
    installed_at_utc = [DateTime]::UtcNow.ToString('o')
    jadx = @{ version = $jadxVersion; url = "$jadxBase/$jadxName"; sha256 = $jadxHash; validation = 'release SHA-256 matched; REA analysis still required' }
    apktool = @{ version = $apktoolReported; url = $apktoolUrl; sha256 = $apktoolHash; validation = 'GitHub release digest matched; --version passed' }
    platform_tools = @{ version = $adbReported; url = $platformUrl; sha256 = $platformHash; validation = 'archive hash recorded, not compared to publisher checksum; adb version passed' }
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $toolRoot 'installed-tools.json') -Encoding UTF8
Set-Content -LiteralPath (Join-Path $toolRoot 'environment.cmd') -Encoding ASCII -Value (
    '@echo off',
    ('set "REA_JADX_MCP_JAR=%~dp0jadx\' + $jadxName + '"'),
    ('set "PATH=%~dp0bin;%~dp0android-' + $platformHash.Substring(0, 12) + '\platform-tools;%PATH%"')
)
Write-Host 'Local tools installed. Run tools\with-tools.cmd rea.cmd inspect-android-package --help.'
