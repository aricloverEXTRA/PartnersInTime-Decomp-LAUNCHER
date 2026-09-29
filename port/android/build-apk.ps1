<#
.SYNOPSIS
Builds the Android patcher APK with the platform tools directly.

.DESCRIPTION
No Gradle, no Android Gradle Plugin and no NDK: the app is plain Java against
the platform APIs, so aapt2, javac, d8, zipalign and apksigner are enough. The
script drives them in the order the APK format requires and stops on the first
nonzero exit status, because PowerShell keeps going after a failed native
command and a later success would otherwise hide a broken earlier step.

It compiles the same generated PatchData the Windows build uses, so the two
patchers cannot drift apart.

.PARAMETER OutDir
Build output directory. Defaults to android/app/build.

.PARAMETER Unsigned
Build an unsigned APK instead of a signed one. CmdletBinding() already supplies
-Debug as a common parameter, so the switch is named Unsigned to avoid the
collision. An unsigned APK will not install until it is signed.
#>
[CmdletBinding()]
param(
    [string] $OutDir,
    [switch] $Unsigned
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# $PSScriptRoot is not reliable in a param default under Windows PowerShell 5.1,
# so the default is resolved here instead.
if (-not $OutDir) { $OutDir = Join-Path $PSScriptRoot 'app\build' }

$appRoot     = Join-Path $PSScriptRoot 'app'
$manifestDir = Join-Path $appRoot 'src\main'
$javaRoot    = Join-Path $manifestDir 'java'
$resRoot     = Join-Path $manifestDir 'res'
$packageName = 'com.partnersintime.patcher'

# Build tools and platform, overridable for a different SDK layout.
$sdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME }
       elseif ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT }
       else { 'C:\Users\ruthi\AppData\Local\Temp\opencode\toolchain\android-sdk' }

$buildTools = Get-ChildItem (Join-Path $sdk 'build-tools') -Directory |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $buildTools) { throw "No build-tools found under $sdk" }

$platform = Get-ChildItem (Join-Path $sdk 'platforms') -Directory |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $platform) { throw "No platform found under $sdk" }

$bt = $buildTools.FullName
$androidJar = Join-Path $platform.FullName 'android.jar'

$javac = if ($env:JAVA_HOME) { Join-Path $env:JAVA_HOME 'bin\javac.exe' }
         else { (Get-Command javac -ErrorAction Stop).Source }
$keytool = if ($env:JAVA_HOME) { Join-Path $env:JAVA_HOME 'bin\keytool.exe' }
           else { (Get-Command keytool -ErrorAction Stop).Source }
$apksigner = Join-Path $bt 'apksigner.bat'
$zipalign = Join-Path $bt 'zipalign.exe'

# Every step goes through this, so a failure stops the build instead of being
# swallowed by PowerShell's habit of continuing after a native command fails.
function Invoke-Step {
    # $Arguments rather than $Args: the latter is an automatic variable, so
    # binding to it silently drops the argument list.
    param([string] $Name, [string] $Exe, [string[]] $Arguments)
    Write-Host "==> $Name"
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Name failed with exit code $LASTEXITCODE" }
}

if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
$gen    = Join-Path $OutDir 'gen'
$classes = Join-Path $OutDir 'classes'
$dexDir = Join-Path $OutDir 'dex'
New-Item -ItemType Directory -Force -Path $gen, $classes, $dexDir | Out-Null

# 1. Resource and manifest compilation. aapt2 emits flat files, then link turns
#    them plus the manifest into the packaged resources of the APK.
Invoke-Step 'aapt2 compile' (Join-Path $bt 'aapt2.exe') @(
    'compile', '--dir', $resRoot, '-o', (Join-Path $OutDir 'res.zip')
)
$flatDir = Join-Path $gen 'res'
# The link output is an APK rather than a resource-only .ap_ because aapt2 puts
# the compiled AndroidManifest.xml at the archive root only for an .apk target.
# A .ap_ holds resources alone, and apksigner rejects an APK with no manifest.
Invoke-Step 'aapt2 link' (Join-Path $bt 'aapt2.exe') @(
    'link',
    '-I', $androidJar,
    '--manifest', (Join-Path $manifestDir 'AndroidManifest.xml'),
    '-o', (Join-Path $OutDir 'resources.apk'),
    '--java', $gen,
    (Join-Path $OutDir 'res.zip')
)

# 2. Java to class files, then classes to dex. -source/-target pin the bytecode
#    level so d8 sees something it can convert on any toolchain version.
$sources = @(Get-ChildItem $javaRoot -Recurse -Filter *.java | ForEach-Object FullName)
$generated = @(Get-ChildItem $gen -Recurse -Filter *.java -ErrorAction SilentlyContinue |
    ForEach-Object FullName)
Invoke-Step 'javac' $javac (@('-source', '8', '-target', '8', '-nowarn',
    '-encoding', 'UTF-8', '-classpath', $androidJar, '-d', $classes) + $sources + $generated)

