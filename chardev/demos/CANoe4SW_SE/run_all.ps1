# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT

param (
    [string]$SILKitDir
)

# Check if exactly one argument is passed
if (-not $SILKitDir) {
    # If no argument is passed, check if SIL Kit dir has its own environment variable (for the ci-pipeline)
    $SILKitDir = $env:SILKit_InstallDir
    if (-not $SILKitDir) {
        Write-Host "Error: Either provide the path to the SIL Kit directory as an argument or set the `$env:SILKit_InstallDir` environment variable"
        Write-Host "Usage: .\run_all.ps1 <path_to_sil_kit_dir>"
        exit 1
    }
}

$logDir = Join-Path $PSScriptRoot "logs"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

# Create the log directory
if (-not (Test-Path -Path $logDir))
{
    mkdir -p $logDir | Out-Null
}

# Processes to run the executables and commands in background
$RegistryProcess = New-Object System.Diagnostics.Process
$RegistryProcess.StartInfo.FileName = "$SILKitDir\sil-kit-registry.exe"
$RegistryProcess.StartInfo.Arguments = "--listen-uri 'silkit://0.0.0.0:8501'"
$RegistryProcess.StartInfo.UseShellExecute = $false
$RegistryProcess.StartInfo.RedirectStandardOutput = $true
$RegistryProcess.StartInfo.RedirectStandardError = $true

$RunQEMUProcess = New-Object System.Diagnostics.Process
$RunQEMUProcess.StartInfo.FileName = "powershell"
$RunQEMUProcess.StartInfo.Arguments = "$PSScriptRoot\..\..\..\tools\run-silkit-qemu-demos-guest.ps1 | Out-File -FilePath $logDir\run-silkit-qemu-demos-guest_$timestamp.out"
$RunQEMUProcess.StartInfo.UseShellExecute = $false
$RunQEMUProcess.StartInfo.RedirectStandardInput = $true

$AdapterProcess = New-Object System.Diagnostics.Process
$AdapterProcess.StartInfo.FileName = "$PSScriptRoot\..\..\..\bin\sil-kit-adapter-qemu.exe"
$AdapterProcess.StartInfo.Arguments = "--socket-to-chardev localhost:23456,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --configuration $PSScriptRoot\..\SilKitConfig_Adapter.silkit.yaml"
$AdapterProcess.StartInfo.UseShellExecute = $false
$AdapterProcess.StartInfo.RedirectStandardOutput = $true
$AdapterProcess.StartInfo.RedirectStandardError = $true

$DemoProcess = New-Object System.Diagnostics.Process
$DemoProcess.StartInfo.FileName = "$PSScriptRoot\..\..\..\bin\sil-kit-demo-chardev-echo-device.exe"
$DemoProcess.StartInfo.Arguments = "--log Debug"
$DemoProcess.StartInfo.UseShellExecute = $false
$DemoProcess.StartInfo.RedirectStandardOutput = $true
$DemoProcess.StartInfo.RedirectStandardError = $true

# Define the output handler (uses $Event.MessageData for the log file path)
$OutputHandler = {
    param($sending, $data)
    if ($data.Data) {
        Add-Content -Path $Event.MessageData -Value $data.Data
    }
}

Clear-Content -Path $logDir\sil-kit-registry_$timestamp.out -ErrorAction SilentlyContinue
Clear-Content -Path $logDir\sil-kit-adapter-qemu_$timestamp.out -ErrorAction SilentlyContinue
Clear-Content -Path $logDir\sil-kit-demo-chardev-echo-device_$timestamp.out -ErrorAction SilentlyContinue

Register-ObjectEvent -InputObject $RegistryProcess -EventName OutputDataReceived -Action $OutputHandler -MessageData "$logDir\sil-kit-registry_$timestamp.out" | Out-Null
Register-ObjectEvent -InputObject $AdapterProcess -EventName OutputDataReceived -Action $OutputHandler -MessageData "$logDir\sil-kit-adapter-qemu_$timestamp.out" | Out-Null
Register-ObjectEvent -InputObject $DemoProcess -EventName OutputDataReceived -Action $OutputHandler -MessageData "$logDir\sil-kit-demo-chardev-echo-device_$timestamp.out" | Out-Null
Register-ObjectEvent -InputObject $RegistryProcess -EventName ErrorDataReceived -Action $OutputHandler -MessageData "$logDir\sil-kit-registry_$timestamp.out" | Out-Null
Register-ObjectEvent -InputObject $AdapterProcess -EventName ErrorDataReceived -Action $OutputHandler -MessageData "$logDir\sil-kit-adapter-qemu_$timestamp.out" | Out-Null
Register-ObjectEvent -InputObject $DemoProcess -EventName ErrorDataReceived -Action $OutputHandler -MessageData "$logDir\sil-kit-demo-chardev-echo-device_$timestamp.out" | Out-Null

