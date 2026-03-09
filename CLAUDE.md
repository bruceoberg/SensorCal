# CLAUDE.md — SensorCal

## What this repo is

SensorCal is a macOS/Windows/Linux desktop application for calibrating IMU sensors on
Adafruit Feather devices. It reads sensor data over USB serial, runs calibration algorithms
via libcalib, and sends the resulting calibration matrices back to the device.

SensorCal was forked from MotionCal (Paul Stoffregen, 2016), which was pure C with a fragile
Makefile. SensorCal has been migrating to C++ and CMake. The calibration logic lives in
`vendor/libcalib`; SensorCal is responsible only for UI, serial I/O, and orchestration.

The long-term goal is for Adafruit to take ownership of SensorCal and libcalib as first-party
tools. Adafruit already references MotionCal in their learning guides; SensorCal is its
intended normie-friendly successor. Code quality, cross-platform hygiene, and documentation
must reflect that ambition.

## How to work in this repo

- **Never `git add`, `git commit`, or `git push` anything**, ever, under any circumstances.
  Leave all changes as working tree modifications only.
- **Never create or switch branches** without explicit instruction from the user.
- When a task is complete, summarize what files changed and why. The user will review
  every diff, stage what they want, and commit on their own schedule.
- Before making changes that touch more than one file, propose a plan and wait for approval.
- Prefer small, focused changes over large rewrites in a single pass.
- Write code as if the user will need to read, understand, and debug it without your help.
  Clarity over cleverness, always.

## Repo layout

```
SensorCal/
├── src/
│   ├── gui.cpp/h         # wxWidgets UI
│   ├── lineparser.cpp/h  # CLineParser — serial line parsing
│   ├── serialdata.cpp    # serial I/O, calibration send
│   ├── visualize.cpp     # OpenGL sphere rendering
│   └── ...
├── vendor/
│   ├── libcalib/         # git submodule — calibration library (see vendor/libcalib/CLAUDE.md)
│   ├── Fusion/           # git submodule — xioTechnologies AHRS (libcalib dependency)
│   └── serial_cpp/       # git submodule — gbionics/serial_cpp
├── CMakeLists.txt
├── CMakePresets.json     # named presets: debug, dev, release
├── CLAUDE.md             # this file
└── CLAUDE-coding.md      # coding style reference
```

Build system: **CMake + Ninja**. Presets use separate `binaryDir` paths per configuration.
UI framework: **wxWidgets**. One panel uses raw OpenGL for the magnetometer sample sphere.
Build environment: **Nix/devenv** on macOS (aarch64-darwin). VSCode with CMake Tools.

## Build / debug notes

- CMakePresets.json has named presets: `debug`, `dev`, `release`
- Each preset uses a separate `binaryDir`
- Debugger: VSCode with `llvm-vs-code-extensions.lldb-dap` extension, `"type": "lldb-dap"`,
  pointing at Xcode's `lldb-dap` binary via `xcrun -f lldb-dap` — do NOT use the
  Nix-provided lldb-dap (version incompatibility with the extension)
- The app requires ad-hoc code signing with `get-task-allow` entitlement, automated via
  CMake `POST_BUILD` custom command

## Current state of the code

The codebase is split between:
- **Clean C++**: libcalib and newer SensorCal code
- **Legacy C**: original MotionCal code that needs systematic replacement

When rewriting legacy C, bring it fully up to the coding standard. Do not blend styles
within a file.

### serialdata.cpp

`src/serialdata.cpp` handles serial I/O and calibration packet sending.

**Serial I/O** — uses `gbionics/serial_cpp` (vendored at `vendor/serial_cpp`), a cross-platform
library with port enumeration support. The library throws exceptions; `serialdata.cpp` catches
them at the API boundary and converts to return codes.

**Line parsing** — delegated to `CLineParser` (see `src/lineparser.cpp`).

**`send_calibration()`** — still has known issues:
- Sends `0.0f` for accelerometer and gyro offsets — placeholders, not real calibration
- `cal_data_sent` index mapping is hand-counted with magic number comments like `//10`
- `invW` indices in `cal_data_sent` don't match the binary packet ordering

## Serial wire protocol

The Feather (running kitelite or Adafruit imucal firmware) sends two line types at ~100Hz:

