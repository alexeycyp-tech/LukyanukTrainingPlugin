# LukyanukTrainingPlugin 1.0

An x86 plugin for Richard Burns Rally, based on the 0.5.17 build.

## Use

- Press **Enter** to save a restart position and start a new timing session. The stopwatch display appears immediately; the measured run starts when the car crosses the line 5 m after the saved position.
- Press **Space** at the desired finish point. The first timed pass becomes the comparison reference when it contains at least two valid samples, and the car returns to the saved position. Later Space presses repeat the run and show the time difference.
- Use the assigned **Jump Back** button to reset to the nearest saved point behind the car. The distance value keeps its existing size; the “JUMP BACK” label is smaller.
- Save, Restart, and Jump Back buttons can be assigned in the plugin menu.

## Build

The build script uses Zig 0.13.0 and targets 32-bit Windows. From the repository root:

```powershell
.\source\build.ps1 -Zig "C:\path\to\zig.exe"
```

Generated build files are written under `build`, `Plugins`, and `Symbols`; these folders are excluded from Git.

## Package

The ready-to-install package is `release/LukyanukTrainingPlugin-1.0.zip`. See [INSTALL.txt](INSTALL.txt) for installation steps and [VALIDATION.txt](VALIDATION.txt) for build checks.

The experimental phone Turn History bridge from 0.5.18/0.5.19 is not included in this 1.0 build.
