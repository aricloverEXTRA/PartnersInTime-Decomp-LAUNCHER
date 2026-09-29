<#
.SYNOPSIS
Builds the distributable Windows patcher and packages it as a ZIP.

.DESCRIPTION
Produces a release ZIP containing pit_patcher.exe, the headless pit_patch.exe,
a rendered preview of the launcher and a README.

The build goes through CMake, which is the same path CI uses, so a ZIP built
here is built the same way a CI artifact is. CMake fetches and statically links
SDL2 from the tag pinned in CMakeLists.txt, so the result is a single EXE with
no runtime to install and no SDL2 development package to find first. A
generator is chosen from what is present; Ninja is preferred.

Patched ROMs are test artifacts. Nothing under build/ is packaged, and no ROM,
extracted data or generated export is ever copied into the ZIP.
#>
[CmdletBinding()]
param(
    [string] $OutDir,
    [string] $BuildDir
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Defaults are resolved in the body: $PSScriptRoot is not reliable in a param
# default under Windows PowerShell 5.1.
if (-not $OutDir) { $OutDir = Join-Path $PSScriptRoot 'build\release' }
if (-not $BuildDir) { $BuildDir = Join-Path $PSScriptRoot 'build' }

$portRoot = $PSScriptRoot

function Invoke-Step {
    # $Arguments rather than $Args: the latter is an automatic variable, so
    # binding to it silently drops the argument list.
    param([string] $Name, [string] $Exe, [string[]] $Arguments)
    Write-Host "==> $Name"
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Name failed with exit code $LASTEXITCODE" }
}

function Resolve-Tool {
    param([string] $Name, [string[]] $Candidates)
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path $candidate)) { return $candidate }
    }
    return $null
}

$cmake = Resolve-Tool 'cmake' @(
    'C:\Program Files\CMake\bin\cmake.exe',
    'C:\Strawberry\c\bin\cmake.exe')
if (-not $cmake) { throw 'No CMake found: install CMake 3.21 or newer.' }

# Ninja when it is available, because it is what CI uses; otherwise the Visual
# Studio or NMake generator, whichever this host has.
$ninja = Resolve-Tool 'ninja' @(
    'C:\Program Files\CMake\bin\ninja.exe',
    'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe')
$generator = $null
$generatorArgs = @()
if ($ninja -and (Split-Path $ninja -Leaf) -eq 'ninja.exe') {
    $generator = 'Ninja'
    $generatorArgs = @('-G', 'Ninja')
} elseif (-not (Resolve-Tool 'cl' @()) -and -not $env:VSCMD_ARG_TGT_ARCH) {
    # No compiler on PATH and no Visual Studio environment: say so rather than
    # letting CMake fail with a generator error.
    throw @'
No C compiler found.

Run this from a Visual Studio developer prompt, or put gcc, clang or cl on
PATH. CI uses the windows-latest image, which has a compiler already.
'@
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

Invoke-Step 'configure' $cmake (@('-S', $portRoot, '-B', $BuildDir) +
    $generatorArgs + @('-DCMAKE_BUILD_TYPE=RelWithDebInfo'))
Invoke-Step 'build' $cmake (@('--build', $BuildDir, '--config', 'RelWithDebInfo'))

$patcher = Join-Path $BuildDir 'pit_patcher.exe'
$headless = Join-Path $BuildDir 'pit_patch.exe'
foreach ($binary in @($patcher, $headless)) {
    if (-not (Test-Path $binary)) { throw "build produced no $binary" }
}

# The UI is rendered headlessly and serialised to PNG. A layout that cannot be
# drawn must fail the release, not ship.
$shot = Join-Path $BuildDir 'release-ui.png'
Invoke-Step 'UI render check' $patcher @('--screenshot', $shot, '--simulate')
if (-not (Test-Path $shot)) { throw "UI render produced no image at $shot" }
Write-Host "    wrote $shot"

# --- package -------------------------------------------------------------
if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$stage = Join-Path $OutDir 'PiT-Patcher'
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item $patcher $stage
Copy-Item $headless $stage
Copy-Item (Join-Path $portRoot 'README.md') $stage
Copy-Item $shot (Join-Path $stage 'ui-preview.png')

# Refuse to package anything that could carry game data. The check is on the
# staged tree, so it also catches a stray file added by a later step.
$forbidden = Get-ChildItem $stage -Recurse -File | Where-Object {
    $_.Extension -in @('.nds', '.bin', '.rom', '.gba', '.json') -or
    $_.Name -match 'baserom|enemies\.json|\.keystore$'
}
if ($forbidden) {
    throw "refusing to package: $($forbidden.Name -join ', ')"
}

$zip = Join-Path $portRoot 'build\PiT-Patcher-win64.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($stage, $zip,
    [System.IO.Compression.CompressionLevel]::Optimal, $false)

Write-Host ''
Write-Host "ZIP: $zip"
Write-Host "     $([math]::Round((Get-Item $zip).Length / 1KB, 1)) KiB"
Write-Host ''
Write-Host 'The ZIP contains no ROM or extracted game data. Both binaries need the'
Write-Host "user's own EUR cartridge ROM, kept outside the repository."
