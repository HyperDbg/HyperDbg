#
# hyperdbg-usb-install.ps1
#
# Install the WinUSB driver package on the debugger.
#
# Run in PowerShell as an administrator:
#   powershell -ExecutionPolicy Bypass -File hyperdbg-usb-install.ps1
#   powershell -ExecutionPolicy Bypass -File hyperdbg-usb-install.ps1 -Uninstall
#
# Windows uses the signed WinUSB driver but still needs a signed catalog.
# The script creates or reuses a local signing certificate.
# It builds the catalog in a temporary folder with Inf2Cat or New-FileCatalog.
# It signs the catalog and trusts the certificate for the local machine.
# It then installs the package with pnputil.
#
# -Uninstall removes the package and the certificate.
#

param([switch]$Uninstall)

$ErrorActionPreference = 'Stop'
$InfName  = 'hyperdbg-usb.inf'
$CatName  = 'hyperdbg-usb.cat'
$Subject  = 'CN=HyperDbg USB WinUSB (local)'
$InfPath  = Join-Path $PSScriptRoot $InfName

$Identity = [Security.Principal.WindowsIdentity]::GetCurrent()
if (-not (New-Object Security.Principal.WindowsPrincipal $Identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error 'Run this script from PowerShell as an administrator.' -ErrorAction Continue
    return
}

function Get-InstalledPackages {
    #
    # Find the published INF names for this package
    #
    $Names   = @()
    $Current = $null
    foreach ($Line in (pnputil /enum-drivers)) {
        if ($Line -match '^\s*Published Name\s*:\s*(\S+)') { $Current = $Matches[1] }
        elseif ($Line -match '^\s*Original Name\s*:\s*(\S+)' -and $Matches[1] -ieq $InfName) { $Names += $Current }
    }
    return $Names
}

if ($Uninstall) {
    foreach ($Name in Get-InstalledPackages) {
        pnputil /delete-driver $Name /uninstall /force | Out-Host
    }
    foreach ($Store in 'My', 'Root', 'TrustedPublisher') {
        Get-ChildItem "Cert:\LocalMachine\$Store" | Where-Object { $_.Subject -eq $Subject } | Remove-Item
    }
    Write-Host 'Removed the HyperDbg USB driver package and certificate.'
    return
}

#
# 1. Certificate
#
$Cert = Get-ChildItem Cert:\LocalMachine\My | Where-Object { $_.Subject -eq $Subject -and $_.NotAfter -gt (Get-Date) } | Select-Object -First 1
if (-not $Cert) {
    $Cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject $Subject `
        -CertStoreLocation Cert:\LocalMachine\My -NotAfter (Get-Date).AddYears(10)
}

#
# 2. Build the catalog in a temporary folder
#
$Stage = Join-Path $env:TEMP ('hyperdbg-usb-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $Stage | Out-Null
try {
    Copy-Item $InfPath $Stage
    $StagedInf = Join-Path $Stage $InfName
    $StagedCat = Join-Path $Stage $CatName

    $Inf2Cat = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x86\Inf2Cat.exe" -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending | Select-Object -First 1
    if ($Inf2Cat) {
        & $Inf2Cat.FullName /driver:$Stage /os:10_X64 /uselocaltime | Out-Host
    }
    else {
        New-FileCatalog -Path $StagedInf -CatalogFilePath $StagedCat -CatalogVersion 2 | Out-Null
    }
    if (-not (Test-Path $StagedCat)) {
        Write-Error 'Unable to create the driver catalog.' -ErrorAction Continue
        return
    }

    #
    # 3. Sign the catalog and trust the certificate
    #
    $Signature = Set-AuthenticodeSignature -FilePath $StagedCat -Certificate $Cert -HashAlgorithm SHA256
    if ($Signature.Status -ne 'Valid' -and $Signature.Status -ne 'UnknownError') {
        Write-Error "Unable to sign the driver catalog: $($Signature.StatusMessage)" -ErrorAction Continue
        return
    }
    $PublicCert = Join-Path $Stage 'hyperdbg-usb.cer'
    Export-Certificate -Cert $Cert -FilePath $PublicCert | Out-Null
    Import-Certificate -FilePath $PublicCert -CertStoreLocation Cert:\LocalMachine\Root | Out-Null
    Import-Certificate -FilePath $PublicCert -CertStoreLocation Cert:\LocalMachine\TrustedPublisher | Out-Null

    #
    # 4. Install
    #
    pnputil /add-driver $StagedInf /install | Out-Host
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 3010) {
        Write-Error "pnputil failed with exit code $LASTEXITCODE." -ErrorAction Continue
        return
    }
}
finally {
    Remove-Item $Stage -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host 'Installed the HyperDbg USB driver package.'
