<#
.SYNOPSIS
    Configures a freshly-booted tempmon device in AP mode via Wi-Fi 2.

.DESCRIPTION
    1. Connects Wi-Fi 2 to the device's AP (tempmon)
    2. Waits up to 45 seconds for the connection to establish
    3. Submits the configuration form to the device at http://192.168.1.1

.NOTES
    Credentials are read from environment variables:
        TEMPMON_SSID        - WiFi network name to configure on the device
        TEMPMON_PASSWORD    - WiFi password
        TEMPMON_LOCATION    - Device location label

    Alternatively, create scripts/config.local.ps1 (gitignored) with:
        $env:TEMPMON_SSID     = "YourSSID"
        $env:TEMPMON_PASSWORD = "YourPassword"
        $env:TEMPMON_LOCATION = "YourLocation"
#>

# Load local config if present (gitignored)
$localConfig = Join-Path $PSScriptRoot "config.local.ps1"
if (Test-Path $localConfig) {
    . $localConfig
}

# Validate credentials
$ssid     = $env:TEMPMON_SSID
$password = $env:TEMPMON_PASSWORD
$location = $env:TEMPMON_LOCATION

if (-not $ssid -or -not $password -or -not $location) {
    Write-Error "Missing credentials. Set TEMPMON_SSID, TEMPMON_PASSWORD, and TEMPMON_LOCATION environment variables, or create scripts/config.local.ps1."
    exit 1
}

# Step 1: Connect Wi-Fi 2 to the device AP
Write-Host "Connecting Wi-Fi 2 to tempmon AP..."
$connectResult = netsh wlan connect name=tempmon interface="Wi-Fi 2" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "netsh wlan connect failed: $connectResult"
    exit 1
}
Write-Host "Connect command accepted. Waiting for association..."

# Step 2: Poll until connected or timeout (45 seconds)
$deadline = [DateTime]::UtcNow.AddSeconds(45)
$connected = $false

while ([DateTime]::UtcNow -lt $deadline) {
    Start-Sleep -Seconds 2
    $status = netsh wlan show interfaces interface="Wi-Fi 2" 2>&1
    if ($status -match 'SSID\s*:\s*tempmon' -and $status -match 'State\s*:\s*connected') {
        $connected = $true
        Write-Host "Connected to tempmon AP."
        break
    }
}

if (-not $connected) {
    Write-Error "Timed out waiting for Wi-Fi 2 to connect to tempmon after 45 seconds."
    exit 1
}

# Step 3: Give the device a moment to be ready
Start-Sleep -Seconds 2

# Step 4: Submit the configuration form
Write-Host "Submitting device configuration..."
$deviceUrl = "http://10.0.0.9/submit"
$body = "ssid=$([Uri]::EscapeDataString($ssid))&password=$([Uri]::EscapeDataString($password))&location=$([Uri]::EscapeDataString($location))"

try {
    # Use curl.exe directly to avoid .NET TLS issues on captive-portal networks
    $curlResult = & curl.exe -s -o NUL -w "%{http_code}" `
        -X POST `
        -H "Content-Type: application/x-www-form-urlencoded" `
        -d $body `
        --connect-timeout 10 `
        $deviceUrl 2>&1

    if ($curlResult -match "200") {
        Write-Host "Configuration submitted successfully."
        Write-Host "Device is rebooting to connect to $ssid..."
    } else {
        Write-Error "Device returned unexpected status: $curlResult"
        exit 1
    }
} catch {
    Write-Error "Failed to submit configuration: $_"
    exit 1
}
