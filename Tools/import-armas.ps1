# ENTROPIA · Importa los modelos de armas de Fab (con el editor cerrado).
# Los modelos NO van a GitHub (la licencia de Fab no deja publicarlos): viven en Content/Entropia/ArmasFab (ignorado).
# Fuentes: D:\unreal projets\armas_assets\ (revolver.glb, fusil.fbx, pistola.fbx). En otra compu: bajarlos de Fab
# ("Free Revolver / Handcanon", "Low poly weapons + Test map": fusil_6 y pistol_2), copiarlos ahí y correr esto.
# Uso: powershell -ExecutionPolicy Bypass -File Tools\import-armas.ps1   ·   Logs en Saved\BuildTest\import_*.log
param([string]$Source = 'D:\unreal projets\armas_assets')
$ErrorActionPreference = 'Continue'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Proj = Join-Path $Root 'entropy.uproject'
$W = Join-Path $Root 'Saved\BuildTest'; New-Item -ItemType Directory -Force $W | Out-Null
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { Write-Output 'ABORT: editor abierto'; exit 2 }

$Items = @(
	@{ File = 'revolver.glb'; Dest = '/Game/Entropia/ArmasFab/Revolver' },
	@{ File = 'fusil.fbx'; Dest = '/Game/Entropia/ArmasFab/Fusil' },
	@{ File = 'pistola.fbx'; Dest = '/Game/Entropia/ArmasFab/Pistola' }
)
$Failed = 0
foreach ($Item in $Items) {
	$Path = Join-Path $Source $Item.File
	if (-not (Test-Path $Path)) { Write-Output "FALTA: $Path"; $Failed++; continue }
	$Name = [IO.Path]::GetFileNameWithoutExtension($Item.File)
	$Log = Join-Path $W "import_$Name.log"
	& 'D:\unreal projets\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $Proj -run=ImportAssets "-source=$($Path -replace '\\','/')" "-dest=$($Item.Dest)" -replaceexisting -nosourcecontrol -unattended -nullrhi -NoSound "-ABSLOG=$Log" *> $null
	$Result = Select-String -Path $Log -Pattern 'Success - |Failure - ' | Select-Object -Last 1
	Write-Output "$($Item.File): $($Result.Line -replace '^.*Display: ','')"
	if (-not ($Result -and $Result.Line -match 'Success - 0 error')) { $Failed++ }
}
# Marca para que, al abrir el editor (o el tour), se limiten las texturas a 1024 y se apague Nanite una sola vez.
Set-Content -Path (Join-Path $Root 'Saved\ArmasFab.pendiente') -Value (Get-Date -Format o) -Encoding utf8
if ($Failed -gt 0) { Write-Output "IMPORTACION CON FALLAS: $Failed"; exit 1 }
exit 0
