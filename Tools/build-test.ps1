# ENTROPIA · Compila el editor y corre las pruebas Entropia.* (con el editor cerrado).
# Uso: powershell -ExecutionPolicy Bypass -File Tools\build-test.ps1 [-NoTests]   ·   Logs en Saved\BuildTest\
param([switch]$NoTests)
$ErrorActionPreference = 'Continue'
$W = Join-Path (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)) "Saved\BuildTest"; New-Item -ItemType Directory -Force $W | Out-Null
$Proj = 'D:\unreal projets\entropy\entropy.uproject'
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { Write-Output 'ABORT: editor abierto'; exit 2 }

& 'D:\unreal projets\UE_5.8\Engine\Build\BatchFiles\Build.bat' entropyEditor Win64 Development "-Project=$Proj" -WaitMutex -NoHotReloadFromIDE *> "$W\build.log"
$build = Select-String -Path "$W\build.log" -Pattern 'error|warning C|Result:' | ForEach-Object { $_.Line.Trim() } | Select-Object -Last 25
$build
if (-not ($build -match 'Result: Succeeded')) { Write-Output 'BUILD FAILED'; exit 1 }
if ($NoTests) { exit 0 }

& 'D:\unreal projets\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $Proj -ExecCmds="Automation RunTests Entropia;Quit" -unattended -nopause -nullrhi -nosplash -NoSound -log "-ABSLOG=$W\tests.log" *> "$W\tests-stdout.log"
$res = Select-String -Path "$W\tests.log" -Pattern 'Test Completed\. Result=|No automation tests matched|Error: ' | ForEach-Object { $_.Line.Trim() } | Where-Object { $_ -notmatch 'Condition failed' } | ForEach-Object { $_ -replace '^.*(Result=\{\w+\}).*Path=\{([^}]*)\}.*$', '$1 $2' }
$res
$fails = ($res | Where-Object { $_ -match 'Result=\{Fail' }).Count
$ok = ($res | Where-Object { $_ -match 'Result=\{Success' }).Count
Write-Output "TESTS: $ok ok, $fails fail"
if ($fails -gt 0 -or $ok -eq 0) { exit 1 }
exit 0
