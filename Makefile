SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
VENV_DIR=$(SRC_DIR)venv
BUILD_DIR=$(SRC_DIR)_build
CACHE_DIR=$(BUILD_DIR)/cache
OBJECTS_DIR=$(BUILD_DIR)/objs
ARTIFACTS=$(BUILD_DIR)/artifacts

# SHELL=/bin/bash
PYTHON_EXE=/usr/bin/env python3
SOURCE_VENV=. $(VENV_DIR)/bin/activate
ARDUINO_CLI=$(SOURCE_VENV) && $(shell which arduino-cli)
ARDUINO_USER=$(shell $(ARDUINO_CLI) config get directories.user)
ARDUINO_DATA=$(shell $(ARDUINO_CLI) config get directories.data)
ARDUINO_BOARD=$(ARDUINO_DATA)/packages/adafruit/hardware/nrf52
ADAFRUIT_URL=https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
CUSTOM_BOARD_URL=https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino
FULL_LAMP_TYPE=LMBD_LAMP_TYPE__$(shell echo ${LMBD_LAMP_TYPE} | tr '[:lower:]' '[:upper:]')

FQBN=adafruit:nrf52:lampDa_nrf52840
PROJECT_INO=LampColorControler.ino
COMPILER_CMD=$(shell $(ARDUINO_CLI) compile -b $(FQBN) --show-properties|grep compiler.cpp.cmd|cut -f2 -d=)
COMPILER_PATH=$(shell $(ARDUINO_CLI) compile -b $(FQBN) --show-properties|grep compiler.path|cut -f2 -d=)
CPP_BASIC_FLAGS=-std=gnu++17 -fconcepts -D$(FULL_LAMP_TYPE) -DLMBD_EXPLICIT_CPP17_SUPPORT
CPP_BUILD_FLAGS=-fdiagnostics-color=always -Wno-unused-parameter -ftemplate-backtrace-limit=1
#
# to enable warnings:
# 	LMBD_CPP_EXTRA_FLAGS="-Wall -Wextra" make

all: build

#
# to pick a lamp flavor:
# 	LMBD_LAMP_TYPE="indexable" make build
#

cct: build-cct

simple: build-simple

indexable: build-indexable

build-cct:
	LMBD_LAMP_TYPE="cct" make build

build-simple:
	LMBD_LAMP_TYPE="simple" make build

build-indexable:
	LMBD_LAMP_TYPE="indexable" make build

upload-cct:
	LMBD_LAMP_TYPE="cct" make upload

upload-simple:
	LMBD_LAMP_TYPE="simple" make upload

upload-indexable:
	LMBD_LAMP_TYPE="indexable" make upload

has-lamp-type:
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

has-board-installed: install-venv
	@echo; echo " --- $@"
	@($(ARDUINO_CLI) board search '$(FQBN)' \
		| grep -q 'LampDa') || \
		(echo; echo\
		 ; echo 'ERROR: board "$(FQBN)" not found!' \
		 ; echo \
		 ; echo 'Troubleshooting:' \
		 ; echo ' - home directory is "${HOME}"' \
		 ; echo ' - detected arduino data directory is "$(ARDUINO_DATA)"' \
		 ; echo ' - adafruit should be listed in "$(ARDUINO_DATA)/packages" below:' \
		 ; echo -n '        '; ls '$(ARDUINO_DATA)/packages' \
		 ; echo ' - nrf52 should be listed in "$(ARDUINO_DATA)/packages/hardware" below:' \
		 ; echo -n '        '; ls '$(ARDUINO_DATA)/packages/adafruit/hardware' \
		 ; echo ' - have you replaced it by the "custom LampDa platform" repository?' \
		 ; echo ' - if yes, the "LampDa_nrf52840" board should be listed below:'; echo \
		 ; find $(ARDUINO_DATA)/packages/adafruit/hardware/nrf52 -name 'boards.txt' -exec cat '{}' \; \
			| grep 'nrf52840.build.variant' \
		 ; echo; echo ' - you can also try "make unsafe-board-install" (may break local IDE install!)' \
		 ; echo; echo \
		 ; false)

# this target may break local IDE install!
unsafe-install-libs: install-venv
	@echo " --- $@"
	# installing Adafruit NeoPixel
	@$(ARDUINO_CLI) lib install "Adafruit NeoPixel"
	# installing arduinoFFT
	@$(ARDUINO_CLI) lib install "arduinoFFT"

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
		 ; echo ' 		 make unsafe-install-libs # (may break local IDE install!)' \
		 ; echo '        arduino-cli lib install "$%"' \
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
		 ; echo ' - package like "gcc-arm-none-eabi" or "arm-none-eabi-gcc" may be needed locally' \
		 ; echo ' - you can also try "make unsafe-board-install" (may break local IDE install!)' \
		 ; echo \
		 ; false)

check-arduino-deps: has-board-installed check-compiler-cmd has-libs-installed(Adafruit\ NeoPixel) has-libs-installed(arduinoFFT)
	@echo " --- ok: $@"

#
# make doc
#

$(SRC_DIR)/doc/index.html:
	@echo; echo " --- $@"
	cd $(SRC_DIR) && doxygen doxygen.conf

