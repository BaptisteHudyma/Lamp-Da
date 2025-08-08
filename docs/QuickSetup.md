
# Quick Setup

First clone the repository:

```sh
git clone https://github.com/BaptisteHudyma/Lamp-Da.git LampColorControler
cd LampColorControler
git submodule update --init
```

This project uses the Arduino SDK, you will need to install several
dependencies before being able to build the project and upload your
firmware.

## Useful Linux packages

For reference, if you are on a `Debian/Linux (debian12,bookworm)` system, here
is a list of packages that you may need to use this project:

 - `build-essential gcc g++ clang clang-format make cmake git` for development
 - `python3 python3-venv python-is-python3` to flash the board & use tools
 - optionally, if you want to build the simulator:

    - `libsfml-dev` if you are on a platform that have `SFML 3.0.1`
      (e.g. `sid` or `experimental`)
    - `libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev` and
      `libfreetype-dev libvorbis-dev libflac-dev` if you only have `SFML 2.6.2`
      or lower available, as the simulator build system will try to download
      and compile the last SFML version from source.

Depending on your setup (Ubuntu, WSL, Fedora, Archlinux, …) you may need to
install other packages, such as `arduino-cli` or `arduino-ide` as described
below.

## Arduino Setup

You will need to:

 - Install the Arduino IDE and verify that `arduino-cli` is accessible in path
 - Install the Adafruit nRF52 board support (version 1.6.1) [as described here](https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino?tab=readme-ov-file#recommended-adafruit-nrf52-bsp-via-the-arduino-board-manager)
 - Replace the content of the `$HOME/.arduino15/packages/adafruit/hardware/nrf52` by the content of the [above repository](https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino)
 - Install [`adafruit-nrfutil`](https://github.com/adafruit/Adafruit_nRF52_nrfutil) from PyPI
 - Install the [`Adafruit Neopixel`](https://docs.arduino.cc/libraries/adafruit-neopixel) and
   [`arduinofft`](https://docs.arduino.cc/libraries/arduinofft/) libraries

To check that `arduino-cli` is available in path, you can run:

```sh
arduino-cli version
```

If your system does not package `arduino-cli` or if it's not working for you,
you may try the following recipe to make it available locally:

```sh
# also download+install arduino-cli in $SRC_DIR/_build/arduino-cli/
cd LampColorControler
make arduino-cli-download safe-install
```

Now that `arduino-cli` is available, if you find yourself in trouble
installing the Arduino dependencies by hand, you can use the following
to install everything locally:

```sh
# download+install all dependencies (~1.4Go) in $SRC_DIR/_build/arduino-cli/
cd LampColorControler
make safe-install
```

## Building the project

Once all dependencies are set up, you will be able to build the project
as follows:

```sh
# build project assuming simple lamp type
cd LampColorControler
make simple
```

If you are working on the `indexable` variant of the lamp, you
can use:

```sh
# build project assuming simple lamp type
cd LampColorControler
make indexable
```

When changing lamp type or adding a new file to the sketch, use:

```sh
cd LampColorControler
make clean
make indexable
```

You can also use the `clean-artifacts` recipe to remove only the output
binaries, and to only rebuild what is necessary:

```sh
# remove build artifacts & rebuild the project
cd LampColorControler
make clean-artifacts {simple,cct,indexable} # pick one
```

## Uploading to board

After plugging your board, you should be able to upload as follows:

```sh
# use upload-indexable if you are working with indexable
cd LampColorControler
make upload-simple
```

Note that `upload-{simple,cct,indexable}` checks that you have build the
associated `{simple,cct,indexable}` recipe, and that you may get an error
message if you are mixing up the different versions.

You may need to configure the serial port of the board before uploading:

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

If you have a `virtualenv` somewhere with `adafruit-nrfutil` already
installed, you may setup a link to explicitly use it:

```sh
# the Makefile will source $SRC_DIR/venv/bin/activate
cd LampColorControler
ln -s venv path/to/other/virtualenv
make clean-artifacts upload-indexable
```

## Building the documentation

To build the documentation, you must have `doxygen` installed:

```sh
# use "make clean-doc doc" to force documentation rebuild
cd LampColorControler
make doc
```

You will be able to preview the documentation as follows:

```sh
$(BROWSER) ./docs/html/index.html
```

## Building the simulator

If you want simulate a view of an "indexable mode" animation and have the
[SFML](https://www.sfml-dev.org/) installed:

```sh
cd LampColorControler
make simulator
./_build/simulator/indexable-simulator
```

This may build the SFML library if `SFML 3.0.1` is not available locally, and
it will fail if some of the required dependencies are missing.

Note that the compiler used is `/usr/bin/c++` by default, and as we are using
modern C++17 features as well as things like `-fconcepts` you will require an
up-to-date local compiler -- here "up-to-date" typically means something from
2020 :)

## Cleaning up

There are several `clean-*` recipes available:

```sh
# "quick" clean recipes
make clean-artifacts # removes output build artifacts, use it to force rebuild
make clean-doc # removes output "index.html" artifact, use it to force rebuild
make clean-simulator # removes simulator artifacts, use it to force rebuild :)

# "full" clean recipes
make clean # must be called when changing LAMP_TYPE, or if you added a .cpp file
```

Once you're finished with your work on the project:

```sh
# this removes build artifacts AND all local dependencies
cd LampColorControler
make mr_proper
```

Note that this last method WILL remove all the locally installed dependencies,
that may be very expensive to install again (more than a gigabyte of setup).
