# Original tooling: MIT, copyright 2026 35mmIsle contributors.
# Run in PowerShell 7 on Windows before the first host CMake configure.
# No bootstrap is executed: the release URL is mutable, its accepted bytes are not.
# Sources: microsoft/vcpkg@4da8ab8ef910293046e651287ff1115d9a4f14b0
# scripts/vcpkg-tool-metadata.txt; vcpkg-tool release 2026-03-04, asset 367069288.
[CmdletBinding()]
param(
    [string] $Root = (Join-Path $PSScriptRoot '../build-dependencies/windows-host-vcpkg')
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows -or -not [Environment]::Is64BitProcess) {
    throw 'Windows host provisioning requires 64-bit PowerShell 7 on Windows.'
}
$pin = '4da8ab8ef910293046e651287ff1115d9a4f14b0'
$toolHash = '13a1c66b9c7578427b3eda7eba2332b73d4fb86706e053ad6426dad2f354cbc3'
$Root = [IO.Path]::GetFullPath($Root)
$marker = "$Root.35mmisle-owned"
$ownership = "35mmIsle Windows host vcpkg $pin"
# Do not accept ambient overlays, chainloaded toolchains or unreviewed binary caches.
foreach ($name in @('VCPKG_OVERLAY_PORTS', 'VCPKG_OVERLAY_TRIPLETS', 'VCPKG_CHAINLOAD_TOOLCHAIN_FILE')) {
    if ([Environment]::GetEnvironmentVariable($name)) { throw "Unset $name before provisioning." }
}
function Invoke-Git {
    $result = & git @args
    if ($LASTEXITCODE -ne 0) { throw "git failed: $args" }
    return $result
}
if (Test-Path -LiteralPath $Root) {
    if (-not (Test-Path -LiteralPath $marker -PathType Leaf) -or
        (Get-Content -LiteralPath $marker -Raw).Trim() -cne $ownership) {
        throw 'Refusing an existing checkout not owned by this provisioning script.'
    }
    if ((Get-Item -LiteralPath $Root).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw 'Refusing a redirected vcpkg checkout.'
    }
} else {
    if (Test-Path -LiteralPath $marker) { throw 'Stale ownership marker; choose a fresh root.' }
    New-Item -ItemType Directory -Path $Root -Confirm:$false | Out-Null
    # Write ownership before acquisition so an interrupted owned checkout can be inspected.
    Set-Content -LiteralPath $marker -Value $ownership -NoNewline -Confirm:$false
    Invoke-Git init $Root | Out-Host
    Invoke-Git -C $Root remote add origin https://github.com/microsoft/vcpkg
    Invoke-Git -C $Root fetch --depth=1 origin $pin | Out-Host
    Invoke-Git -C $Root -c core.hooksPath=NUL checkout --detach $pin | Out-Host
}
# Never reset, pull, clean or repair an existing checkout, even one we own.
if ((Invoke-Git -C $Root rev-parse HEAD) -cne $pin) { throw 'vcpkg source pin mismatch.' }
if ((Invoke-Git -C $Root remote get-url origin) -cne 'https://github.com/microsoft/vcpkg') {
    throw 'vcpkg source origin mismatch.'
}
if (Invoke-Git -C $Root status --porcelain --untracked-files=all) {
    throw 'vcpkg source checkout is not clean; refusing to modify it.'
}
$exe = Join-Path $Root 'vcpkg.exe'
if (-not (Test-Path -LiteralPath $exe)) {
    $download = Join-Path $Root ('.35mmisle-tool-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    try {
        Invoke-WebRequest -Uri 'https://github.com/microsoft/vcpkg-tool/releases/download/2026-03-04/vcpkg.exe' -OutFile $download
        if ((Get-Item -LiteralPath $download).Length -ne 6591048 -or
            (Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash -ine $toolHash) {
            throw 'Downloaded vcpkg.exe failed pinned size/SHA256 verification.'
        }
        Move-Item -LiteralPath $download -Destination $exe -Confirm:$false
    } finally {
        if (Test-Path -LiteralPath $download) { Remove-Item -LiteralPath $download -Confirm:$false }
    }
}
if ((Get-Item -LiteralPath $exe).Length -ne 6591048 -or
    (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -ine $toolHash) {
    throw 'Existing vcpkg.exe failed pinned size/SHA256 verification; it was not executed or replaced.'
}
# CMake's pinned toolchain executes this verified tool during project(), not bootstrap.
$environment = @{
    ISLE_WINDOWS_VCPKG_ROOT = $Root
    VCPKG_ROOT = $Root
    VCPKG_DISABLE_METRICS = '1'
    VCPKG_BINARY_SOURCES = 'clear'
}
# MSBuild discovers tools independently; recipient-side native LINK needs the
# actual developer environment. Export its selected tool/SDK paths by the same
# mechanism as the verified vcpkg root, never guessed include/library paths.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$visualStudio = @(& $vswhere -latest -version '[17.0,18.0)' -products '*' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
if ($LASTEXITCODE -ne 0 -or $visualStudio.Count -ne 1) {
    throw 'Cannot locate the required native VS2022 x64 toolchain.'
}
$developerShell = Join-Path $visualStudio[0] 'Common7/Tools/VsDevCmd.bat'
$developerEnvironment = @(& $env:ComSpec /d /s /c "`"$developerShell`" -no_logo -arch=x64 -host_arch=x64 >nul && set")
if ($LASTEXITCODE -ne 0) { throw 'VS2022 x64 developer environment initialization failed.' }
$selectedVariables = @('Path', 'INCLUDE', 'LIB', 'LIBPATH', 'VCToolsInstallDir',
    'VCToolsVersion', 'VSINSTALLDIR', 'VCINSTALLDIR', 'VisualStudioVersion',
    'WindowsSdkDir', 'WindowsSDKVersion', 'WindowsSdkBinPath', 'WindowsSdkVerBinPath',
    'WindowsSDKLibVersion', 'UniversalCRTSdkDir', 'UCRTVersion',
    'VSCMD_ARG_HOST_ARCH', 'VSCMD_ARG_TGT_ARCH')
foreach ($line in $developerEnvironment) {
    if ($line -match '^([^=]+)=(.*)$' -and $selectedVariables -contains $Matches[1]) {
        $environment[$Matches[1]] = $Matches[2]
    }
}
foreach ($name in @('Path', 'INCLUDE', 'LIB', 'VCToolsInstallDir', 'WindowsSdkDir')) {
    if (-not $environment.ContainsKey($name) -or -not $environment[$name]) {
        throw "VS2022 x64 developer environment omitted $name."
    }
}
foreach ($entry in $environment.GetEnumerator()) {
    [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process')
    if ($env:GITHUB_ENV) {
        if ($entry.Value -match '[\r\n]') { throw 'Invalid multiline environment value.' }
        Add-Content -LiteralPath $env:GITHUB_ENV -Value "$($entry.Key)=$($entry.Value)" -Confirm:$false
    }
}
Write-Host "Verified Windows host source/tool; root=$Root; triplet=x64-windows-static; metrics disabled."
Write-Host 'Next in this PowerShell: cmake --preset windows-host-release'