Invoke-Step 'd8' (Join-Path $bt 'd8.bat') (@(
    '--lib', $androidJar, '--min-api', '21', '--output', $dexDir
) + @(Get-ChildItem $classes -Recurse -Filter *.class | ForEach-Object FullName))

# 3. Assemble the unsigned APK. dex and resources.ap_ go in under the names the
#    APK format requires; the manifest is already inside resources.ap_.
#    $unsignedApk, not $unsigned: PowerShell variable names are
#    case-insensitive, so a shorter spelling would overwrite the [switch]
#    $Unsigned parameter above.
$unsignedApk = Join-Path $OutDir 'pit-patcher-unsigned.apk'
if (Test-Path $unsignedApk) { Remove-Item $unsignedApk -Force }

# Compress-Archive emits directory entries and, on some host versions,
# backslashes in entry names, which apksigner rejects. Entries are therefore
# written directly, in a fixed order, with a constant timestamp so two builds of
# the same sources produce the same archive.
Add-Type -AssemblyName System.IO.Compression.FileSystem

# Every entry is stored uncompressed with a constant timestamp. Stored entries
# are required for resources.arsc and the dex, and a fixed timestamp keeps two
# builds of identical sources byte-identical.
function Add-StoredEntry {
    param($Archive, [string] $Name, [byte[]] $Bytes)
    $entry = $Archive.CreateEntry($Name, [System.IO.Compression.CompressionLevel]::NoCompression)
    $entry.LastWriteTime = [DateTimeOffset]::FromUnixTimeSeconds(315532800)
    $dest = $entry.Open()
    try { $dest.Write($Bytes, 0, $Bytes.Length) } finally { $dest.Dispose() }
}

$apkStream = [System.IO.Compression.ZipFile]::Open($unsignedApk, 'Create')
try {
    # Carry the linked manifest and resources across unchanged, preserving
    # aapt2's own entry names and order.
    $linked = [System.IO.Compression.ZipFile]::OpenRead((Join-Path $OutDir 'resources.apk'))
    try {
        foreach ($entry in $linked.Entries) {
            if ($entry.Name -eq '') { continue }
            $input = $entry.Open()
            try {
                $memory = New-Object System.IO.MemoryStream
                try {
                    $input.CopyTo($memory)
                    Add-StoredEntry $apkStream $entry.FullName $memory.ToArray()
                } finally { $memory.Dispose() }
            } finally { $input.Dispose() }
        }
    } finally { $linked.Dispose() }

    Add-StoredEntry $apkStream 'classes.dex' `
        ([System.IO.File]::ReadAllBytes((Join-Path $dexDir 'classes.dex')))
} finally {
    $apkStream.Dispose()
}

# 4. Align, then sign. Alignment must precede signing because signing rewrites
#    the central directory, which would undo it.
$alignedApk = Join-Path $OutDir 'pit-patcher-aligned.apk'
Invoke-Step 'zipalign' $zipalign @('-f', '-p', '4', $unsignedApk, $alignedApk)

$variant = if ($Unsigned) { 'unsigned' } else { 'release' }
$apk = Join-Path $OutDir "pit-patcher-$variant.apk"

if ($Unsigned) {
    Copy-Item $alignedApk $apk -Force
    Write-Warning 'Unsigned build will not install until it is signed.'
} else {
    $keystore = Join-Path $OutDir 'signing.keystore'
    if (-not (Test-Path $keystore)) {
        # A throwaway local key, generated here and never committed. Release
        # APKs are signed with the project's own key outside this script.
        Invoke-Step 'keytool' $keytool @('-genkeypair', '-keystore', $keystore,
            '-storepass', 'android', '-keypass', 'android', '-alias', 'pitpatcher',
            '-keyalg', 'RSA', '-keysize', '2048', '-validity', '10000',
            '-dname', 'CN=PiT Patcher, OU=Port, O=PartnersInTime, C=US')
    }
    # --v3-signing-enabled false keeps the signature scheme v1/v2 only, so the
    # APK verifies all the way down to the declared minSdk. The v3+ block
    # carries a SHA-256 signature that older platforms reject outright.
    Invoke-Step 'apksigner' $apksigner @('sign', '--ks', $keystore,
        '--ks-pass', 'pass:android', '--key-pass', 'pass:android',
        '--ks-key-alias', 'pitpatcher', '--min-sdk-version', '21',
        '--v3-signing-enabled', 'false', '--out', $apk, $alignedApk)
    Invoke-Step 'apksigner verify' $apksigner @('verify', '--min-sdk-version', '21', $apk)
}

Write-Host ''
Write-Host "APK: $apk"
Write-Host "Size: $([math]::Round((Get-Item $apk).Length / 1MB, 2)) MiB"
Write-Host "Package: $packageName"
Write-Host ''
Write-Host 'This APK contains no ROM data. It needs the user''s own EUR cartridge ROM.'