**`Raw:` line** — 9 comma-separated int16 values, CR-terminated (MotionCal-compatible):
```
Raw:<ax>,<ay>,<az>,<gx>,<gy>,<gz>,<mx>,<my>,<mz>\r\n
```
Encoding (see `imucal.ino`):
- accel: g × 8192 → int16 (decode: × 1/8192 → g)
- gyro: deg/s × 16 → int16 (decode: × 1/16 → deg/s)
- mag: µT × 10 → int16 (decode: × 1/10 → µT)

**`Uni:` line** — 9 comma-separated floats, CR-terminated (full precision):
```
Uni:<ax>,<ay>,<az>,<gx>,<gy>,<gz>,<mx>,<my>,<mz>\r\n
```
- accel: m/s², gyro: rad/s, mag: µT — SI units

**Unit mismatch:** The `Raw:` and `Uni:` lines use different units for accel (g vs m/s²)
and gyro (deg/s vs rad/s). This is a quirk of the MotionCal-compatible protocol.
`CLineParser` normalizes both to (g, deg/s, µT) for libcalib, which expects deg/s for gyro.

**Parser implementation:** `CLineParser` in `src/lineparser.cpp`:
- Line-buffered, per-instance state (no statics), fully reentrant
- Uses `strtof`/`strtol` for number parsing
- Handles `Raw:`, `Uni:`, `Cal1:`, and `Cal2:` line types
- Outputs `SSample` struct with values in (g, deg/s, µT)

**Send protocol** — binary packet (68 bytes) sent to device:
- 2-byte signature (0x75, 0x54)
- 3× float: accelerometer offsets
- 3× float: gyroscope offsets
- 3× float: magnetometer hard iron (V vector)
- 1× float: field strength (B)
- 6× float: soft iron upper triangle (invW[0][0], [1][1], [2][2], [0][1], [0][2], [1][2])
- 2-byte CRC16

## Cross-platform serial library

Serial I/O uses **gbionics/serial_cpp** (vendored at `vendor/serial_cpp`), a fork of
wjwwood/serial with the catkin/ROS dependency removed. Features:

- Cross-platform: macOS (IOKit), Windows (setupapi), Linux
- Port enumeration built-in (needed for wizard auto-detect)
- CMake-native, easy to vendor as a git submodule
- PySerial-like API (familiar to Adafruit audience)

Note: The library uses exceptions for error handling. `serialdata.cpp` catches these at
the API boundary and converts to return codes to match SensorCal's no-exceptions convention.

## The wizard UI vision

The current UI is a single screen with a port dropdown, a sphere, and a send button.
The target is a **guided wizard** with these steps:

1. **Auto-detect**: cycle all serial ports, identify the one sending `Raw:`/`Uni:` data
2. **Magnetometer**: "START MOVING" prompt, live sphere display, completion from libcalib
3. **Accelerometer**: guided HOLD STILL sequence through 6 orientations
4. **Gyroscope**: single HOLD STILL step
5. **Send**: transmit all three calibration results, confirm echo

All step logic and completion criteria live in **libcalib**. SensorCal provides wxWidgets
panels. kitelite provides LED/TFT feedback for the same state machine.

## Coding style

See `CLAUDE-coding.md` in the repo root for full reference. Key rules:

- Tabs (not spaces), 4-wide
- `m_` prefix on all class/struct members
- `g_` prefix on file-static globals; `s_` on function-static variables
- No `enum class` — plain enums, integer-convertible
- `ASSERT` / `CASSERT` / `VERIFY` for correctness checks
- `nullptr` not `NULL`
- C++ casts only — never C-style casts
- `const` on read-only methods and pointed-to data
- `override` on all overridden virtual functions
- `//` comments only, never `/* */`
- `BB(username)` for known improvement areas; `NOTE(username)` for non-obvious decisions
- Functions named `[ReturnTag]VerbNoun`: `FIsOpen()`, `ReadData()`, `SendCalibration()`

## Vendor submodules

All vendored dependencies live under `vendor/`:
- `vendor/libcalib` — calibration library (see `vendor/libcalib/CLAUDE.md`)
- `vendor/Fusion` — xioTechnologies/Fusion (dependency of libcalib, not used directly)
- `vendor/serial_cpp` — gbionics/serial_cpp, cross-platform serial I/O

## CI / release goals

- GitHub Actions CI targeting macOS, Windows, Linux
- Produce actual release binaries
- Target audience: Adafruit/Feather users who are not developers