echo "[info] Starting the QEMU image"
# Pre-create the log file so Get-Content -Wait can tail it immediately
New-Item -Path "$logDir\run-silkit-qemu-demos-guest_$timestamp.out" -ItemType File -Force | Out-Null
$out=$RunQEMUProcess.Start()

# Sleep 2 seconds to let QEMU start producing output
Start-Sleep -Seconds 2

# Timeout for starting the QEMU image
$timeout = 180
$startTime = Get-Date
$sentenceFound = $false

# While loop which will break in the ForEach-Object call
while ($true)
{
    # Tail the file and search for the sentence before logging
    Get-Content -Path "$logDir\run-silkit-qemu-demos-guest_$timestamp.out" -Tail 0 -Wait | ForEach-Object {
        if ($_ -match 'Debian GNU/Linux 11 silkit-qemu-demos-guest ttyS0') 
        {
            echo "[info] QEMU image ready to log in"
            $sentenceFound = $true
            break
        }

        # Check the elapsed time
        $elapsedTime = (Get-Date) - $startTime
        if ($elapsedTime.TotalSeconds -ge $timeout) 
        {
            echo "[error] Too long to display logging info on QEMU image"
            break
        }
    }

    # Exit with status 1 if the sentence is not found
    if (-not $sentenceFound) {
        exit 1
    }
}

# Use the input of the RunQEMUProcess to log and then ping
echo "[info] Log in into the QEMU image"
$StdIn = $RunQEMUProcess.StandardInput
$StdIn.WriteLine("root")
Start-Sleep -Seconds 1
$StdIn.WriteLine("root")
Start-Sleep -Seconds 1
echo "[info] Sending data to /dev/ttyS1"
$StdIn.WriteLine("stty -echo raw -F /dev/ttyS1")
Start-Sleep -Seconds 1
$StdIn.WriteLine("while true;\")
$StdIn.WriteLine("do echo test > /dev/ttyS1;\")
$StdIn.WriteLine("sleep 1;\")
$StdIn.WriteLine("done")

# Start the SIL Kit registry
echo "[info] Starting SIL Kit registry"
$out=$RegistryProcess.Start()

# Start recording the SIL Kit registry logs
$RegistryProcess.BeginOutputReadLine()
$RegistryProcess.BeginErrorReadLine()

Start-Sleep -Seconds 2

# Start the adapter and the demo
echo "[info] Starting the adapter and the demo"
$out=$AdapterProcess.Start()
$out=$DemoProcess.Start()

# Start recording the adapter and the demo logs
$AdapterProcess.BeginOutputReadLine()
$AdapterProcess.BeginErrorReadLine()
$DemoProcess.BeginOutputReadLine()
$DemoProcess.BeginErrorReadLine()

echo "[info] Starting run.ps1 test script"
# Get the last line telling the overall test verdict (passed/failed)
$scriptResult = & $PSScriptRoot\run.ps1 | Select-Object -Last 1

$isPassed = select-string -pattern "passed" -InputObject $scriptResult

echo "[info] Stopping the QEMU image"
$StdIn.WriteLine("shutdown now")

# Stop the started processes
Stop-Process -Id $RegistryProcess.Id
Stop-Process -Id $DemoProcess.Id
Stop-Process -Id $AdapterProcess.Id
Get-Process qemu-system-x86_64 | Stop-Process

# Unregister all event handlers
Get-EventSubscriber | Unregister-Event -Force
Remove-Job * -Force

if($StdIn) 
{
    $StdIn.Close()
}

if($isPassed)
{
    Write-Output "Tests passed"
    exit 0
}
else
{
    Write-Output "Tests failed"
    exit 1
}
