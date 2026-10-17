# ENTROPIA · Modo tour: abre el juego (sin editor), recorre las salas solo, usa los poderes, mide FPS y toma capturas.
# Uso (editor cerrado): powershell -ExecutionPolicy Bypass -File Tools\run-tour.ps1
# Resultado: líneas ENTROPIA_TOUR en Saved\BuildTest\tour.log y capturas en Saved\Tour\.
param([int]$TimeoutSeconds = 240)
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Proj = Join-Path $Root 'entropy.uproject'
$Log = Join-Path $Root 'Saved\BuildTest\tour.log'
$Shots = Join-Path $Root 'Saved\Tour'
New-Item -ItemType Directory -Force (Split-Path $Log) | Out-Null
if (Test-Path $Shots) { Remove-Item -Recurse -Force $Shots }
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { Write-Output 'ABORT: editor abierto'; exit 2 }

$Exe = 'D:\unreal projets\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$Args = @("`"$Proj`"", '/Game/ThirdPerson/Lvl_ThirdPerson', '-game', '-windowed', '-ResX=1280', '-ResY=720', '-EntropiaTour', '-NoSound', '-log', "-ABSLOG=`"$Log`"")
$P = Start-Process -FilePath $Exe -ArgumentList $Args -PassThru
if (-not $P.WaitForExit($TimeoutSeconds * 1000)) { Write-Output "TIMEOUT: el tour no terminó en $TimeoutSeconds s"; Stop-Process -Id $P.Id -Force }

Select-String -Path $Log -Pattern 'ENTROPIA_TOUR|ENTROPIA:' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[\s*\d+\]', '' }
Write-Output '--- errores / avisos del juego ---'
Select-String -Path $Log -Pattern 'Error|Warning|Ensure|Accessed None' | Where-Object { $_.Line -notmatch 'Condition failed|UnifiedErrorTest|FError that has|Failed to load|LayoutService|ModelContextProtocol|Wintab|LogEditorDataStorage|LogPython' } | Select-Object -First 25 | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[\s*\d+\]', '' }
Write-Output '--- capturas ---'
Get-ChildItem $Shots -Filter *.png -ErrorAction SilentlyContinue | ForEach-Object { $_.Name }
