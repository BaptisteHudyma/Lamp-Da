# Lamp-Da

A programmable board integrated with a liion battery charger, battery balancer, multiple sensors and gpios (with 3 analog inputs).

This repository contains the bootloader, software and hardware files to create and program the board.

The system supports programming and charge throught the same USB-C port.

The board is made to fit into a 50mm tube.

## File details
- LampColorControler.ino: main class of the program, containing the setup and loop functions

## Physical build and architecture :

The electrical circuit and build files can be found in the **electrical** folder.

![electrical circuit](/Medias/circuit.jpg)

The PCB can be ordered directly assembled from JLC PCB, for a total cost of around 270$ for the minimal 5 pieces command (price drops with a more commands, until around 22$/circuit).

The circuit is 3 cells li-ion USB C charger & balancer & power source, that is also programmable via the same USB port.

The circuit features:
- USB-C 3S li-ion charger, based on BQ25713 ic.
- Battery balancing, based on BQ76905 ic.
- USB short circuit and EC protection, based on TPD8S300 ic.
- USB-C power negotiation, base on FUSB302B ic.
- Programable voltage source, from 3V to 20V, up to 3A, using the charge component
- 8 programmable IO pins (3 of which can be analog inputs, 4 can be pwm outputs). Based on nRF52840 ic.
- MEMS Microphone, placed away from parasitic ringing components.
- LSM6DS3TR IMU
- Bluetooth 5.1 low power, with 5/10m range.

- Multiple protection features (ESD spikes protection, USB voltage snubber, USB voltage limitation, ...)

Be careful:
- **NO REVERSE VOLTAGE PROTECTION FOR BATTERY**: it will blow the circuit right up
- **IO are 3.3V max**, any voltage greater will destroy the system

## Update the system

You can update the program without any tools :

Open a serial connection to the lamp, and use the "DFU" command to reset the system.
The system will appear on a computer as a USB file device.

You just need to copy the .uf2 file matching your lamp type into this USB device.
It will then reboot and update the system itself.

## Behavior

The code in this repo is made to be used as compact lanterns.

base behavior:
- The lamp starts and stops with one click.
- The luminosity can be raised by clicking than holding, and diminished by double-clicking than holding.

The battery level is displayed as a color gradient from green (high) to red (low).

![battery levels](/Medias/lamp_top.jpg)

Error and alerts are displayed as blinking animations:
- low battery: slow red blinks
- critical battery: fast red blinks, emergency shutdown after short delay
- charging: breeze animation, changing color to indicate battery level
- incoherent battery readings: fast green blinks (battery reading do not match the set battery type)
- main loop too slow: fast fushia blinks
- temperature too high: fast orange blinks
- temperature extreme: emergency shutdown
- OTG failed: pink fast blink (failed to start the output power)
- bluetooth advertising: blue breeze animation
- hardware alert: fast purple blinks: generic alert to indicate the failure of a component, without recovery
- unhandled: fast white blinks

### User defined behaviors
The user MUST define the target behavior for the target uses.

Some functions are defined in functions.h files, and must be implemented in functions.cpp
They allow the user to program the exact desired behavior.

Some user types are available for base models, in the user folder:
- cct: Program designed for a constant color temperature led strip.
- indexable: program designed for an indexable led strip wrapped around the lamp body
- simple: Program designed for a constant color led strip

![The different lamps](/Medias/lamp_types.jpg)
Above: simple, cct, indexable

#### Bootloader flash
Out of factory, the board will miss a bootloader.

- Use a raspberry of any other linux system with gpios
- Use the open source program **openocd** as described in the **flashInfo** folder of this repository.

# Building the project

The supported platform to work with the project is:

- Linux (Debian/Ubuntu/Archlinux/etc)

## Installing dependencies

