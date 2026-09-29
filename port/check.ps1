<#
.SYNOPSIS
Runs every patcher check that does not need a ROM.

.DESCRIPTION
Covers the checks that are meaningful in a clean checkout: generated data is
current, the plan and font are internally consistent, the portable Java classes
compile and pass their class-path tests, and the source tree carries no ROM or
extracted game data.

Checks that need a cartridge are not run here. They require the user's own copy
of the EUR ROM, which is deliberately never part of the repository, and are run
separately:

    python tools/check_patch_roundtrip.py <patched.nds> <export.json>
    python tools/check_c_java_parity.py --rom <rom.nds>
#>
[CmdletBinding()]
param(
    [switch] $SkipJava,
    # The launcher frame check needs a built pit_patcher.exe. It is skipped with a
    # message rather than failing when the binary is absent, so a fresh checkout
    # without a toolchain still runs the rest.
    [switch] $SkipUi
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$portRoot = $PSScriptRoot
$buildDir = Join-Path $portRoot 'build\checks'
$failed = 0

function Invoke-Check {
    param([string] $Name, [scriptblock] $Body)
    Write-Host ''
    Write-Host "--- $Name"
    try {
        & $Body
        if ($LASTEXITCODE -ne 0 -and $null -ne $LASTEXITCODE) {
            throw "exit code $LASTEXITCODE"
        }
        Write-Host "    PASS"
    } catch {
        Write-Host "    FAIL $($_.Exception.Message)"
        $script:failed++
    }
}

$python = (Get-Command py -ErrorAction SilentlyContinue).Source
if (-not $python) { $python = (Get-Command python -ErrorAction SilentlyContinue).Source }
if (-not $python) { throw 'No Python found: install Python 3.11+ or put py/python on PATH.' }
function Invoke-Python {
    param([string[]] $Arguments)
    if ($python -match 'py(\.exe)?$') { & $python -3.12 @Arguments }
    else { & $python @Arguments }
}

Invoke-Check 'generated patch data is current' {
    Push-Location $portRoot
    try { Invoke-Python @('tools\gen_patchplan.py', '--check') } finally { Pop-Location }
}

Invoke-Check 'font glyphs are valid and match the generated tables' {
    Push-Location $portRoot
    try { Invoke-Python @('tools\check_font.py') } finally { Pop-Location }
}

Invoke-Check 'Windows and Android layouts match' {
    # Needs the Android SDK: aapt2 resolves the classpath so the view's Canvas
    # calls are type-checked. Skipped with a message when it is absent, since a
    # developer without the SDK should still get the rest of the suite.
    $sdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME }
           elseif ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT }
           else { 'C:\Users\ruthi\AppData\Local\Temp\opencode\toolchain\android-sdk' }
    $androidJar = Get-ChildItem (Join-Path $sdk 'platforms') -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1 |
        ForEach-Object { Join-Path $_.FullName 'android.jar' }
    $javac = if ($env:JAVA_HOME) { Join-Path $env:JAVA_HOME 'bin\javac.exe' }
             else { (Get-Command javac -ErrorAction SilentlyContinue).Source }

    if (-not $androidJar -or -not (Test-Path $androidJar) -or -not $javac) {
        Write-Host '    (no Android SDK or JDK; Android sources not type-checked)'
        return
    }
    $viewClasses = Join-Path $buildDir 'android-view'
    New-Item -ItemType Directory -Force $viewClasses | Out-Null
    $pkgDir = Join-Path $portRoot 'android\app\src\main\java\com\partnersintime\patcher'
    & $javac -nowarn -encoding UTF-8 -classpath $androidJar -d $viewClasses `
        (Join-Path $pkgDir 'PatchData.java') (Join-Path $pkgDir 'PatcherView.java')
    if ($LASTEXITCODE -ne 0) { throw "javac failed with exit code $LASTEXITCODE" }
}

Invoke-Check 'Windows and Android layout constants agree' {
    Push-Location $portRoot
    try { Invoke-Python @('tools\check_ui_parity.py') } finally { Pop-Location }
}

Invoke-Check 'no ROM or extracted game data in the tree' {
    Push-Location $portRoot
    try {
        $bad = Get-ChildItem . -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object {
                $_.FullName -notmatch '\\build(\\|$)' -and
                ($_.Extension -in @('.nds', '.gba', '.rom', '.sav') -or
                 $_.Name -match '^baserom|enemies\.json|\.keystore$')
            } |
            Select-Object -ExpandProperty FullName
        if ($bad) { throw "found: $($bad -join ', ')" }
    } finally { Pop-Location }
}

if (-not $SkipJava) {
    $javac = $null
    if ($env:JAVA_HOME) { $javac = Join-Path $env:JAVA_HOME 'bin\javac.exe' }
    if (-not $javac -or -not (Test-Path $javac)) {
        $javac = (Get-Command javac -ErrorAction SilentlyContinue).Source
    }
    $java = if ($env:JAVA_HOME) { Join-Path $env:JAVA_HOME 'bin\java.exe' } else { $null }
    if (-not $java -or -not (Test-Path $java)) {
        $java = (Get-Command java -ErrorAction SilentlyContinue).Source
    }

    if (-not $javac -or -not $java) {
        Write-Host ''
        Write-Host '--- Java checks SKIPPED (no JDK found; set JAVA_HOME)'
    } else {
        New-Item -ItemType Directory -Force $buildDir | Out-Null

        # Only the Android-independent classes: MainActivity and PatcherView
        # need android.* and cannot run on a desktop JVM.
        $pkg = Join-Path $portRoot 'android\app\src\main\java\com\partnersintime\patcher'
        $srcs = @('PatchData.java', 'Patcher.java', 'NitroFs.java', 'Sha1.java') |
            ForEach-Object { Join-Path $pkg $_ }
        $srcs += Join-Path $portRoot 'tools\java\com\partnersintime\patcher\PitSelfTest.java'
        $srcs += Join-Path $portRoot 'tools\java\com\partnersintime\patcher\PitUnitTest.java'

        Invoke-Check 'Java patcher compiles without warnings' {
            & $javac -Xlint:all -Werror -d $buildDir @srcs
        }
        Invoke-Check 'Java patcher unit tests' {
            & $java -cp $buildDir com.partnersintime.patcher.PitUnitTest
        }
    }
}

if ($SkipUi) {
    Write-Host ''
    Write-Host '--- Launcher frame check SKIPPED (-SkipUi)'
} else {
    $patcherExe = Join-Path $portRoot 'build\pit_patcher.exe'
    if (-not (Test-Path $patcherExe)) {
        Write-Host ''
        Write-Host '--- Launcher frame check SKIPPED (build\pit_patcher.exe not built; run cmake --build build --target pit_patcher)'
    } else {
        $frameDir = Join-Path $portRoot 'build\ui-frames'
        New-Item -ItemType Directory -Force $frameDir | Out-Null

        # Two frames: idle and mid-patch. Both are drawn by the same render() the
        # window uses, so these catch layout and legibility regressions without a
        # display.
        Invoke-Check 'launcher frame: idle layout and legibility' {
            & $patcherExe --screenshot (Join-Path $frameDir 'idle.png')
            if ($LASTEXITCODE -ne 0) { throw "pit_patcher exited $LASTEXITCODE" }
            Push-Location $portRoot
            try {
                Invoke-Python @('tools\check_ui.py', 'build\ui-frames\idle.png')
            } finally { Pop-Location }
        }
        Invoke-Check 'launcher frame: mid-patch state' {
            & $patcherExe --screenshot (Join-Path $frameDir 'busy.png') --simulate
            if ($LASTEXITCODE -ne 0) { throw "pit_patcher exited $LASTEXITCODE" }
            Push-Location $portRoot
            try {
                Invoke-Python @('tools\check_ui.py', 'build\ui-frames\busy.png', '--expect-half')
            } finally { Pop-Location }
        }
    }
}

Write-Host ''
if ($failed -gt 0) {
    Write-Host "$failed check(s) failed"
    exit 1
}
Write-Host 'All ROM-independent checks passed.'
Write-Host 'ROM-dependent checks still to run:'
Write-Host '  python tools/check_patch_roundtrip.py <patched.nds> <export.json>'
Write-Host '  python tools/check_c_java_parity.py --rom <rom.nds>'
exit 0
