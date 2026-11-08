#!/bin/sh
# Prints the full name of the CI image matching this checkout.
#
# The tag is a hash of every file that defines the image content, so a change
# to the build environment points CI to a new image, which must be built with
# the "CI image" workflow (or "make ci-image-build" locally).
set -e
cd "$(dirname "$0")/../.."

CI_IMAGE_REPOSITORY=ghcr.io/baptistehudyma/lamp-da-ci

tag=$(cat tools/ci/Dockerfile tools/ci/environment.mk requirements.txt \
	tools/arduino-cli .dockerignore | sha256sum | cut -c1-16)

echo "$CI_IMAGE_REPOSITORY:$tag"
