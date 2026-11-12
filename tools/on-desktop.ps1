# Starts a program on the desktop of the signed-in user, from an SSH session.
#
# A program started directly from an SSH session runs in session 0: it has no window anyone can see, and the Unreal
# editor gets no usable display there. This registers a one-off scheduled task that runs "only when the user is
# logged on", which puts the program on the real desktop, where Marwan sees it through Moonlight.
#
# Usage:  powershell -ExecutionPolicy Bypass -File tools\on-desktop.ps1 <exe> ["<arguments as one string>"]
# Never run when written: if it fails, fix it here and note the fix in docs/UNREAL_MCP.md.
param(
  [Parameter(Mandatory = $true)][string]$Exe,
  [string]$Arguments = "",
  [string]$TaskName = "JamOnDesktop"
)
$ErrorActionPreference = "Stop"
if (-not (Test-Path $Exe)) { throw "Not found: $Exe" }

$actionArgs = @{ Execute = $Exe; WorkingDirectory = (Split-Path -Parent $Exe) }
if ($Arguments) { $actionArgs.Argument = $Arguments }
$action    = New-ScheduledTaskAction @actionArgs
$user      = [Security.Principal.WindowsIdentity]::GetCurrent().Name
$principal = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Limited
$settings  = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit (New-TimeSpan -Seconds 0)
Register-ScheduledTask -TaskName $TaskName -Action $action -Principal $principal -Settings $settings -Force | Out-Null
Start-ScheduledTask -TaskName $TaskName

Start-Sleep -Seconds 5
$name = [IO.Path]::GetFileNameWithoutExtension($Exe)
$onDesktop = @(Get-Process -Name $name -ErrorAction SilentlyContinue | Where-Object { $_.SessionId -gt 0 })
if ($onDesktop.Count -gt 0) {
  "Started $name on the desktop (session $($onDesktop[0].SessionId), pid $($onDesktop[0].Id))."
} else {
  "$name is not running on the desktop. Nobody may be signed in to Windows: ask Marwan to sign in through Moonlight, then retry."
  exit 1
}
