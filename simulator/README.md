
Lamp-Da simulator
-----------------

This project attempt to simulate the behavior of both Lamp-Da hardware & software.

Not everything is (yet) finished, beware!

Quick setup
-----------

## 0. Dependencies

The simulator uses the [SFML](https://www.sfml-dev.org/) for display, as well as
[CMAKE](https://cmake.org/) to build itself.

You may need to install both packages using your distribution package manager:

```sh
apt-get install libsfml-dev cmake # debian / ubuntu
pacman -S sfml cmake # archlinux / manjaro
```

Note that if SFML is not found, the build system will download & build the SFML
itself.

This may be more suitable for your setup, but takes more time.

## 1. Using the top-level `Makefile`

You can use the top-level repository `Makefile` to build the simulator:

```sh
cd LampColorControler
ls Makefile simulator/Makefile # (use the first one)
make simulator
```

This will setup `cmake` & build the project into `_build/simulator`:

```sh
cd LampColorControler
make simulator
ls _build/simulator # the build directory
_build/simulator/indexable-simulator # the output binary
```

As you edit the code, you can benefit from `cmake` tracking your changes and
rebuilding only what is necessary.

For that, use the generated `Makefile` present in the build directory:

```sh
cd LampColorControler
cd _build/simulator
make
./indexable_simulator
```

You can also use `make -j` to use faster parallel builds.

## 2. Using `cmake` directly

You can also setup the `cmake` project by yourself as follows:

```sh
cd LampColorControler
cd simulator
mkdir _build && cd _build
cmake ..
```

You can then build using the generated `Makefile` as follows:

```sh
cd LampColorControler
cd simulator/_build
make
./indexable-simulator
```

Depending on your setup, this may be more practical to you, or not :)
