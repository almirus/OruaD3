$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$files = Get-ChildItem $root -Recurse -File
$textExtensions = '.cpp', '.hpp', '.md', '.ps1', '.bat', '.txt'
$textFiles = $files | Where-Object { $_.Extension -in $textExtensions }
$trailing = $textFiles | Select-String -Pattern '[ \t]+$'
if ($trailing) {
    $trailing | Select-Object -First 20 | Format-Table -AutoSize
    throw 'trailing whitespace found'
}
$markerPattern = ('TO' + 'DO|' + 'FIX' + 'ME|' + ('<' * 7) + '|' + ('>' * 7))
$markers = $textFiles | Select-String -Pattern $markerPattern
if ($markers) {
    $markers | Select-Object -First 20 | Format-Table -AutoSize
    throw 'unfinished or conflict marker found'
}
$forbiddenPattern = ('Auro' + 'CX|auro' + 'cx|Auro-' + 'CX')
$codeFiles = $files | Where-Object { $_.Extension -in '.cpp', '.hpp', '.bat' }
$forbidden = $codeFiles | Select-String -Pattern $forbiddenPattern
if ($forbidden) {
    $forbidden | Select-Object -First 20 | Format-Table -AutoSize
    throw 'AuroCX reference found in codec-v3 encoder'
}
$buildText = Get-Content (Join-Path $root 'build.bat') -Raw
$listed = [regex]::Matches($buildText, 'src\\[^\s^]+\.cpp') |
    ForEach-Object { $_.Value } | Sort-Object -Unique
$actual = Get-ChildItem (Join-Path $root 'src') -Recurse -Filter *.cpp |
    ForEach-Object { $_.FullName.Substring($root.Length + 1) -replace '/', '\' } |
    Sort-Object -Unique
$sourceDiff = Compare-Object $listed $actual
if ($sourceDiff) {
    $sourceDiff | Format-Table -AutoSize
    throw 'build.bat source list differs from src tree'
}
Write-Output 'auro3d-encode lint-clean'
