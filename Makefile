SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
VENV_DIR=$(SRC_DIR)venv
BUILD_DIR=$(SRC_DIR)_build
CACHE_DIR=$(BUILD_DIR)/cache
OBJECTS_DIR=$(BUILD_DIR)/objs
ARTIFACTS=$(BUILD_DIR)/artifacts
FQBN=adafruit:nrf52:lampDa_nrf52840
PROJECT_INO=LampColorControler.ino
CPP_STD_FLAGS=-std=gnu++17 -fconcepts -DLMBD_EXPLICIT_CPP17_SUPPORT
CPP_BUILD_FLAGS=-fdiagnostics-color=always -Wno-unused-parameter -ftemplate-backtrace-limit=1
#
# to enable warnings:
# 	LMBD_CPP_EXTRA_FLAGS="-Wall -Wextra" make

all: doc build

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
	@echo -e "\n --- $@"
	@test ! -z "$$LMBD_LAMP_TYPE" || \
		(echo -e '\n\nERROR: unspecified flavor!\n\nTo build, either use:'; \
		 echo '    make cct'; \
		 echo '    make simple'; \
		 echo '    make indexable'; \
		 echo -e '\nTo upload, either use:'; \
		 echo '    make upload-cct'; \
		 echo '    make upload-simple'; \
		 echo '    make upload-indexable'; \
		 echo -e '\nOr specify flavor manually:'; \
		 echo -e '    LMBD_LAMP_TYPE='indexable' make process\n\n' \
		; false)

#
# make doc
#

$(SRC_DIR)/doc/index.html:
	@echo -e "\n --- $@"
	cd $(SRC_DIR) && doxygen doxygen.conf

doc: $(SRC_DIR)/doc/index.html
	@echo -e " --- ok: $@"

$(VENV_DIR)/bin/activate:
	@echo -e "\n --- $@"
	python -m venv $(VENV_DIR)
	source $(VENV_DIR)/bin/activate && pip install -r $(VENV_DIR)/../requirements.txt

#
# clean build
#  - detects arduino's way of building the user sketch
#

install-venv: $(VENV_DIR)/bin/activate
	@echo -e " --- ok: $@"

$(BUILD_DIR)/.gitignore:
	@echo -e "\n --- $@"
	# check if build directory is available...
	@mkdir -p $(BUILD_DIR)
	@echo "*" > $(BUILD_DIR)/.gitignore

