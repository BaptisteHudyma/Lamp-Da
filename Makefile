SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
VENV_DIR=$(SRC_DIR)venv
BUILD_DIR=$(SRC_DIR)_build
TOOLS_DIR=$(SRC_DIR)tools
CACHE_DIR=$(BUILD_DIR)/cache
ARTIFACTS=$(BUILD_DIR)/artifacts
OBJECTS_DIR=$(BUILD_DIR)/objs
ARDUINO_LOC=$(BUILD_DIR)/arduino-cli
COMPILED_UF2=$(BUILD_DIR)/uf2

SHELL:=/bin/bash # required by python3 virtualenv
PYTHON_EXE=/usr/bin/env python3
SOURCE_VENV=. $(VENV_DIR)/bin/activate
ARDUINO_CLI_DETECTED=$(shell PATH="$(SRC_DIR):${PATH}" which arduino-cli)
ARDUINO_CLI_ENV=ARDUINO_LOC=$(ARDUINO_LOC) VENV_DIR=$(VENV_DIR) NB_JOBS=${LMBD_JOBS}
ARDUINO_CLI=$(SOURCE_VENV) && $(ARDUINO_CLI_ENV) $(ARDUINO_CLI_DETECTED)
ARDUINO_USER=$(shell $(ARDUINO_CLI) config get directories.user)
ARDUINO_DATA=$(shell $(ARDUINO_CLI) config get directories.data)
ARDUINO_BOARD=$(ARDUINO_DATA)/packages/adafruit/hardware/nrf52
ADAFRUIT_URL=https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
CUSTOM_BOARD_URL=https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino
FULL_LAMP_TYPE=LMBD_LAMP_TYPE__$(shell echo ${LMBD_LAMP_TYPE} | tr '[:lower:]' '[:upper:]')

FOLDER_NAME=$(shell basename $(SRC_DIR))
FQBN=adafruit:nrf52:lampDa_nrf52840
PROJECT_NAME=LampColorControler
PROJECT_INO=$(PROJECT_NAME).ino
COMPILER_CMD=$(shell $(ARDUINO_CLI) compile -b $(FQBN) --show-properties|grep compiler.cpp.cmd|cut -f2 -d=)
COMPILER_PATH=$(shell $(ARDUINO_CLI) compile -b $(FQBN) --show-properties|grep compiler.path|cut -f2 -d=)

CPP_INCLUDES=-I$(OBJECTS_DIR)/sketch -DLMBD_MISSING_DEFINE
CPP_BASIC_FLAGS=-std=gnu++17 -fconcepts -DNDEBUG -DLMBD_CPP17 -D$(FULL_LAMP_TYPE) $(CPP_INCLUDES) ${LMBD_EXTRA_FLAGS}
CPP_BUILD_FLAGS=$(CPP_BASIC_FLAGS) -fdiagnostics-color=always -Wno-unused-parameter -ftemplate-backtrace-limit=1 # -fsanitize=undefined,address -fstack-protector-all
ARDUINO_EXTRA_FLAGS="compiler.cpp.extra_flags='$(CPP_INCLUDES) -D$(FULL_LAMP_TYPE) ${LMBD_EXTRA_FLAGS}'"
#
# to enable warnings:
# 	LMBD_CPP_EXTRA_FLAGS="-Wall -Wextra" make

all: build

#
# make doc
#

$(SRC_DIR)/docs/html/index.html:
	@echo; echo " --- $@"
	cd $(SRC_DIR) && doxygen doxygen.conf

doc: $(SRC_DIR)/docs/html/index.html
	@echo " --- ok: $@"

#
# to pick a lamp flavor:
# 	LMBD_LAMP_TYPE="indexable" make build
#

cct: build-cct

simple: build-simple

indexable: build-indexable

simple2indexable: build-simple2indexable

build-cct:
	LMBD_LAMP_TYPE="cct" make build

build-simple:
	LMBD_LAMP_TYPE="simple" make build

build-indexable:
	LMBD_LAMP_TYPE="indexable" make build

build-simple2indexable:
	LMBD_LAMP_TYPE="indexable" LMBD_EXTRA_FLAGS="-DLMBD_SIMPLE_EMULATOR=1" make build

upload-cct:
	LMBD_LAMP_TYPE="cct" make upload

upload-simple:
	LMBD_LAMP_TYPE="simple" make upload

