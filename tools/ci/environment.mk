# Pinned build environment, shared by the Makefile and the CI image.
#
# The CI image tag is a hash of this file (see tools/ci/image-name.sh): after
# any change here, run the "CI image" workflow, or CI will fail and ask for it.
# See docs/CIImage.md.

# arduino-cli release (https://github.com/arduino/arduino-cli/releases)
ARDUINO_CLI_VERSION=1.5.1

# adafruit:nrf52 core, which provides the ARM toolchain and upload tools
ADAFRUIT_CORE_VERSION=1.7.0

# custom LampDa board package, cloned over the adafruit:nrf52 core
# (its LAMPDA_FIRMWARE_VERSION must match EXPECTED_FIRMWARE_VERSION in
# src/system/utils/constants.h)
CUSTOM_BOARD_URL=https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino
CUSTOM_BOARD_REF=f7c14f1bef902ee9da43655cf8ae2f678ce6affb

# bump to force a new CI image when an install step changes outside of the
# hashed files (for example the install targets of the Makefile)
CI_IMAGE_REVISION=1
