<#
.SYNOPSIS
Builds the distributable Windows patcher, verifies it, and packages a ZIP.

.DESCRIPTION
Produces a release ZIP containing pit_patcher.exe, the headless pit_patch.exe
and a README. The build is self-contained: it resolves an SDL2 development
install, failing with a clear message rather than producing a binary that
cannot start.

Patched ROMs are test artifacts. Nothing under build/ is packaged, and no ROM,
extracted data or generated export is ever copied into the ZIP.
#>
[CmdletBinding()]
param(
    [string] $OutDir,
    [string] $Sdl2Path
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Defaults are resolved in the body: $PSScriptRoot is not reliable in a param
# default under Windows PowerShell 5.1.
if (-not $OutDir) { $OutDir = Join-Path $PSScriptRoot 'build\release' }

# $PSScriptRoot is already the port directory; the decompilation root is its
# parent, which is only used for the packaged ZIP path.
$portRoot = $PSScriptRoot
$src      = Join-Path $portRoot 'src'
$buildDir = Join-Path $portRoot 'build'

function Invoke-Step {
    # $Arguments rather than $Args: the latter is an automatic variable, so
    # binding to it silently drops the argument list.
    param([string] $Name, [string] $Exe, [string[]] $Arguments)
    Write-Host "==> $Name"
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Name failed with exit code $LASTEXITCODE" }
}

# A C compiler and SDL2 are the only prerequisites. The patcher links SDL2
# dynamically, so a user with a matching runtime can run it without a rebuild.
$gcc = if ($env:CC) { $env:CC }
       else { (Get-Command gcc -ErrorAction SilentlyContinue).Source }
if (-not $gcc) { $gcc = 'C:\Strawberry\c\bin\gcc.exe' }
if (-not (Test-Path $gcc)) { throw 'No C compiler found: set $env:CC or install MinGW gcc.' }

function Resolve-Sdl2 {
    # An explicit -Sdl2Path wins. Otherwise the usual prefixes are searched for
    # the two things a build actually needs: SDL2.h and libSDL2.a. Both may sit
    # directly in the prefix, as they do in a CMake install tree, or under lib.
    $candidates = @($Sdl2Path, $env:SDL2DIR, 'C:\msys64\mingw64', 'C:\msys64\ucrt64',
                    'C:\Strawberry\c', 'C:\SDL2', 'C:\Program Files\SDL2')

    foreach ($root in $candidates) {
        if (-not $root -or -not (Test-Path $root)) { continue }

        $header = Get-ChildItem $root -Recurse -Filter 'SDL.h' -ErrorAction SilentlyContinue |
            Where-Object { $_.DirectoryName -match 'SDL2$' } | Select-Object -First 1
        $libs = Get-ChildItem $root -Recurse -Filter 'libSDL2.a' -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($header -and $libs) {
            # SDL.h lives in include/SDL2, so its own directory is the include
            # path: <SDL.h> needs the parent of SDL2, not the parent of include.
            # SDL_config.h is generated rather than installed, so a CMake build
            # tree needs its directory added as a second include path.
            $includes = @($header.DirectoryName)
            $config = Get-ChildItem $root -Recurse -Filter 'SDL_config.h' -ErrorAction SilentlyContinue |
                Select-Object -First 1
            if ($config) { $includes += $config.DirectoryName }

            return [pscustomobject]@{
                Root     = $root
                Includes = $includes
                LibDir   = $libs.Directory.FullName
                Sdl2Lib  = $libs.FullName
            }
        }
    }
    return $null
}

$sdl2 = Resolve-Sdl2
if (-not $sdl2) {
    throw @'
SDL2 development files were not found.

Install the MinGW SDL2 development package, or pass the prefix explicitly:

    .\build-release.ps1 -Sdl2Path C:\path\to\sdl2
'@
}

# SDL2_main supplies main() and SDL2main is the WinMain wrapper; a
# console-subsystem build on MinGW needs both.
$sdl2main = Get-ChildItem $sdl2.LibDir -Filter 'libSDL2main.a' -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $sdl2main) {
    throw "libSDL2main.a not found in $($sdl2.LibDir). Install the SDL2 development package."
}
$libs = @($sdl2main.FullName, $sdl2.Sdl2Lib)

# Win32 libraries SDL2's MinGW build pulls in, and comdlg32 for the Browse
# dialog in the graphical patcher.
$winLibs = @('-lmingw32', '-lwinmm', '-limm32', '-lole32', '-loleaut32', '-lshell32',
             '-lsetupapi', '-lcomdlg32', '-lgdi32', '-luser32', '-ladvapi32',
             '-lversion', '-luuid', '-lz')

$core = @(
    (Join-Path $src 'core\pit_patcher.c'),
    (Join-Path $src 'core\pit_rom.c'),
    (Join-Path $src 'core\pit_sha1.c'),
    (Join-Path $src 'core\pit_gfx.c'),
    (Join-Path $src 'core\pit_png.c')
)

# Sources include their headers as "core/pit_rom.h", so src/ itself is on the
# include path rather than src/core/.
$cflags = @('-std=c11', '-Wall', '-Wextra', '-O2', '-D_CRT_SECURE_NO_WARNINGS',
           '-I', $src, '-L', $sdl2.LibDir)
$cflags += $sdl2.Includes | ForEach-Object { '-I'; $_ }

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

Invoke-Step 'pit_patch (headless)' $gcc (
    $cflags + @('-o', (Join-Path $buildDir 'pit_patch.exe'), (Join-Path $src 'patcher_main.c')) + $core + $libs + $winLibs)

Invoke-Step 'pit_patcher (graphical)' $gcc (
    $cflags + @('-I', $src, '-o', (Join-Path $buildDir 'pit_patcher.exe'),
        (Join-Path $src 'patcher_ui_main.c'),
        (Join-Path $src 'platform\sdl2\pit_patcher_ui.c')) + $core + $libs + $winLibs)

# The UI is rendered headlessly and serialised to PNG and text. A layout that
# cannot be drawn must fail the build, not ship.
$shot = Join-Path $buildDir 'release-ui.png'
Invoke-Step 'UI render check' (Join-Path $buildDir 'pit_patcher.exe') @(
    '--screenshot', $shot, '--simulate')
if (-not (Test-Path $shot)) { throw "UI render produced no image at $shot" }
Write-Host "    wrote $shot"

# --- package -------------------------------------------------------------
if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$stage = Join-Path $OutDir 'PiT-Patcher'
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item (Join-Path $buildDir 'pit_patcher.exe') $stage
Copy-Item (Join-Path $buildDir 'pit_patch.exe') $stage
Copy-Item (Join-Path $portRoot 'README.md') $stage
$shotDest = Join-Path $stage 'ui-preview.png'
Copy-Item $shot $shotDest

# Refuse to package anything that could carry game data. The check is on the
# staged tree, so it also catches a stray file added by a later step.
$forbidden = Get-ChildItem $stage -Recurse -File | Where-Object {
    $_.Extension -in @('.nds', '.bin', '.rom', '.gba', '.zip', '.json') -or
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