upload-indexable:
	LMBD_LAMP_TYPE="indexable" make upload

upload-simple2indexable:
	LMBD_LAMP_TYPE="indexable" LMBD_EXTRA_FLAGS="-DLMBD_SIMPLE_EMULATOR=1" make upload

has-correct-folder-name:
	@echo; echo " --- $@"
	@[ $(FOLDER_NAME) == $(PROJECT_NAME) ] || \
		(echo; echo \
		 ; echo 'ERROR: The folder name should be $(PROJECT_NAME)' \
		 ; echo \
		 ; echo 'The repository should have been cloned using' \
		 ; echo '    git clone https://github.com/BaptisteHudyma/Lamp-Da.git $(PROJECT_NAME)' \
		 ; echo \
		 ; echo \
		 ; false)

has-lamp-type: has-correct-folder-name
	@echo; echo " --- $@"
	@test ! -z "$$LMBD_LAMP_TYPE" || \
		(echo; echo \
		 ; echo 'ERROR: unspecified flavor!' \
		 ; echo; echo \
		 ; echo 'To build, either use:' \
		 ; echo '    make cct' \
		 ; echo '    make simple' \
		 ; echo '    make indexable' \
		 ; echo \
		 ; echo 'To upload, either use:' \
		 ; echo '    make upload-cct' \
		 ; echo '    make upload-simple' \
		 ; echo '    make upload-indexable' \
		 ; echo \
		 ; echo 'Or specify flavor manually:' \
		 ; echo '    LMBD_LAMP_TYPE='indexable' make process' \
		 ; echo \
		 ; echo \
		 ; false)


#
# dependency checks
#

has-board-installed: install-venv
	@echo; echo " --- $@"
	@($(ARDUINO_CLI) board listall\
		| grep -q '$(FQBN)') || \
		(echo; echo\
		 ; echo 'ERROR: board "$(FQBN)" not found!' \
		 ; echo \
		 ; echo 'Troubleshooting:' \
		 ; echo ' - home directory is "${HOME}"' \
		 ; echo ' - detected "arduino-cli" is "$(ARDUINO_CLI_DETECTED)"' \
		 ; echo '    - if empty, verify that "arduino-cli" is installed' \
		 ; echo '    - try "make mr_proper arduino-cli-download safe-install" to try fully local setup' \
		 ; echo '      (on some distributions, such local arduino-cli binary may not have network access)' \
		 ; echo ' - detected $$(ARDUINO_DATA) directory is "$(ARDUINO_DATA)"' \
		 ; echo '    - if empty, verify that "arduino-cli" is executable in $$PATH' \
		 ; echo ' - adafruit should be listed in "$$(ARDUINO_DATA)/packages" below:' \
		 ; echo -n '        '; ls '$(ARDUINO_DATA)/packages' \
		 ; echo ' - nrf52 should be listed in "$$(ARDUINO_DATA)/packages/hardware" below:' \
		 ; echo -n '        '; ls '$(ARDUINO_DATA)/packages/adafruit/hardware' \
		 ; echo ' - have you replaced it by the "custom LampDa platform" repository?' \
		 ; echo ' - if yes, the "LampDa_nrf52840" board should be listed below:'; echo \
		 ; find $(ARDUINO_DATA)/packages/adafruit/hardware/nrf52 -name 'boards.txt' -exec cat '{}' \; \
			| grep 'nrf52840.build.variant' \
		 ; echo; echo ' - you can also try "make mr_proper safe-install" (may take 1.4 gigabyte of space)' \
		 ; echo; echo \
		 ; false)

has-libs-installed(%): install-venv
	@echo " --- $@($%)"
	@($(ARDUINO_CLI) lib list\
		| grep -q '$%') || \
		(echo; echo\
		 ; echo 'ERROR: library "$%" not found!' \
		 ; echo \
		 ; echo 'Troubleshooting:' \
		 ; echo ' - home directory is "${HOME}"' \
		 ; echo ' - detected arduino user directory is "$(ARDUINO_USER)"' \
		 ; echo ' - library should be listed in "$(ARDUINO_USER)/libraries" below:' \
		 ; echo -n '        '; ls '$(ARDUINO_USER)/libraries' \
		 ; echo ' - install libraries through the IDE or using one of the following:' \
		 ; echo '        arduino-cli lib install "$%"' \
		 ; echo ' 		 make mr_proper safe-install-libs # (may take 1.4 gigabyte of space)' \
		 ; echo; echo \
		 ; false)

