#requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('DEBUG', 'RELEASE')][string]$Target = 'DEBUG',
    [string]$Edk2,
    [switch]$Bootstrap
)
$ErrorActionPreference = 'Stop'
$script = Join-Path $PSScriptRoot 'edk2.py'
$edkArgs = @()
if ($Edk2) { $edkArgs = @('--edk2', $Edk2) }
if ($Bootstrap) {
    & python $script bootstrap @edkArgs
    if ($LASTEXITCODE -ne 0) { throw "BaseTools bootstrap failed ($LASTEXITCODE)" }
}
& python $script build --target $Target @edkArgs
if ($LASTEXITCODE -ne 0) { throw "UEFI build failed ($LASTEXITCODE)" }
