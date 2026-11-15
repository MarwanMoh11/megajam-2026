# The coordinator's batched restart (docs/SCRIPT_ADAPT.md "Restarts"). Run from PowerShell in the repo root:
#   powershell -NoProfile -File tools\script\restart.ps1 [-Label 1440]
# Takes the editor lock at the front of the queue, saves dirty packages, exits the editor, builds ReelEditor,
# starts the editor, waits for MCP, releases the lock. Log: C:\Jam\Builds\script\restart-<Label>.log
param([string]$Label = (Get-Date -Format "HHmm"))
$ErrorActionPreference = "Continue"
$Repo = "C:\Jam\megajam"
$Py = "C:\Epic Games\UE_5.9\Engine\Binaries\ThirdParty\Python3\Win64\python.exe"
$Log = "C:\Jam\Builds\script\restart-$Label.log"
$BuildLog = "C:\Jam\Builds\script\build-$Label.log"
function Say($m) { $l = "$(Get-Date -Format 'HH:mm:ss') $m"; $l | Tee-Object -FilePath $Log -Append }
Set-Location $Repo

Say "acquire lock (front)"
& $Py -I tools\editor_lock.py acquire coordinator --front --wait 900 --note "restart $Label" | ForEach-Object { Say $_ }
if ($LASTEXITCODE -ne 0) { Say "RESULT: no lock"; exit 2 }

$up = $false
try { Invoke-WebRequest -UseBasicParsing -Method Post -Uri http://127.0.0.1:8000/mcp -TimeoutSec 3 | Out-Null; $up = $true } catch { if ($_.Exception.Response) { $up = $true } }
if ($up) {
  Say "save dirty packages"
  $save = Join-Path $env:TEMP "reel_save_all.py"
  "import unreal`nunreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)`nprint('SAVED')" | Set-Content -Encoding utf8 $save
  & $Py -I tools\ue_mcp.py py $save 100 2>&1 | Select-Object -Last 3 | ForEach-Object { Say $_ }
  Say "exit editor"
  & $Py -I tools\ue_mcp.py tool EditorToolset.EditorAppToolset.ExitEditor '{\"bDiscardUnsavedChanges\":true}' 30 2>&1 | Select-Object -Last 2 | ForEach-Object { Say $_ }
}
for ($i = 0; $i -lt 60 -and (Get-Process UnrealEditor -ErrorAction SilentlyContinue); $i++) { Start-Sleep 2 }
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { Say "editor still running after 120 s: killing"; Stop-Process -Name UnrealEditor -Force; Start-Sleep 3 }

Say "build ReelEditor"
& "C:\Epic Games\UE_5.9\Engine\Build\BatchFiles\Build.bat" ReelEditor Win64 Development "-Project=C:\Jam\megajam\Reel\Reel.uproject" -WaitMutex > $BuildLog 2>&1
$code = $LASTEXITCODE
Say "build exit $code"
if ($code -ne 0) { Select-String -Path $BuildLog -Pattern "error " | Select-Object -First 8 | ForEach-Object { Say ("ERR " + $_.Line.Trim()) } }

Say "start editor"
Start-Process "C:\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor.exe" '"C:\Jam\megajam\Reel\Reel.uproject"'
$ok = $false
for ($i = 0; $i -lt 60; $i++) {
  Start-Sleep 3
  try { Invoke-WebRequest -UseBasicParsing -Method Post -Uri http://127.0.0.1:8000/mcp -TimeoutSec 3 | Out-Null; $ok = $true } catch { if ($_.Exception.Response) { $ok = $true } }
  if ($ok) { break }
}
Say ("mcp up: " + $ok)
& $Py -I tools\editor_lock.py release coordinator | ForEach-Object { Say $_ }
"$(Get-Date -Format 'HH:mm') coordinator restart $Label build=$code mcp=$ok" | Add-Content C:\Jam\Builds\script\restarts.log
Say "RESULT: build=$code mcp=$ok"