check-compiler-cmd: has-board-installed
	@echo " --- $@"
	@($(COMPILER_PATH)$(COMPILER_CMD) --version \
		| grep -qi 'Arm Embedded Processors') || \
		(echo; echo\
		 ; echo 'ERROR: compiler "$(COMPILER_CMD)" seems to be inappropriate?' \
		 ; echo \
		 ; echo 'Troubleshooting:' \
		 ; echo ' - detected compiler path is "$(COMPILER_PATH)"' \
		 ; echo ' - detected compiler command is "$(COMPILER_CMD)"' \
		 ; echo ' - output of "$(COMPILER_CMD) --version" below:' \
		 ; echo; $(COMPILER_PATH)$(COMPILER_CMD) --version \
		 ; echo ' - expected "Arm Embedded Processors" in compiler version string' \
		 ; echo ' - packages like "libc6-dev-i386" or "lib32-glibc" may be needed locally for 32-bit support' \
		 ; echo ' - packages like "gcc-arm-none-eabi" or "arm-none-eabi-gcc" may be needed locally for ARM support' \
		 ; echo ' - you can also try "make mr_proper safe-install" (may take 1.4 gigabyte of space)' \
		 ; echo \
		 ; false)

$(BUILD_DIR)/.deps-ok:
	@echo " --- $@"
	@mkdir -p $(BUILD_DIR)
	@test -f $(BUILD_DIR)/.deps-ok \
		|| (make has-board-installed check-compiler-cmd 'has-libs-installed(Adafruit\ NeoPixel)' 'has-libs-installed(arduinoFFT)' \
			&& touch $(BUILD_DIR)/.deps-ok)

check-arduino-deps: install-venv $(BUILD_DIR)/.deps-ok
	@echo " --- ok: $@"

#
# automatic install
#	- try "make safe-install" to install all dependencies in a local directory
#	- this could take up to 1.4 gigabytes of space :)
#

# virtualenv install
$(VENV_DIR)/bin/activate:
	@echo; echo " --- $@"
	@($(PYTHON_EXE) -m venv $(VENV_DIR) \
		&& $(SOURCE_VENV) \
		&& $(PYTHON_EXE) -m pip install -r $(VENV_DIR)/../requirements.txt \
		&& (adafruit-nrfutil version | grep -q 'adafruit-nrfutil version')) || \
		(echo; echo \
		 ; echo 'ERROR: python3 virtualenv seems to be not be functioning?' \
		 ; echo \
		 ; echo 'Troubleshooting:' \
		 ; echo ' - virtualenv created at "$(VENV_DIR)"' \
		 ; echo ' - "source $(VENV_DIR)/bin/activate" should make 'adafruit-nrfutil' available in $$PATH' \
		 ; echo ' - try "python3 -m pip install --break-system-packages --user adafruit-nrfutil" to force install' \
		 ; echo ' - packages like "python3-pip", "python3-venv" or "python3-wheel" may be needed locally' \
		 ; echo ' - expected "adafruit-nrfutil version" literal string in the following command output:' \
		 ; echo; echo ' $$ . $(VENV_DIR)/bin/activate && adafruit-nrfutil version' \
		 ; . $(VENV_DIR)/bin/activate && adafruit-nrfutil version \
		 ; echo \
		 ; false)

install-venv: $(VENV_DIR)/bin/activate
	@echo " --- ok: $@"
	@($(SOURCE_VENV) \
		&& adafruit-nrfutil version | grep -q 'adafruit-nrfutil version') || \
		(echo; echo \
		 ; echo 'ERROR: python3 virtualenv seems to be not be functioning?' \
		 ; echo \
		 ; echo 'Troubleshooting:' \
		 ; echo " - current shell is \"$$($$0 --version|head -n1|cut -f1 -d,)\" and must be \"GNU bash\" to support virtualenv" \
		 ; echo ' - try "make mr_proper install-venv" to reinstall potentially missing requirements.txt' \
		 ; echo ' - expected "adafruit-nrfutil version" literal string in the following command output:' \
		 ; echo; echo ' $$ . $(VENV_DIR)/bin/activate && adafruit-nrfutil version' \
		 ; . $(VENV_DIR)/bin/activate && adafruit-nrfutil version \
		 ; echo \
		 ; false)

