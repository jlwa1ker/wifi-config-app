[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$Name,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$Interface,

    [int]$TimeoutSeconds = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-Netsh {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    $output = & netsh @Arguments 2>&1
    [pscustomobject]@{
        ExitCode = $LASTEXITCODE
        Output   = $output
    }
}

function Test-WlanProfileExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProfileName,

        [Parameter(Mandatory = $true)]
        [string]$InterfaceName
    )

    $result = Invoke-Netsh -Arguments @(
        'wlan', 'show', 'profiles',
        "name=$ProfileName",
        "interface=$InterfaceName"
    )

    return ($result.ExitCode -eq 0 -and ($result.Output -join "`n") -match 'Profile information')
}

function New-OpenWlanProfile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Ssid
    )

    $settings = [System.Xml.XmlWriterSettings]::new()
    $settings.OmitXmlDeclaration = $false
    $settings.Encoding = [System.Text.UTF8Encoding]::new($false)
    $settings.Indent = $true

    $builder = [System.Text.StringBuilder]::new()
    $writer = [System.Xml.XmlWriter]::Create($builder, $settings)

    $writer.WriteStartDocument()
    $writer.WriteStartElement('WLANProfile', 'http://www.microsoft.com/networking/WLAN/profile/v1')
    $writer.WriteElementString('name', $Ssid)
    $writer.WriteStartElement('SSIDConfig')
    $writer.WriteStartElement('SSID')
    $writer.WriteElementString('name', $Ssid)
    $writer.WriteEndElement()
    $writer.WriteEndElement()
    $writer.WriteElementString('connectionType', 'ESS')
    $writer.WriteElementString('connectionMode', 'manual')
    $writer.WriteStartElement('MSM')
    $writer.WriteStartElement('security')
    $writer.WriteStartElement('authEncryption')
    $writer.WriteElementString('authentication', 'open')
    $writer.WriteElementString('encryption', 'none')
    $writer.WriteElementString('useOneX', 'false')
    $writer.WriteEndElement()
    $writer.WriteEndElement()
    $writer.WriteEndElement()
    $writer.WriteEndElement()
    $writer.WriteEndDocument()
    $writer.Dispose()

    return $builder.ToString()
}

function Add-OpenWlanProfile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Ssid,

        [Parameter(Mandatory = $true)]
        [string]$InterfaceName
    )

    $profilePath = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath ("open-wlan-profile-{0}.xml" -f ([guid]::NewGuid()))

    try {
        New-OpenWlanProfile -Ssid $Ssid | Set-Content -LiteralPath $profilePath -Encoding utf8

        $result = Invoke-Netsh -Arguments @(
            'wlan', 'add', 'profile',
            "filename=$profilePath",
            "interface=$InterfaceName",
            'user=current'
        )

        if ($result.ExitCode -ne 0) {
            throw "Failed to add open WLAN profile for SSID '$Ssid'. netsh output:`n$($result.Output -join "`n")"
        }
    }
    finally {
        if (Test-Path -LiteralPath $profilePath) {
            Remove-Item -LiteralPath $profilePath -Force
        }
    }
}

function Connect-WlanProfile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProfileName,

        [string]$Ssid,

        [Parameter(Mandatory = $true)]
        [string]$InterfaceName
    )

    $arguments = @(
        'wlan', 'connect',
        "name=$ProfileName",
        "interface=$InterfaceName"
    )

    if (-not [string]::IsNullOrWhiteSpace($Ssid)) {
        $arguments += "ssid=$Ssid"
    }

    $result = Invoke-Netsh -Arguments $arguments

    if ($result.ExitCode -ne 0) {
        throw "Failed to initiate connection using profile '$ProfileName' on interface '$InterfaceName'. netsh output:`n$($result.Output -join "`n")"
    }
}

function Get-WlanInterfaceState {
    param(
        [Parameter(Mandatory = $true)]
        [string]$InterfaceName
    )

    $result = Invoke-Netsh -Arguments @('wlan', 'show', 'interfaces')
    if ($result.ExitCode -ne 0) {
        throw "Failed to query WLAN interfaces. netsh output:`n$($result.Output -join "`n")"
    }

    $currentName = $null
    $currentState = $null

    foreach ($line in $result.Output) {
        if ($line -match '^\s*Name\s*:\s*(.+?)\s*$') {
            if ($null -ne $currentName -and $currentName -eq $InterfaceName) {
                return $currentState
            }

            $currentName = $Matches[1]
            $currentState = $null
            continue
        }

        if ($null -ne $currentName -and $line -match '^\s*State\s*:\s*(.+?)\s*$') {
            $currentState = $Matches[1]
        }
    }

    if ($null -ne $currentName -and $currentName -eq $InterfaceName) {
        return $currentState
    }

    return $null
}

function Wait-WlanConnected {
    param(
        [Parameter(Mandatory = $true)]
        [string]$InterfaceName,

        [Parameter(Mandatory = $true)]
        [int]$TimeoutSeconds
    )

    $deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)

    do {
        $state = Get-WlanInterfaceState -InterfaceName $InterfaceName
        if ($null -ne $state -and $state.Trim().ToLowerInvariant() -eq 'connected') {
            return $true
        }

        Start-Sleep -Seconds 1
    } while ([DateTimeOffset]::UtcNow -lt $deadline)

    return $false
}

try {
    $profileExists = Test-WlanProfileExists -ProfileName $Name -InterfaceName $Interface

    if (-not $profileExists) {
        Add-OpenWlanProfile -Ssid $Name -InterfaceName $Interface
        Connect-WlanProfile -ProfileName $Name -Ssid $Name -InterfaceName $Interface
    }
    else {
        Connect-WlanProfile -ProfileName $Name -InterfaceName $Interface
    }

    if (Wait-WlanConnected -InterfaceName $Interface -TimeoutSeconds $TimeoutSeconds) {
        Write-Host "Interface '$Interface' is connected."
        exit 0
    }

    Write-Error "Timed out after $TimeoutSeconds seconds waiting for interface '$Interface' to become connected."
    exit 1
}
catch {
    Write-Error $_
    exit 1
}
