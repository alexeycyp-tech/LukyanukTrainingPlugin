# LukyanukTrainingPlugin

An experimental x86 plugin for Richard Burns Rally. This repository contains the plugin source code and the 0.5.11 test package.

## Current test build

Version 0.5.11 removes engine RPM capture and restore. During the frozen restart countdown it temporarily enables Clutch Help and attempts to send an Ignition action through an internal RBRControls route. That route is enabled only for the analyzed RBRControls.dll hash and a matching code signature.

The ignition call and the meaning of its press/release values have not been verified in-game. A log entry only records an internal call attempt; it does not confirm that RBR processed it or started the engine.

## Build

The build script uses Zig 0.13.0 and targets 32-bit Windows. From the repository root, run:

```powershell
.\source\build.ps1 -Zig "C:\path\to\zig.exe"
```

The script writes generated build files under `build`, `Plugins`, and `Symbols`. These folders are excluded from Git.

## Package

The ready-to-install test ZIP is in `release`. See [INSTALL.txt](INSTALL.txt) for the English installation and test steps. Static build details are in [VALIDATION.txt](VALIDATION.txt).

## Scope

This is a test build for local RBR training. It has not been verified in-game. No license file is included.
