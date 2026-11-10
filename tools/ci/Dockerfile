# Build environment for the Lamp-Da CI: tests, simulators, documentation and
# firmware, with arduino-cli and the board package already installed.
#
# Built by the "CI image" workflow, or locally with "make ci-image-build".
# Versions are pinned in tools/ci/environment.mk. See docs/CIImage.md.

FROM ubuntu:24.04

LABEL org.opencontainers.image.source="https://github.com/BaptisteHudyma/Lamp-Da"
LABEL org.opencontainers.image.description="Lamp-Da CI build environment"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
		ca-certificates curl git make bash unzip \
		build-essential cmake \
		clang clang-format llvm dos2unix doxygen \
		python3 python3-venv python3-pip \
		libsfml-dev \
		libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
		libfreetype-dev libvorbis-dev libflac-dev libgl1-mesa-dev \
		libopenal-dev libalut-dev \
		libgtest-dev libgmock-dev \
	&& rm -rf /var/lib/apt/lists/* \
	&& git config --system --add safe.directory '*'

# install locations, read by the Makefile and tools/arduino-cli
ENV LMBD_VENV_DIR=/opt/lampda/venv
ENV LMBD_ARDUINO_LOC=/opt/lampda/arduino-cli
ENV ARDUINO_DIRECTORIES_USER=/opt/lampda/arduino-cli/user
ENV ARDUINO_DIRECTORIES_DATA=/opt/lampda/arduino-cli/data
ENV ARDUINO_DIRECTORIES_DOWNLOADS=/opt/lampda/arduino-cli/downloads
ENV PATH=/opt/lampda/venv/bin:/opt/lampda/arduino-cli:$PATH

# run the same install targets as a local setup, from a minimal copy of the
# repository (the folder name matters to the Makefile)
WORKDIR /tmp/LampColorControler
COPY Makefile requirements.txt ./
COPY tools/arduino-cli tools/arduino-cli
COPY tools/ci/environment.mk tools/ci/environment.mk
RUN make arduino-cli-download safe-install \
	&& rm -rf /tmp/LampColorControler $LMBD_ARDUINO_LOC/downloads \
	&& chmod -R a+rwX /opt/lampda

WORKDIR /