# this target may break local IDE install!
clean-board-install: install-venv
	@echo; echo " --- $@"
	# remove (potentially modified) existing install...
	@(test -d $(ARDUINO_BOARD) && rm -rf $(ARDUINO_BOARD)) \
		|| echo ' -> no existing install found'
	# install clean 'adafruit:nrf52' core to repair install...
	@$(ARDUINO_CLI) core install 'adafruit:nrf52' --additional-urls "$(ADAFRUIT_URL)"
	# uninstall 'adafruit:nrf52' immediately after to clean install...
	@$(ARDUINO_CLI) core uninstall 'adafruit:nrf52' \
		--additional-urls "$(ADAFRUIT_URL)" || echo ' -> nothing to remove'

# this target may break local IDE install!
unsafe-board-install: clean-board-install install-venv
	@echo; echo " --- $@"
	# install 'adafruit:nrf52' on a fresh clean install...
	@$(ARDUINO_CLI) core install 'adafruit:nrf52' --additional-urls "$(ADAFRUIT_URL)"
	# replacing board by custom repository...
	@rm -rf $(ARDUINO_BOARD)
	git clone --recurse-submodules $(CUSTOM_BOARD_URL) $(ARDUINO_BOARD)

# this target may break local IDE install!
unsafe-install-libs: install-venv
	@echo " --- $@"
	# installing Adafruit NeoPixel
	@$(ARDUINO_CLI) lib install "Adafruit NeoPixel"
	# installing arduinoFFT
	@$(ARDUINO_CLI) lib install "arduinoFFT"

local-arduino-cli:
	@echo " --- $@"
	# enabling local arduino-cli command
	@cp $(TOOLS_DIR)/arduino-cli $(SRC_DIR)
	# adding local arduino-cli to .gitignore
	# @echo 'arduino-cli' >> $(SRC_DIR).gitignore
	# @echo '.gitignore' >> $(SRC_DIR).gitignore

arduino-cli-download:
	mkdir -p $(ARDUINO_LOC)
	# attempting to download arduino-cli in a local directory
	@cd $(ARDUINO_LOC) \
		&& curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=$(ARDUINO_LOC) /bin/sh
	@echo 'WARNING: on some distributions, such local arduino-cli binaries may not have network access!'

safe-install: local-arduino-cli
	@echo " --- $@"
	make unsafe-board-install unsafe-install-libs
	make check-arduino-deps
	rm -rf $(ARDUINO_LOC)/downloads

#
# clean build
#  - detects arduino's way of building the user sketch
#

$(BUILD_DIR)/.gitignore:
	@echo; echo " --- $@"
	# check if build directory is available...
	@mkdir -p $(BUILD_DIR)
	@echo "*" > $(BUILD_DIR)/.gitignore

