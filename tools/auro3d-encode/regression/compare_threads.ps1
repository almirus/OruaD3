param(
    [string]$InputFile = "test_files\correlation 2\01. Nature's Kiss 7.1.4.wav",
    [string]$InputChannelOrder = '',
    [double]$Seconds = 2.0,
    [int]$ParallelThreads = [Math]::Min([Environment]::ProcessorCount, 8)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$encoder = Join-Path $repo 'bin\Release\orua3d-encode.exe'
$input = if ([IO.Path]::IsPathRooted($InputFile)) {
    [IO.Path]::GetFullPath($InputFile)
} else {
    [IO.Path]::GetFullPath((Join-Path $repo $InputFile))
}
$tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temp = Join-Path $tempRoot ('orua3d-thread-regression-' + [Guid]::NewGuid().ToString('N'))

if ($ParallelThreads -lt 2 -or $ParallelThreads -gt 256) {
    throw 'ParallelThreads must be in 2..256'
}
if (-not [IO.File]::Exists($encoder)) {
    throw "encoder executable not found: $encoder"
}
if (-not [IO.File]::Exists($input)) {
    throw "input file not found: $input"
}
if (-not $temp.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'temporary regression path escaped the system temporary directory'
}

function Invoke-Encoding(
    [string]$Source,
    [string]$Output,
    [string[]]$ThreadArguments
) {
    $arguments = @('-i', $Source, '--output', $Output, '--seed', '305419896')
    if ($InputChannelOrder) {
        $arguments += @('--input-channel-order', $InputChannelOrder)
    }
    $arguments += $ThreadArguments
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $encoder @arguments 2>&1 | Out-Null
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $oldPreference
    }
    $timer.Stop()
    if ($code -ne 0) {
        throw "encoder failed with exit code $code for: $($ThreadArguments -join ' ')"
    }
    return $timer.Elapsed.TotalSeconds
}

New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $source = Join-Path $temp 'source.wav'
    & ffmpeg -hide_banner -loglevel error -y -i $input -t $Seconds `
        -map 0:a:0 -c:a pcm_s24le $source
    if ($LASTEXITCODE -ne 0) {
        throw "ffmpeg failed to prepare the $Seconds-second regression source"
    }

    $auto = Join-Path $temp 'auto.wav'
    $serial = Join-Path $temp 'threads-1.wav'
    $parallel = Join-Path $temp "threads-$ParallelThreads.wav"
    $autoSeconds = Invoke-Encoding $source $auto @()
    $serialSeconds = Invoke-Encoding $source $serial @('--threads', '1')
    $parallelSeconds = Invoke-Encoding `
        $source $parallel @('--threads', [string]$ParallelThreads)

    $autoHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $auto).Hash
    $serialHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $serial).Hash
    $parallelHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $parallel).Hash
    if ($autoHash -ne $serialHash -or $autoHash -ne $parallelHash) {
        throw "thread regression mismatch: auto=$autoHash serial=$serialHash parallel=$parallelHash"
    }

    Write-Output 'OK: auto, --threads 1 and explicit parallel output are byte-identical'
    Write-Output "sha256=$autoHash"
    Write-Output ('seconds auto={0:F3} serial={1:F3} parallel={2:F3} speedup={3:F2}x' -f `
        $autoSeconds, $serialSeconds, $parallelSeconds,
        ($serialSeconds / $parallelSeconds))
} finally {
    if ([IO.Directory]::Exists($temp)) {
        Remove-Item -LiteralPath $temp -Recurse -Force
    }
}
