# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Lamp-Da is firmware (plus hardware files and a desktop simulator) for an nRF52840 board with a 3S li-ion charger, built as a USB-C rechargeable lantern. It is built on the Arduino/Adafruit nRF52 core, with a custom board package (`adafruit:nrf52:lampDa_nrf52840`).

The repo folder must be named `LampColorControler` (the Makefile enforces this), and submodules are required (`git submodule update --init`). Dependencies are listed in `docs/QuickSetup.md`.

## Commands

All commands run from the repo root.

```sh
# Tests (host build, ASan + GoogleTest; requires gtest/gmock and SFML)
make test                # build unit and integration tests
make run-utest           # tests/utest
make run-itest           # tests/itest

# Run a single test (after make test)
cd _build/tests/utest && ./tests_utest --gtest_filter='cmd_parser.*'
cd _build/tests/itest && ./tests_itest --gtest_filter='...'

# Format (CI fails on violations, and on CRLF line endings)
make format
make format-verify
make format-hook         # install as a git pre-commit hook

# Simulator (SFML window, output in _build/simulator/)
make simulator-indexable       # also: simulator-indexable-hd, simulator-simple
make run-simulator-indexable   # build and run

# Firmware (output in _build/artifacts/ and _build/uf2/)
make indexable           # also: simple, cct, indexable-hd
make upload-indexable    # LMBD_SERIAL_PORT=/dev/ttyACM2 to change port (default ttyACM0)
make monitor             # serial console, 115200 baud

# Everything CI cares about: format, tests, all simulators, all lamp types
make verify-all

make doc                 # doxygen, output in docs/html/
make check-stack         # stack analysis of the last firmware build
```

Use `LMBD_JOBS=<n>` for parallel firmware builds and `LMBD_CPP_EXTRA_FLAGS="-Wall -Wextra"` to enable warnings.

## Build system quirks

- **Firmware builds go around arduino-cli.** A first "clean" build records how Arduino compiles the sketch. That log generates `_build/process-<type>.sh`, which recompiles sketch objects with our flags (`gnu++17 -fconcepts`), and arduino-cli then links them. See `docs/Notes.md`. Because the object list and flags are baked into that script, run `make clean` after adding a source file or changing `LMBD_EXTRA_FLAGS`.
- **The HAL is selected by a symlink.** `make` points `src/generated/hal` at `hal/adafruit_nrf/` or `hal/simulator/` depending on the target (gitignored, replaced on each build). arduino-cli follows the link when it copies the sketch into `_build/objs/sketch/`. HAL interfaces are in `src/system/hal/*.h`, and every platform must implement them.
- **Simulator and test sources are listed by hand** in `simulator/compile.cmake`. A new `.cpp` under `src/system/` must be added there as well, and a new HAL file must be added to `SIMULATOR_HAL`. Test files are globbed from `tests/{utest,itest}/src/*.test.cpp`.
- **CI runs in a Docker image** (`tools/ci/Dockerfile`, published to ghcr.io by the manual "CI image" workflow). Its tag is a hash of the environment files, pinned versions live in `tools/ci/environment.mk`, and changing them requires re-running that workflow. See `docs/CIImage.md`.
- **Simulator and test sources are listed by hand** in `simulator/compile.cmake`. A new `.cpp` under `src/system/` must be added there as well, and a new HAL file must be added to `SIMULATOR_HAL`. Test files are globbed from `tests/{utest,utest_simple,itest}/src/*.test.cpp`.
- **Images** in `Medias/to_generate/` are converted to headers in `src/generated/` by `tools/convert_image.py`.

## Architecture

`LampColorControler.ino` only calls `lampda::main_setup()` and `lampda::main_loop()` (`src/system/global.cpp`).

### `src/system/`: layered platform code

Each layer has a matching namespace under `lampda::`. From high level to low level:

| Layer | Role |
|-------|------|
| `logic` | Main state machine (`behavior.cpp`), inputs, alerts, brightness, CLI, power handling, sun timers |
| `component` | Battery, button, charger, output power, sound, IMU, LED strip, time |
| `bsp` | Board logic above the HAL: balancer, charging IC, power gates, BLE, USB-PD |
| `driver` | Device drivers (LSM6DS3, FUSB302) |
| `hal` | Platform interfaces, implemented in the top-level `hal/` |
| `common`, `utils`, `ext` | Generic state machine, command parser, curves, FFT, colors, FastLED-style math |

`logic/behavior.cpp` drives the system through `START_LOGIC`, `PRE_CHARGER_OPERATION`/`CHARGER_OPERATIONS`, `PRE_OUTPUT_LIGHT`/`OUTPUT_LIGHT`/`POST_OUTPUT_LIGHT`, `SHUTDOWN` and `ERROR`. User code only runs in `OUTPUT_LIGHT`.

The main loop runs at a fixed period (`MAIN_LOOP_UPDATE_PERIOD_MS`) under a watchdog, so code on it must not block. Slow loops raise an alert (`check_loop_runtime` in `global.cpp`), and a stuck loop is reset by the watchdog.

### `src/user/`: lamp type integration

`functions.h` declares the callbacks the system calls (power on/off, button events, `loop`, `user_thread`, parameters). Exactly one of `simple_functions.cpp`, `cct_functions.cpp` or `indexable_functions.cpp` provides them, selected by the `LMBD_LAMP_TYPE__*` define; `src/compile.h` checks that the define is consistent. The other two compile to nothing.

### `src/modes/`: lighting modes (indexable lamps)

A header-only, compile-time mode framework, included from `src/user/indexable_functions.cpp`, where modes are registered in the `ManagerTy` list (`modes::GroupFor<...>` groups inside a manager). There is no dynamic dispatch: modes are structs inheriting from `modes::BasicMode` and are never instantiated.

- All methods are `static` and take `auto& ctx`; members must be `static constexpr`.
- State lives in a nested `struct StateTy` and is accessed through `ctx.state`. Do not use globals, statics or instance members.
- Optional features are opt-in through flags such as `hasCustomRamp`, `hasButtonCustomUI`, `hasSystemCallbacks` and `requireUserThread`.

The full API is in `src/modes/include/mode_type.hpp`, and the tutorial is `docs/WritingModes.md`.

### Simulator and tests

`simulator/` compiles the real `src/` code against `hal/simulator/` and mocks the electrical ICs (`simulator/electrical/`). `simulator/resources/simulation_parameters.txt` (battery voltage, VBUS, CPU temperature, added loop delay) is re-read on every tick, so edits to this tracked file apply at runtime and show up as local git changes.

Tests link the `simulator_indexable` library, so they mostly exercise the indexable build. `tests/utest_simple` links `simulator_simple` for behaviors that only exist on the simple lamp (such as output blips, which assert on indexable), and `make run-utest` runs it too. Integration tests drive the system through `simulator/include/simulator_state.h` and `hardware_influencer.h`, and they use real threads and timing.

There is no CCT simulator: `cct` is only checked by the firmware build.

## Conventions

- Code targets a RAM-limited microcontroller: prefer fixed-capacity ETL containers (such as `etl::string<N>`, from `libs/etl`).
- `src/depends/` and `libs/etl` are submodules; do not edit them here.