$(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt: has-lamp-type install-venv $(BUILD_DIR)/.gitignore
	@echo -e "\n --- $@"
	# read build properties to file...
	@source $(VENV_DIR)/bin/activate \
		&& declare -u LMBD_LAMP_TYPE=${LMBD_LAMP_TYPE} \
		&& arduino-cli compile -b $(FQBN) \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "compiler.cpp.extra_flags=-DLMBD_LAMP_TYPE__$$LMBD_LAMP_TYPE" \
			--show-properties > $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt

build-clean: has-lamp-type $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt
	@echo -e "\n --- $@"
	# build "clean" project to detect arduino-cli behavior...
	@mkdir -p $(OBJECTS_DIR) $(CACHE_DIR)
	@(test ! -e $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean \
		&& (rm -f $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean \
			&& source $(VENV_DIR)/bin/activate \
			&& declare -u LMBD_LAMP_TYPE=${LMBD_LAMP_TYPE} \
			&& (arduino-cli compile -b $(FQBN) --clean -v \
				--log-level=trace --log-file $(BUILD_DIR)/build-clean-log.txt \
				--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
				--build-property "compiler.cpp.extra_flags=-DLMBD_LAMP_TYPE__$$LMBD_LAMP_TYPE" \
		| tee $(BUILD_DIR)/verbose-clean.txt \
		| grep -v "/bin/arm-none-eabi") \
		&& touch $(BUILD_DIR)/.skip-${LMBD_LAMP_TYPE}-clean)) \
	|| echo '->' using existing verbose-clean.txt log as reference

#
# generate scripts used to build with custom flags
#  - use verbose output from build-clean to detect arduino behaviors
#

$(BUILD_DIR)/verbose-clean.txt: build-clean

$(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh: has-lamp-type $(BUILD_DIR)/verbose-clean.txt has-lamp-type
	@echo -e "\n --- $@"
	# generating process.sh script from verbose-clean.txt log...
	@echo '#!/bin/bash' > $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'rm -f $(BUILD_DIR)/.process-${LMBD_LAMP_TYPE}-success' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'set -xe' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@declare -u LMBD_LAMP_TYPE=${LMBD_LAMP_TYPE} \
		&& tac _build/verbose-clean.txt \
		| grep -Eo '^.*bin/arm-none-eabi-g++.*-std=gnu\+\+11.*-o [^ ]*_build/objs/sketch/src[^ ]*.cpp.o' \
		| sed 's/-std=gnu++11/$(CPP_STD_FLAGS) $(CPP_BUILD_FLAGS) $$LMBD_CPP_EXTRA_FLAGS -DLMBD_LAMP_TYPE__$$LMBD_LAMP_TYPE/g' \
	>> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'touch $(BUILD_DIR)/objs/.last-used' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo 'touch $(BUILD_DIR)/.process-${LMBD_LAMP_TYPE}-success' >> $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh

$(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh: has-lamp-type $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo -e "\n --- $@"
	# generating clear.sh script from process.sh script...
	@cat _build/process-${LMBD_LAMP_TYPE}.sh \
		| grep -v ' -E -o' | grep -v 'touch /' \
		| sed 's#^.*-o \([^ ]*/sketch/src/[^ ]*.cpp.o\)$$#rm -f \1#' \
	> $(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh

process-clear: has-lamp-type $(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh
	@echo -e "\n --- $@"
	@test -e $(BUILD_DIR)/clear-${LMBD_LAMP_TYPE}.sh
	# removing existing sketch objects...
	@bash _build/clear-${LMBD_LAMP_TYPE}.sh &> /dev/null

#
# build process
#  - first do a "dry build" to inform adruino about which source files changed
#  - then do our custom "process" step to build our own objects for the sketch
#  - finally let arduino build the final artifact using our cached objects
#

build-dry: build-clean install-venv has-lamp-type
	# rebuild files to refresh cache before processing...
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR)
	@source $(VENV_DIR)/bin/activate \
		&& declare -u LMBD_LAMP_TYPE=${LMBD_LAMP_TYPE} \
		&& arduino-cli compile -b $(FQBN) \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "compiler.cpp.extra_flags=-DLMBD_LAMP_TYPE__$$LMBD_LAMP_TYPE" \
			&> /dev/null || true

process: has-lamp-type build-dry process-clear $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	@echo -e "\n --- $@"
	@test -e $(BUILD_DIR)/process-${LMBD_LAMP_TYPE}.sh
	# compiling sketch objects using process.sh script...
	@bash _build/process-${LMBD_LAMP_TYPE}.sh &> /dev/stdout \
		| tee _build/process-log.txt \
		| grep -v ' -E -o' \
	    | sed 's#^+ .*arm-none-eabi-g.*sketch/\(src/.*.cpp.o\)$$#building \1...#g'

build: has-lamp-type process $(BUILD_DIR)/properties-${LMBD_LAMP_TYPE}.txt
	@echo -e "\n --- $@"
	# checking if sketch object preprocessing was successful...
	@test -e $(BUILD_DIR)/.process-${LMBD_LAMP_TYPE}-success || (echo '->' error happened! && false)
	# building project...
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR)
	@source $(VENV_DIR)/bin/activate \
		&& declare -u LMBD_LAMP_TYPE=${LMBD_LAMP_TYPE} \
		&& arduino-cli compile -b $(FQBN) -e -v \
			--log-level=trace --log-file $(BUILD_DIR)/build-log.txt \
			--build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
			--build-property "compiler.cpp.extra_flags=-DLMBD_LAMP_TYPE__$$LMBD_LAMP_TYPE" \
			--output-dir $(ARTIFACTS) \
		| tee $(BUILD_DIR)/verbose.txt \
		| grep -v "/bin/arm-none-eabi"
	# verifying if cached sketch objects were used...
	@(grep -q -E ' -o [^ ]*/_build/objs/sketch/src/[^ ]*.cpp.o' $(BUILD_DIR)/verbose.txt \
		&& echo -e "\n\nERROR: arduino-cli rebuilt sketch without our custom setup!\n\n" \
	) || echo '-> everything went fine!'
	# exporting artifacts...
	@cp $(OBJECTS_DIR)/*.ino* $(ARTIFACTS)/

#
# verify artifact
# 	- we check the presence of "canary string" in output artifact for sanity
#

$(ARTIFACTS)/$(PROJECT_INO).zip:
	@echo -e "\n --- $@"
	# no artifact found, building it from scratch...
	make build

verify-canary: has-lamp-type $(ARTIFACTS)/$(PROJECT_INO).zip
	@echo -e "\n --- $@"
	# verifying artifact canary...
	@(unzip -c $(ARTIFACTS)/$(PROJECT_INO).zip \
		| strings \
		| grep -q '_lmbd__build_canary__${LMBD_LAMP_TYPE}' \
		&& echo '-> canary found') \
	|| (echo -e '\n\nERROR: string canary "_lmbd__build_canary__${LMBD_LAMP_TYPE}" not found in artifact!\n\n' \
		; echo 'Troubleshooting:' \
		; echo ' - was the artifact build for the ${LMBD_LAMP_TYPE} lamp type?' \
		; echo ' - try "make clean upload-${LMBD_LAMP_TYPE}" to rebuild from scratch' \
		; echo -e ' - try "make mr_proper" to remove everything if needed\n' \
		; false)

verify: verify-canary
	@echo -e " --- ok: $@"

#
# to customize upload port:
# 	LMBD_SERIAL_PORT=/dev/ttyACM2 make # (default is /dev/ttyACM0)
#

upload: has-lamp-type verify install-venv
	@echo -e "\n --- $@"
	@source $(VENV_DIR)/bin/activate \
		&& export LMBD_UPLOAD_PORT="$${LMBD_SERIAL_PORT:-/dev/ttyACM0}" \
		&& arduino-cli upload -b $(FQBN) -v -t -l serial -p "$$LMBD_UPLOAD_PORT" --input-dir '$(ARTIFACTS)'

#
# cleanup
#

clean-artifacts:
	@echo -e "\n --- $@"
	rm -rf $(ARTIFACTS)

clean-doc:
	@echo -e "\n --- $@"
	rm -f doc/index.html

clean: clean-artifacts clean-doc
	@echo -e "\n --- $@"
	rm -rf $(OBJECTS_DIR) $(CACHE_DIR) $(BUILD_DIR)/*.txt
	rm -rf $(BUILD_DIR)/.process-*-success $(BUILD_DIR)/.skip-*-clean

#
# remove
#

remove: mr_proper

mr_proper:
	@echo -e "\n --- $@"
	@(test ! -L venv && rm -rf venv) || true
	rm -rf doc/*
	rm -rf _build
