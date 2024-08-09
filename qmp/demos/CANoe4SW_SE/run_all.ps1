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

# Create the log directory
if (-not (Test-Path -Path $PSScriptRoot/logs))
{
    mkdir $PSScriptRoot/logs | Out-Null
}

# Processes to run the executables and commands in background
$RegistryProcess = New-Object System.Diagnostics.Process
$RegistryProcess.StartInfo.FileName = "$SILKitDir\sil-kit-registry.exe"
$RegistryProcess.StartInfo.Arguments = "--listen-uri 'silkit://0.0.0.0:8501' -s"
$RegistryProcess.StartInfo.UseShellExecute = $false
$RegistryProcess.StartInfo.RedirectStandardOutput = $true

$RunQEMUProcess = New-Object System.Diagnostics.Process
$RunQEMUProcess.StartInfo.FileName = "powershell"
$RunQEMUProcess.StartInfo.Arguments = "$PSScriptRoot\..\..\..\tools\run-silkit-qemu-demos-guest.ps1 | Out-File -FilePath $PSScriptRoot\logs\run-silkit-qemu-demos-guest.out"
$RunQEMUProcess.StartInfo.UseShellExecute = $false
$RunQEMUProcess.StartInfo.RedirectStandardInput = $true

$AdapterProcess = New-Object System.Diagnostics.Process
$AdapterProcess.StartInfo.FileName = "$PSScriptRoot\..\..\..\bin\SilKitAdapterQemu.exe"
$AdapterProcess.StartInfo.Arguments = "--socket-to-ethernet localhost:12345,network=qemu_demo --socket-to-chardev localhost:4444,Namespace::toQMP,VirtualNetwork=Default,Instance=CANoe,Namespace::fromQMP,VirtualNetwork:Default,Instance:Adapter --configuration $PSScriptRoot\..\SilKitConfig_Adapter.silkit.yaml"
$AdapterProcess.StartInfo.UseShellExecute = $false
$AdapterProcess.StartInfo.RedirectStandardOutput = $true

# Define all the output handlers
$RegistryOutputHandler = {
    param($sending, $data)
    if ($data.Data) {
        Add-Content -Path "$PSScriptRoot\logs\sil-kit-registry.out" -Value $data.Data
    }
}

$AdapterOutputHandler = {
    param($sending, $data)
    if ($data.Data) {
        Add-Content -Path "$PSScriptRoot\logs\SilKitAdapterQemu.out" -Value $data.Data
    }
}

Clear-Content -Path $PSScriptRoot\logs\sil-kit-registry.out -ErrorAction SilentlyContinue
Clear-Content -Path $PSScriptRoot\logs\SilKitAdapterQemu.out -ErrorAction SilentlyContinue

Register-ObjectEvent -InputObject $RegistryProcess -EventName OutputDataReceived -Action $RegistryOutputHandler | Out-Null
Register-ObjectEvent -InputObject $AdapterProcess -EventName OutputDataReceived -Action $AdapterOutputHandler | Out-Null

echo "[info] Starting the QEMU image"
$out=$RunQEMUProcess.Start()

# Sleep 1 second to create the output file
Start-Sleep -Seconds 1

# Timeout for starting the QEMU image
$timeout = 180
$startTime = Get-Date
$sentenceFound = $false

# While loop which will break in the ForEach-Object call
while ($true)
{
    # Tail the file and search for the sentence before logging
    Get-Content -Path "$PSScriptRoot\logs\run-silkit-qemu-demos-guest.out" -Tail 0 -Wait | ForEach-Object {
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
echo "[Info] Ping 192.168.7.34" 
$StdIn.WriteLine("ping 192.168.7.34")

# Start the SIL Kit registry
echo "[info] Starting SIL Kit registry"
$out=$RegistryProcess.Start()

# Start recording the SIL Kit registry logs
$RegistryProcess.BeginOutputReadLine()

Start-Sleep -Seconds 2

# Start the adapter and the demo
echo "[info] Starting the adapter"
$out=$AdapterProcess.Start()

# Start recording the adapter logs
$AdapterProcess.BeginOutputReadLine()

echo "[info] Starting run.ps1 test script"
# Get the last line telling the overall test verdict (passed/failed)
$scriptResult = & $PSScriptRoot\run.ps1 | Select-Object -Last 1

$isPassed = select-string -pattern "passed" -InputObject $scriptResult

echo "[info] Stopping the QEMU image"
$StdIn.WriteLine("shutdown now")

# Stop the started processes
Stop-Process -Id $RegistryProcess.Id
Stop-Process -Id $AdapterProcess.Id
Get-Process qemu-system-x86_64 | Stop-Process

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