You will need to:

 - Install the Arduino IDE and verify that `arduino-cli` is accessible in path
 - Install the Adafruit nRF52 board support (version 1.6.1) [as described here](https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino?tab=readme-ov-file#recommended-adafruit-nrf52-bsp-via-the-arduino-board-manager)
 - Replace the content of the `$HOME/.arduino15/packages/adafruit/hardware/nrf52` by the content of the [above repository](https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino)
 - Install [`adafruit-nrfutil`](https://github.com/adafruit/Adafruit_nRF52_nrfutil) from PyPI
 - Install the [`Adafruit Neopixel`](https://docs.arduino.cc/libraries/adafruit-neopixel) and
   [`arduinofft`](https://docs.arduino.cc/libraries/arduinofft/) libraries

Alternatively, at the cost of some additional disk space, you may use the provided
`Makefile` to install all dependencies:

```sh
git clone https://github.com/BaptisteHudyma/Lamp-Da.git LampColorControler
cd LampColorControler
make mr_proper arduino-cli-download safe-install
```

This will install everything in the `$SRC_DIR/_build/arduino-cli/` directory.

## Quick setup

First clone the repository:

```sh
git clone https://github.com/BaptisteHudyma/Lamp-Da.git LampColorControler
cd LampColorControler
git submodule update --init
```

As highlighted above, this project uses Arduino SDK, you will need to
install Arduino before continuing to the next step.

Then, check that `arduino-cli` is available in path:

```sh
arduino-cli version
```

If `arduino-cli` is available and you find yourself in trouble installing the
dependencies by hand, you can use:

```sh
# download+install all dependencies (~1.4Go) in $SRC_DIR/_build/arduino-cli/
cd LampColorControler
make safe-install
```

If your system does not package `arduino-cli` or if you're not satisfied with
its packaging, you may try:

```sh
# also download+install arduino-cli in $SRC_DIR/_build/arduino-cli/
cd LampColorControler
make arduino-cli-download safe-install
```

You will then be able to build the project as follows:

```sh
# build project assuming {simple,cct,indexable} lamp type
cd LampColorControler
make simple
```

After plugging your board, you should be able to upload as follows:

```sh
# upload to board using the upload-{simple,cct,indexable} target
cd LampColorControler
make upload-simple
```

If you need to configure the serial port of the board before upload, use:

```sh
# by default LMBD_SERIAL_PORT is /dev/ttyACM0
cd LampColorControler
LMBD_SERIAL_PORT=/dev/ttyACM1 make upload-simple
```

Once your board is running, to connect to its serial monitor, use:

```sh
# use LMBD_SERIAL_PORT as above to configure the serial port
cd LampColorControler
make monitor
```

To build the documentation, you must have `doxygen` installed:

```sh
# use "make clean-doc doc" to force documentation rebuild
cd LampColorControler
make doc
```

When changing lamp type or adding a new file to the sketch, use:

```sh
# use "make clean-artifacts" to remove the output binary artifact
cd LampColorControler
make clean
make indexable
```

If you have a `virtualenv` somewhere with `adafruit-nrfutil` already installed,
you may setup a link to explicitly use it:

```sh
# the Makefile will source $SRC_DIR/venv/bin/activate
cd LampColorControler
ln -s venv path/to/other/virtualenv
make clean-artifacts upload-indexable
```

If you want simulate a view of an "indexable mode" animation and have the
[SFML](https://www.sfml-dev.org/) installed:

```sh
cd LampColorControler
make simulator
./_build/simulator/indexable-simulator
```

Once you're finished with your work on the project:

```sh
# this removes build artifacts AND all local dependencies
cd LampColorControler
make mr_proper
```

## Contributing code

If `clang` is installed on your platform, automatic format will be available:

```sh
# this format all files using clang-format
cd LampColorControler
make format
```

You can verify that all files are properly formatted with:

```sh
# this fails if some files are not properly formatted
cd LampColorControler
make format-verify
```

You can add this verification as a [git pre-commit
hook](https://git-scm.com/book/ms/v2/Customizing-Git-Git-Hooks) with:

```sh
# before every "git commit" checks if formatting is good
cd LampColorControler
make format-hook
git commit # will implictly run "make format-verify"
```

You can check that all files still build after your change using:

```sh
# clean then build everything, including all lamp type
cd LampColorControler
make verify-all
```

Note that simulator files are build only if all dependencies are available.

## How the build system works?

We are relying on Arduino and Adafruit's integrations to do a lot of things.
We want to use modern compiler features and some customization to our setup,
while we don't want to modify how Adafruit's platform core and libraries are
build.

On first build, we will thus first:

0. compile a "clean" `build-clean` target later used to detect how Arduino
   builds our sketch

Then, on all subsequent builds:

1. compile a "dry" `build-dry` target which will build core+libraries with the
   default setup
2. rebuild the sketch objects using our own custom setup (C++17, other flags)
4. compile a "final" `build` target which will let the `arduino-cli` reuse all
   the already-build objects, to produce a final artifact

This is a hack and sanity checks are in place to verify that the sketch have
been properly build (see `make verify-canary`)