doc: $(SRC_DIR)/doc/index.html
	@echo " --- ok: $@"

$(VENV_DIR)/bin/activate:
	@echo; echo " --- $@"
	$(PYTHON_EXE) -m venv $(VENV_DIR)
	$(SOURCE_VENV) && $(PYTHON_EXE) -m pip install -r $(VENV_DIR)/../requirements.txt

#
# clean build
#  - detects arduino's way of building the user sketch
#

install-venv: $(VENV_DIR)/bin/activate
	@echo " --- ok: $@"

$(BUILD_DIR)/.gitignore:
	@echo; echo " --- $@"
	# check if build directory is available...
	@mkdir -p $(BUILD_DIR)
	@echo "*" > $(BUILD_DIR)/.gitignore

$(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt: has-lamp-type check-arduino-deps $(BUILD_DIR)/.gitignore
	@echo; echo " --- $@"
	# read build properties to file...
	@$(ARDUINO_CLI) compile -b $(FQBN) \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "compiler.cpp.extra_flags=-D$(FULL_LAMP_TYPE)" \
			--show-properties > $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt

build-clean: has-lamp-type $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt
	@echo; echo " --- $@"
	# build "clean" project to detect arduino-cli behavior...
	@mkdir -p $(OBJECTS_DIR) $(CACHE_DIR)
	@(test ! -e $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean \
		&& (rm -f $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean \
			&& ($(ARDUINO_CLI) compile -b $(FQBN) --clean -v \
				--log-level=trace --log-file $(BUILD_DIR)/build-clean-log.txt \
				--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
				--build-property "compiler.cpp.extra_flags=-D$(FULL_LAMP_TYPE)" \
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
		| sed 's/-std=gnu++11/$(CPP_BASIC_FLAGS) $(CPP_BUILD_FLAGS) $$LMBD_CPP_EXTRA_FLAGS/g' \
	>> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'touch $(BUILD_DIR)/objs/.last-used' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'touch $(BUILD_DIR)/.process-${LMBD_LAMP_TYPE}-success' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh

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
	# removing existing sketch objects...
	@bash _build/clear-${LMBD_LAMP_TYPE}.sh > /dev/null 2>&1

#
# build process
#  - first do a "dry build" to inform adruino about which source files changed
#  - then do our custom "process" step to build our own objects for the sketch
#  - finally let arduino build the final artifact using our cached objects
#

build-dry: build-clean install-venv has-lamp-type
	# rebuild files to refresh cache before processing...
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR)
	@$(ARDUINO_CLI) compile -b $(FQBN) \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "compiler.cpp.extra_flags=-D$(FULL_LAMP_TYPE)" \
			> /dev/null 2>&1 || true

process: has-lamp-type build-dry process-clear $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo; echo " --- $@"
	@test -e $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
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
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR)
	@$(ARDUINO_CLI) compile -b $(FQBN) -e -v \
			--log-level=trace --log-file $(BUILD_DIR)/build-log.txt \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "compiler.cpp.extra_flags=-D$(FULL_LAMP_TYPE)" \
			--output-dir $(ARTIFACTS) \
		| tee $(BUILD_DIR)/verbose.txt \
		| grep -v "$(COMPILER_PATH)" \
		| grep -v "Using cached library dependencies" \
		| grep -v "Using previously compiled" \
	# verifying if cached sketch objects were used...
	@(grep -q -E ' -o [^ ]*/_build/objs/sketch/src/[^ ]*.cpp.o' $(BUILD_DIR)/verbose.txt \
		&& echo && echo \
		&& echo "ERROR: arduino-cli rebuilt sketch without our custom setup!" \
		&& echo && echo \
	) || echo '-> everything went fine!'
	# exporting artifacts...
	@cp $(OBJECTS_DIR)/*.ino* $(ARTIFACTS)/

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

#
# to customize upload port:
# 	LMBD_SERIAL_PORT=/dev/ttyACM2 make # (default is /dev/ttyACM0)
#

upload: has-lamp-type verify install-venv
	@echo; echo " --- $@"
	@export LMBD_UPLOAD_PORT="$${LMBD_SERIAL_PORT:-/dev/ttyACM0}" \
		&& $(ARDUINO_CLI) upload -b $(FQBN) -v -t -l serial -p "$$LMBD_UPLOAD_PORT" --input-dir '$(ARTIFACTS)'

#
# cleanup
#

clean-artifacts:
	@echo; echo " --- $@"
	rm -rf $(ARTIFACTS)

clean-doc:
	@echo; echo " --- $@"
	rm -f doc/index.html

clean: clean-artifacts clean-doc
	@echo; echo " --- $@"
	rm -rf $(OBJECTS_DIR) $(CACHE_DIR) $(BUILD_DIR)/*.txt
	rm -rf $(BUILD_DIR)/.process-*-success $(BUILD_DIR)/.skip-*-clean

#
# remove
#

remove: mr_proper

mr_proper:
	@echo; echo " --- $@"
	@(test ! -L venv && rm -rf venv) || true
	rm -rf doc/*
	rm -rf _build
