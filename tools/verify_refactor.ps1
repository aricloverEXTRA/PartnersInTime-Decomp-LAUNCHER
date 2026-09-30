# Full verification for refactoring batches.
#
# Runs the configure/build/link chain, packs the ROM, compares its SHA-1 with the
# recorded European hash, regenerates the progress tracker and runs the tests.
# Every step must succeed; the script stops at the first failure.
param(
    [switch]$SkipTests,
    [switch]$SkipRelink
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root

# Both tools are located from PATH rather than a developer's absolute install
# path, so the script works on any machine with a working toolchain.
$ninja = Get-Command ninja.exe -ErrorAction SilentlyContinue
if (-not $ninja) { throw 'ninja not found on PATH: add a Ninja install first.' }
$pwshExe = (Get-Command pwsh.exe -ErrorAction SilentlyContinue).Source
if (-not $pwshExe) { $pwshExe = 'powershell.exe' }

function Step($name, [scriptblock]$body) {
    Write-Host "== $name" -ForegroundColor Cyan
    & $body
    if ($LASTEXITCODE -ne 0) { throw "$name failed with exit code $LASTEXITCODE" }
}

Step 'configure'    { python tools/configure.py eur }
Step 'all source objects' { ninja objects 2>&1 | Select-Object -Last 3 }
Step 'ninja check'  { ninja check 2>&1 | Select-Object -Last 3 }
Step 'package rom'  { & $pwshExe -NoProfile -ExecutionPolicy Bypass -File .\tools\build_nds.ps1 -DisableDataMods 2>&1 | Select-String -Pattern 'SHA-1|Built ROM|error|Error' }

$expected = (Get-Content .\PiT_eur.sha1 -Raw).Trim().Split()[0].ToLower()
$actual = (Get-FileHash -Algorithm SHA1 .\PiT_eur.nds).Hash.ToLower()
if ($actual -ne $expected) { throw "ROM MISMATCH: got $actual expected $expected" }
Write-Host "ROM SHA-1 OK: $actual" -ForegroundColor Green

if (-not $SkipRelink) {
    Step 'native relink' {
        python tools/relink_native.py --version eur --rom extract/baserom_PiT_eur.nds `
            --output-rom build/PiT_eur_native_relinked.nds --require-matching 2>&1 | Select-Object -Last 5
    }
}

Step 'progress'       { python tools/generate_progress.py }
Step 'progress check' { python tools/generate_progress.py --check }
if (-not $SkipTests) { Step 'pytest' { python -m pytest -q tests 2>&1 | Select-Object -Last 5 } }

Write-Host 'ALL VERIFICATION STEPS PASSED' -ForegroundColor Green
