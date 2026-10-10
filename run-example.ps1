param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidatePattern('^[A-Za-z0-9_-]+$')]
    [string]$Example,

    [Parameter()]
    [string]$VicePath
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$source = Join-Path $root "examples\$Example.c"
$compiler = Join-Path $root 'build\c64cc.exe'
$outputDirectory = Join-Path $root 'build'
$program = Join-Path $outputDirectory "$Example.prg"
$assembly = Join-Path $outputDirectory "$Example.asm"
$includeDirectory = Join-Path $root 'include'

if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Example not found: $source"
}
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "Compiler not found: $compiler. Build it with cmake --build build first."
}

if (-not $VicePath) {
    if ($env:C64_VICE_PATH) {
        $VicePath = $env:C64_VICE_PATH
    } elseif (Test-Path -LiteralPath (Join-Path $root 'x64sc.exe') -PathType Leaf) {
        $VicePath = Join-Path $root 'x64sc.exe'
    } else {
        $viceCommand = Get-Command 'x64sc.exe' -ErrorAction SilentlyContinue
        if (-not $viceCommand) {
            throw "VICE not found. Pass -VicePath 'C:\path\to\x64sc.exe' or set C64_VICE_PATH."
        }
        $VicePath = $viceCommand.Source
    }
} elseif (-not [System.IO.Path]::IsPathRooted($VicePath)) {
    $localVice = Join-Path $root $VicePath
    if (Test-Path -LiteralPath $localVice -PathType Leaf) {
        $VicePath = $localVice
    }
}

if (-not (Test-Path -LiteralPath $VicePath -PathType Leaf)) {
    throw "VICE executable not found: $VicePath"
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
& $compiler $source -I $includeDirectory -S $assembly -o $program --run --vice $VicePath
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}