$(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt: has-lamp-type check-arduino-deps $(BUILD_DIR)/.gitignore
	@echo; echo " --- $@"
	# read build properties to file if needed...
	@test -f $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt || (\
		$(ARDUINO_CLI) compile -b $(FQBN) \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "$(ARDUINO_EXTRA_FLAGS)" \
			--show-properties > $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt)

build-clean: has-lamp-type $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt
	@echo; echo " --- $@"
	# build "clean" project to detect arduino-cli behavior...
	@mkdir -p $(OBJECTS_DIR) $(CACHE_DIR)
	@(test ! -e $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean \
		&& (rm -f $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean \
			&& ($(ARDUINO_CLI) compile -b $(FQBN) --clean -v \
				--log-level=trace --log-file $(BUILD_DIR)/build-clean-log.txt \
				--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
				--build-property "$(ARDUINO_EXTRA_FLAGS)" \
		| tee $(BUILD_DIR)/verbose-clean.txt \
		| grep -v "$(COMPILER_PATH)") \
		&& touch $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean)) \
	|| echo '->' using existing verbose-clean.txt log as reference

#
# generate scripts used to build with custom flags
#  - use verbose output from build-clean to detect arduino behaviors
#

$(BUILD_DIR)/verbose-clean.txt: build-clean

$(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh: has-lamp-type $(BUILD_DIR)/verbose-clean.txt has-lamp-type
	@echo; echo " --- $@"
	# generating process.sh script from verbose-clean.txt log...
	@echo '#!/bin/bash' > $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'rm -f $(BUILD_DIR)/.process-${LMBD_LAMP_TYPE}-success' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'set -xe' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@tac _build/verbose-clean.txt \
		| grep -Eo '^.*bin/$(COMPILER_CMD).*-std=gnu\+\+11.*-o [^ ]*_build/objs/sketch/src[^ ]*.cpp.o' \
		| sed 's#-DLMBD_LAMP_TYPE[^ ]*##g' \
		| sed 's#-std=gnu++11#$(CPP_BUILD_FLAGS) $$LMBD_CPP_EXTRA_FLAGS#g' \
	>> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@test -z "${LMBD_JOBS}" || ( \
		sed -i 's#^.*bin/arm-none-eabi-g++.*.cpp.o$$#(& || kill $$PPID) \&#' $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh ; \
		echo 'wait || exit 1' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh)
	@echo 'touch $(BUILD_DIR)/objs/.last-used' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'touch $(BUILD_DIR)/.process-${LMBD_LAMP_TYPE}-success' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@test 8 -le $$(wc -l $@|cut -f 1 -d ' ') \
		|| (echo; echo \
			; echo 'ERROR: process-${LMBD_LAMP_TYPE}.sh is too short to be correct!' \
			; echo \
			; echo 'Troubleshooting:' \
			; echo ' - does "LMBD_LAMP_TYPE=${LMBD_LAMP_TYPE} make clean build-clean" returns with no error?' \
			; echo ' - if not, behavior detection may fail, as build can not be completed!' \
			; echo \
			; false)

$(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh: has-lamp-type $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo; echo " --- $@"
	# generating clear.sh script from process.sh script...
	@cat _build/process-${LMBD_LAMP_TYPE}.sh \
		| grep -v ' -E -o' | grep -v 'touch /' \
		| sed 's#^.*-o \([^ ]*/sketch/src/[^ ]*.cpp.o\)$$#rm -f \1#' \
	> $(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh

process-clear: has-lamp-type $(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh
	@echo; echo " --- $@"
	@test -e $(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh
	# clearing previous builds...
	@bash _build/clear-${LMBD_LAMP_TYPE}.sh > /dev/null 2>&1

#
# build process
#  - first do a "dry build" to inform adruino about which source files changed
#  - then do our custom "process" step to build our own objects for the sketch
#  - finally let arduino build the final artifact using our cached objects
#

build-dry: build-clean install-venv has-lamp-type
	# refresh cache before processing...
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR)
	@$(ARDUINO_CLI) compile -b $(FQBN) \
			--only-compilation-database \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "$(ARDUINO_EXTRA_FLAGS)" \
		> /dev/null 2>&1 || true

process: has-lamp-type build-dry process-clear $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo; echo " --- $@"
	@test -e $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@find $(BUILD_DIR)/objs/sketch -iname '*.cpp.o' -exec rm '{}' \; \
		; # (this can replace process-clear on paper?)
	# compiling sketch objects using process.sh script...
	@bash _build/process-${LMBD_LAMP_TYPE}.sh 2>&1 /dev/stdout \
		| tee _build/process-log.txt \
		| grep -v ' -E -o' \
	    | sed 's#^+ .*$(COMPILER_CMD).*sketch/\(src/.*.cpp.o\)$$#building \1...#g'

build: has-lamp-type process $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt
	@echo; echo " --- $@"
	# checking if sketch object preprocessing was successful...
	@test -e $(BUILD_DIR)/.process-${LMBD_LAMP_TYPE}-success || (echo '->' error happened! && false)
	# building project...
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR) $(COMPILED_UF2)
	@$(ARDUINO_CLI) compile -b $(FQBN) -e -v \
			--log-level=trace --log-file $(BUILD_DIR)/build-log.txt \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "$(ARDUINO_EXTRA_FLAGS)" \
			--output-dir $(ARTIFACTS) \
		| tee $(BUILD_DIR)/verbose.txt \
		| grep -v "$(COMPILER_PATH)" \
		| grep -v "Using cached library dependencies" \
		| grep -v "Using previously compiled" \
	# verifying if cached sketch objects were used...
	@(grep -q -E ' -o [^ ]*/_build/objs/sketch/src/[^ ]*.cpp.o' $(BUILD_DIR)/verbose.txt \
		&& echo && echo \
		&& echo "ERROR: arduino-cli rebuilt sketch without our custom setup :(" \
		&& echo && echo \
	) || echo '-> everything went fine!'
	# exporting artifacts...
	@test -f $(OBJECTS_DIR)/*.ino.zip || (test ! -f $(OBJECTS_DIR)/*.ino.hex \
		|| (echo; echo \
			; echo 'ERROR: looks like "adafruit-nrfutil dfu genpkg" command failed to run?' \
			; echo \
			; echo 'Troubleshooting:' \
			; echo ' - check in arduino logs that "adafruit-nrfutil" command was found in $$PATH' \
			; echo ' - if not, verify that "adafruit-nrfutil" is installed in "$(VENV_DIR)"' \
			; echo ' - if not, try the following to force a local install of the package:' \
			; echo '        python3 -m pip install --break-system-packages --user adafruit-nrfutil' \
			; echo ' - you can also try to run by hand the following:' \
			; echo -n '        '; grep 'dfu genpkg' $(BUILD_DIR)/verbose.txt \
			; echo \
			; false))
	@cp $(OBJECTS_DIR)/*.ino* $(ARTIFACTS)/

	@python scripts/uf2conv.py $(ARTIFACTS)/$(PROJECT_INO).hex -c -f 0xADA52840 -o $(COMPILED_UF2)/${LMBD_LAMP_TYPE}.uf2

#
# Format the code base using the given clang-format file
#

format:
	find $(PROJECT_INO) | xargs clang-format --style=file -i
	find src/ -iname '*.h' -o -iname '*.cpp' -o -iname '*.hpp' -o -iname '*.c' | xargs clang-format --style=file -i
	find simulator/include simulator/src simulator/mocks -iname '*.h' -o -iname '*.cpp' -o -iname '*.hpp' | xargs clang-format --style=file -i
	(which dos2unix > /dev/null) && \
		find src simulator -type f -regex '.*.[hc]p?p?' -exec dos2unix -q -e '{}' \; || echo "(dos2unix not found/failed)"

format-hook:
	cp .pre-commit .git/hooks/pre-commit

format-verify:
	@which clang-format > /dev/null \
		|| (echo; echo Install clang / clang-format to verify format!)
	@find $(PROJECT_INO)| xargs clang-format --style=file --dry-run --Werror
	@find src/ -iname '*.h' -o -iname '*.cpp' -o -iname '*.hpp' -o -iname '*.c' | xargs clang-format --style=file --dry-run -Werror
	@find simulator/ -iname '*.h' -o -iname '*.cpp' -o -iname '*.hpp' | xargs clang-format --style=file --dry-run -Werror
	@if ! grep -IUr "$$(printf '\r')" src; then true; else echo 'You are using CRLF (\\r\\n) in a POSIX project :('; false; fi
	# format is ok :)

#
# verify artifact
# 	- we check the presence of "canary string" in output artifact for sanity
#

$(ARTIFACTS)/$(PROJECT_INO).zip:
	@echo; echo " --- $@"
	# no artifact found, building it from scratch...
	make build

verify-canary: has-lamp-type $(ARTIFACTS)/$(PROJECT_INO).zip
	@echo; echo " --- $@"
	# verifying artifact canary...
	@(unzip -c $(ARTIFACTS)/$(PROJECT_INO).zip \
		| strings \
		| grep -q '_lmbd__build_canary__${LMBD_LAMP_TYPE}' \
		&& echo '-> canary found') \
	|| (echo; echo \
	    ; echo 'ERROR: string canary "_lmbd__build_canary__${LMBD_LAMP_TYPE}" not found in artifact!' \
		; echo \
	    ; echo \
		; echo 'Troubleshooting:' \
		; echo ' - was the artifact build for the ${LMBD_LAMP_TYPE} lamp type?' \
		; echo ' - try "make clean upload-${LMBD_LAMP_TYPE}" to rebuild from scratch' \
		; echo ' - try "make mr_proper" to remove everything if needed' \
		; echo \
		; false)

verify: verify-canary
	@echo " --- ok: $@"

verify-all-simulator:
	@echo; echo " --- $@"
	@mkdir -p $(BUILD_DIR) $(BUILD_DIR)/simulator
	# verify that simulator can be build, and if so, verify it...
	@cd $(SRC_DIR)/simulator && ( :\
		; export LMBD_ROOT_DIR=$(SRC_DIR) \
		; export SIMU_BUILD_DIR=$(BUILD_DIR)/simulator \
		; if make check-deps \
		; then make verify-all || \
			(echo; echo 'ERROR: simulator verify-all failed!'; false) \
		; else echo; echo 'Simulator dependencies not installed, skipping...' \
		; fi)

verify-type(%):
	@echo " --- $@($%)"
	@echo; echo Building with LMBD_LAMP_TYPE=$% to verify artifact...; echo
	LMBD_LAMP_TYPE=$% make build verify

verify-all: format-verify verify-all-simulator
	@echo; echo " --- $@"
	# verify that all lamp types build & validates
	make clean 'verify-type(indexable)'
	@touch $(BUILD_DIR)/.skip-simple-clean # (re-use indexable setup)
	make 'verify-type(simple)'
	@touch $(BUILD_DIR)/.skip-cct-clean # (re-use indexable setup)
	make 'verify-type(cct)'
	@echo; echo 'Everything went fine :)'

#
# to customize upload port:
# 	LMBD_SERIAL_PORT=/dev/ttyACM2 make # (default is /dev/ttyACM0)
#

upload: has-lamp-type verify install-venv
	@echo; echo " --- $@"
	@export LMBD_UPLOAD_PORT="$${LMBD_SERIAL_PORT:-/dev/ttyACM0}" \
		&& $(ARDUINO_CLI) upload -b $(FQBN) -v -t -l serial -p "$$LMBD_UPLOAD_PORT" --input-dir '$(ARTIFACTS)'

#
# monitor
#

RLWRAP=$(shell test -f /usr/bin/rlwrap && echo '/usr/bin/rlwrap' || echo ':;')

monitor: install-venv
	@echo; echo " --- $@"
	@export LMBD_MONITOR_PORT="$${LMBD_SERIAL_PORT:-/dev/ttyACM0}" \
		&& $(RLWRAP) $(ARDUINO_CLI_DETECTED) monitor -p "$$LMBD_MONITOR_PORT" --config 115200

#
# cleanup
#

clean-artifacts:
	@echo; echo " --- $@"
	rm -rf $(ARTIFACTS)

clean-doc:
	@echo; echo " --- $@"
	rm -f docs/html/index.html

clean: clean-artifacts clean-simulator clean-doc
	@echo; echo " --- $@"
	rm -rf $(OBJECTS_DIR) $(CACHE_DIR) $(BUILD_DIR)/*.txt
	rm -rf $(BUILD_DIR)/.process-*-success $(BUILD_DIR)/.skip-*-clean

#
# simulator
#

.PRECIOUS: $(BUILD_DIR)/simulator/%-simulator

$(BUILD_DIR)/simulator/%-simulator:
	@echo; echo " --- $@"
	@mkdir -p $(BUILD_DIR) $(BUILD_DIR)/simulator
	@cd $(SRC_DIR)/simulator && \
		LMBD_ROOT_DIR=$(SRC_DIR) SIMU_BUILD_DIR=$(BUILD_DIR)/simulator make $(shell basename "$@")

clean-simulator:
	@echo; echo " --- $@"
	@test -e $(BUILD_DIR)/simulator/Makefile \
		&& (cd $(BUILD_DIR)/simulator && make clean) \
		|| (rm -rf $(BUILD_DIR)/simulator)

%-simulator: $(BUILD_DIR)/simulator/%-simulator
	@echo " --- ok: $@"
	@test -x '$<' \
		&& (echo 'Artifact is ready here:'; echo '$<'; echo) \
		|| (echo 'No artifact found, build failed?'; rm -f '$<')

simulator: indexable-simulator simple-simulator
	@echo " --- ok: $@"


clean-tests:
	@echo; echo " --- $@"
	@test -e $(BUILD_DIR)/tests/Makefile \
		&& (cd $(BUILD_DIR)/tests && make clean) \
		|| (rm -rf $(BUILD_DIR)/tests)

test:
	@echo; echo " --- $@"
	@mkdir -p $(BUILD_DIR)/tests
	@cd $(SRC_DIR)/tests && \
		LMBD_ROOT_DIR=$(SRC_DIR) TEST_BUILD_DIR=$(BUILD_DIR)/tests make $(shell basename "tests")

#
# remove
#

remove: mr_proper

mr_proper: has-correct-folder-name
	@echo; echo " --- $@"
	@(test ! -L venv && rm -rf venv) || true
	rm -rf docs/html/*
	rm -rf _build
	rm -rf arduino-cli
