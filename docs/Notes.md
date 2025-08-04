
# Notes

This is miscellaneous documentation around the project and its setup.

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
