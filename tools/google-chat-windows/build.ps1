[CmdletBinding()]
param(
    [ValidateSet('win-x64','win-arm64')][string]$Runtime = 'win-x64',
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'dist'),
    [string]$OAuthConfig = '',
    [switch]$RequireOAuth
)
$ErrorActionPreference = 'Stop'
if ([System.Environment]::OSVersion.Platform -ne [System.PlatformID]::Win32NT) { throw 'Run build.ps1 on Windows. Core tests and cross-compilation can also run on macOS/Linux.' }
function Invoke-Dotnet([string[]]$Arguments) {
    & dotnet @Arguments
    if ($LASTEXITCODE -ne 0) { throw 'dotnet build/check failed.' }
}
$repo = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$versionText = Get-Content (Join-Path $repo 'MeteoPlaneRadar/Version.h') -Raw
$versionMatch = [regex]::Match($versionText, 'FW_VERSION "([^"]+)"')
if (-not $versionMatch.Success) { throw 'Missing FW_VERSION.' }
$version = $versionMatch.Groups[1].Value
if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) { throw 'Install the .NET 10 SDK first.' }
if (-not $OAuthConfig) {
    $candidate = Join-Path $PSScriptRoot 'config/google-oauth.json'
    if (Test-Path $candidate) { $OAuthConfig = $candidate }
}
if ($OAuthConfig -and -not (Test-Path $OAuthConfig -PathType Leaf)) { throw 'OAuthConfig file does not exist.' }
if ($RequireOAuth -and -not $OAuthConfig) { throw 'A configured Desktop app OAuth client is required.' }
$temporary = Join-Path ([System.IO.Path]::GetTempPath()) ('meteo-chat-build-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $temporary | Out-Null
try {
    Invoke-Dotnet -Arguments @('run','--project',(Join-Path $PSScriptRoot 'Checks/Checks.csproj'),'-c','Release','--no-launch-profile')
    $publish = @('publish',(Join-Path $PSScriptRoot 'MeteoChat/MeteoChat.csproj'),'-c','Release','-r',$Runtime,
        '--self-contained','true','-p:PublishSingleFile=true','-p:IncludeNativeLibrariesForSelfExtract=true',
        '-p:EnableCompressionInSingleFile=true','-o',(Join-Path $temporary 'publish'))
    if ($OAuthConfig) {
        $sanitized = Join-Path $temporary 'GoogleOAuth.json'
        Invoke-Dotnet -Arguments @('run','--project',(Join-Path $PSScriptRoot 'Checks/Checks.csproj'),'-c','Release',
            '--no-launch-profile','--','--export-oauth',([System.IO.Path]::GetFullPath($OAuthConfig)),$sanitized)
        $publish += ('-p:OAuthConfig=' + $sanitized)
    }
    Invoke-Dotnet -Arguments @('run','--project',(Join-Path $PSScriptRoot 'MeteoChat/MeteoChat.csproj'),'-c','Release','--no-launch-profile','--','--self-test')
    Invoke-Dotnet -Arguments $publish
    $app = Join-Path $temporary 'publish/MeteoChat.exe'
    # Windows-only native checks, without signing into Google or a real display.
    $hostRuntime = 'win-' + [System.Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture.ToString().ToLowerInvariant()
    if ($Runtime -eq $hostRuntime) {
        $nativeCheck = Start-Process -FilePath $app -ArgumentList '--self-test' -PassThru -Wait
        if ($nativeCheck.ExitCode -ne 0) { throw ('Published application self-test failed: ' + $nativeCheck.ExitCode) }
    }
    New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
    $zip = Join-Path $OutputDirectory ('Meteo-Chat-' + $version + '-' + $Runtime + '.zip')
    $readme = Join-Path $PSScriptRoot 'README.md'
    Copy-Item -LiteralPath $readme -Destination (Join-Path $temporary 'publish/README.md')
    Copy-Item -LiteralPath (Join-Path $repo 'LICENSE.txt') -Destination (Join-Path $temporary 'publish/LICENSE.txt')
    $licenseOutput = Join-Path $temporary 'publish/licenses'
    New-Item -ItemType Directory -Force -Path $licenseOutput | Out-Null
    $assets = Get-Content (Join-Path $PSScriptRoot 'MeteoChat/obj/project.assets.json') -Raw | ConvertFrom-Json
    foreach ($entry in $assets.libraries.PSObject.Properties) {
        if ($entry.Name -notmatch '^Microsoft\.(NETCore|WindowsDesktop)\.App\.Runtime\.') { continue }
        foreach ($folder in $assets.packageFolders.PSObject.Properties.Name) {
            $package = Join-Path $folder $entry.Value.path
            if (-not (Test-Path $package)) { continue }
            foreach ($notice in (Get-ChildItem -LiteralPath $package -File | Where-Object { $_.Name -match '(?i)license|third.?party' })) {
                Copy-Item -LiteralPath $notice.FullName -Destination (Join-Path $licenseOutput (($entry.Name -replace '[/\\]','-') + '-' + $notice.Name))
            }
        }
    }
    Compress-Archive -Path (Join-Path $temporary 'publish/*') -DestinationPath $zip -Force
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $zip).Hash.ToLowerInvariant()
    Set-Content -Path ($zip + '.sha256') -Value ($hash + '  ' + [System.IO.Path]::GetFileName($zip)) -Encoding Ascii
    Write-Host ('Built and checked: ' + $zip)
} finally {
    # This is only the unique temporary directory created above.
    Remove-Item -LiteralPath $temporary -Recurse -Force
}
