param([Parameter(Mandatory=$true)][string]$Zig)
$ErrorActionPreference='Stop'
$Zig=(Resolve-Path -LiteralPath $Zig).Path
$zigRoot=Split-Path -Parent $Zig
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskBuild=Join-Path $taskRoot 'build'
New-Item -ItemType Directory -Force -Path $taskBuild | Out-Null
$env:TEMP=$taskBuild
$env:TMP=$taskBuild
$env:ZIG_LIB_DIR=Join-Path $zigRoot 'lib'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $taskBuild 'zig-cache'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $taskBuild 'zig-local'
New-Item -ItemType Directory -Force -Path (Join-Path $env:ZIG_GLOBAL_CACHE_DIR 'tmp'),(Join-Path $env:ZIG_LOCAL_CACHE_DIR 'tmp') | Out-Null
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -g -fno-temp-file -Wall -Wextra -Werror -Wno-unused-function -shared (Join-Path $PSScriptRoot 'bookmark.c') (Join-Path $PSScriptRoot 'jump_back.c') (Join-Path $PSScriptRoot 'LukyanukTrainingPlugin.def') -lbcrypt -luser32 -o (Join-Path $taskRoot 'Plugins\LukyanukTrainingPlugin.dll')
if($LASTEXITCODE -ne 0){throw 'DLL build failed'}
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot 'Symbols') | Out-Null
$taskPdb=Join-Path $taskRoot 'Plugins\LukyanukTrainingPlugin.pdb'
if(Test-Path -LiteralPath $taskPdb){Move-Item -LiteralPath $taskPdb -Destination (Join-Path $taskRoot 'Symbols\LukyanukTrainingPlugin.pdb') -Force}
$taskLib=Join-Path $taskRoot 'Plugins\bookmark.lib'
if(Test-Path -LiteralPath $taskLib){Move-Item -LiteralPath $taskLib -Destination (Join-Path $taskBuild 'bookmark.lib') -Force}