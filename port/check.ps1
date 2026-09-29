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
    [switch] $SkipJava
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
