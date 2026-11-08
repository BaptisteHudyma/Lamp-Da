# CI image

All CI jobs run in a Docker image that already contains the build environment:
compilers, test libraries, SFML, doxygen, the python virtualenv, `arduino-cli`
and the LampDa board package. CI downloads none of them, so the downloads
cannot make it fail and it starts faster.

The image is published at `ghcr.io/baptistehudyma/lamp-da-ci`.

## Files

| File | Role |
|------|------|
| `tools/ci/Dockerfile` | Image definition. It runs `make arduino-cli-download safe-install` from a minimal copy of the repository |
| `tools/ci/environment.mk` | Pinned versions: arduino-cli, adafruit:nrf52 core, custom board commit. The Makefile includes it, so local installs use the same versions |
| `tools/ci/image-name.sh` | Prints the image name for this checkout |
| `.github/workflows/ci-image.yml` | Manual workflow that builds, checks and publishes the image |

## Image tag

The tag is a hash of `tools/ci/Dockerfile`, `tools/ci/environment.mk`,
`requirements.txt`, `tools/arduino-cli` and `.dockerignore`. The first CI job
computes it and fails with "CI image not found" when no image with that tag is
published. A change to the environment can therefore never silently run on an
old image.

Install steps in the `Makefile` are not part of the hash. If you change one of
them, bump `CI_IMAGE_REVISION` in `tools/ci/environment.mk`.

## Updating the environment

1. Edit the versions in `tools/ci/environment.mk` (or the Dockerfile, or
   `requirements.txt`).
2. Push the branch to `BaptisteHudyma/Lamp-Da`.
3. In GitHub, open Actions > "CI image" > "Run workflow", and select the branch.
4. Re-run the failed CI.

The workflow only publishes from `BaptisteHudyma/Lamp-Da`. A pull request from
a fork that changes the environment needs a maintainer to run it on a branch of
the main repository with the same changes. Review the changes first: the
workflow can write packages.

Old tags stay published, so CI on older branches keeps working.

## Visibility

The package must be **public**, so pull requests from forks can pull it without
credentials. GitHub creates a new package as private: after the first publish,
open the package settings on GitHub (Packages > `lamp-da-ci` > Package
settings) and set the visibility to public. This is only needed once.

## Local use

```sh
make ci-image-name    # image name for this checkout
make ci-image-build   # build it locally (needs docker)
make ci-image-shell   # shell in the image, with the repository mounted
```

`make ci-image-shell` pulls the published image if it is not built locally.
Inside, every `make` target works as in CI, for example `make clean indexable`
or `make test`.
