# Builds HALO (VST3 + standalone) and wraps it in the branded installer:
#   dist\HALO-<version>-Windows-Setup.exe
$ErrorActionPreference = "Stop"
$root = Resolve-Path "$PSScriptRoot\..\.."
$version = "1.1.1"
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) { $cmake = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" }

& $cmake -S $root -B "$root\build" -A x64
& $cmake --build "$root\build" --config Release --target HBHalo_VST3 HBHalo_Standalone --parallel

$art = "$root\build\HBHalo_artefacts\Release"
$payload = "$root\packaging\windows\payload"
New-Item -ItemType Directory -Force $payload | Out-Null
Copy-Item "$art\VST3\HALO.vst3\Contents\x86_64-win\HALO.vst3" "$payload\HALO.vst3" -Force
Copy-Item "$art\VST3\HALO.vst3\Contents\Resources\moduleinfo.json" "$payload\moduleinfo.json" -Force
Copy-Item "$art\Standalone\HALO.exe" "$payload\HALO.exe" -Force

& $cmake -S $root -B "$root\build-setup" -A x64 -DHALO_BUILD_INSTALLER=ON
& $cmake --build "$root\build-setup" --config Release --target HALOSetup --parallel

New-Item -ItemType Directory -Force "$root\dist" | Out-Null
Copy-Item "$root\build-setup\HALOSetup_artefacts\Release\HALO Setup.exe" "$root\dist\HALO-$version-Windows-Setup.exe" -Force
Write-Host "Done: dist\HALO-$version-Windows-Setup.exe"
