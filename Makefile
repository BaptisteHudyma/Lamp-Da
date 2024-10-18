SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
VENV_DIR=$(SRC_DIR)venv
BUILD_DIR=$(SRC_DIR)_build
CACHE_DIR=$(BUILD_DIR)/cache
OBJECTS_DIR=$(BUILD_DIR)/objs
ARTIFACTS=$(BUILD_DIR)/artifacts
FQBN=adafruit:nrf52:lampDa_nrf52840

all: doc build

$(SRC_DIR)/doc/index.html:
	@echo -e "\n --- $@"
	cd $(SRC_DIR) && doxygen doxygen.conf

doc: $(SRC_DIR)/doc/index.html
	@echo -e " --- ok: $@"

$(VENV_DIR)/bin/activate:
	@echo -e "\n --- $@"
	python -m venv $(VENV_DIR)
	source $(VENV_DIR)/bin/activate && pip install -r $(VENV_DIR)/../requirements.txt

install-venv: $(VENV_DIR)/bin/activate
	@echo -e " --- ok: $@"

$(BUILD_DIR)/.gitignore:
	@echo -e "\n --- $@"
	# check if build directory is available...
	@mkdir -p $(BUILD_DIR)
	@echo "*" > $(BUILD_DIR)/.gitignore

$(BUILD_DIR)/properties.txt: install-venv $(BUILD_DIR)/.gitignore
	@echo -e "\n --- $@"
	# read build properties to file...
	@source $(VENV_DIR)/bin/activate \
		&& arduino-cli compile -b $(FQBN) --build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) --show-properties > $(BUILD_DIR)/properties.txt

build-clean: $(BUILD_DIR)/properties.txt
	@echo -e "\n --- $@"
	# build "clean" project to detect arduino-cli behavior...
	@mkdir -p $(OBJECTS_DIR) $(CACHE_DIR)
	@(test ! -e $(BUILD_DIR)/.skip-clean && (\
		rm -f $(BUILD_DIR)/.skip-clean && \
		source $(VENV_DIR)/bin/activate && \
		arduino-cli compile -b $(FQBN) --clean -v --log-level=trace --log-file $(BUILD_DIR)/build-clean-log.txt --build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
		| tee $(BUILD_DIR)/verbose-clean.txt \
		| grep -v "/bin/arm-none-eabi" \
		&& touch $(BUILD_DIR)/.skip-clean)) \
	|| echo '->' using existing verbose-clean.txt log as reference

$(BUILD_DIR)/verbose-clean.txt: build-clean

$(BUILD_DIR)/process.sh: $(BUILD_DIR)/verbose-clean.txt
	@echo -e "\n --- $@"
	# generating process.sh script from verbose-clean.txt log...
	@echo '#!/bin/bash' > $(BUILD_DIR)/process.sh
	@echo 'rm -f $(BUILD_DIR)/.process-success' >> $(BUILD_DIR)/process.sh
	@echo 'set -xe' >> $(BUILD_DIR)/process.sh
	@cat _build/verbose-clean.txt \
		| grep -Eo '^.*bin/arm-none-eabi-g++.*-std=gnu\+\+11.*-o [^ ]*_build/objs/sketch/src[^ ]*.cpp.o' \
		| sed 's/-std=gnu++11/-std=gnu++17 -DLMBD_EXPLICIT_CPP17_SUPPORT -fdiagnostics-color=always/g' \
	>> $(BUILD_DIR)/process.sh
	@echo 'touch $(BUILD_DIR)/objs/.last-used' >> $(BUILD_DIR)/process.sh
	@echo 'touch $(BUILD_DIR)/.process-success' >> $(BUILD_DIR)/process.sh

$(BUILD_DIR)/clear.sh: $(BUILD_DIR)/process.sh
	@echo -e "\n --- $@"
	# generating clear.sh script from process.sh script...
	@cat _build/process.sh \
		| grep -v ' -E -o' | grep -v 'touch /' \
		| sed 's#^.*-o \([^ ]*/sketch/src/[^ ]*.cpp.o\)$$#rm -f \1#' \
	> $(BUILD_DIR)/clear.sh

process-clear: $(BUILD_DIR)/clear.sh
	@echo -e "\n --- $@"
	@test -e $(BUILD_DIR)/clear.sh
	# removing existing sketch objects...
	@bash _build/clear.sh &> /dev/null

build-dry: install-venv
	# rebuild files to refresh cache before processing...
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR)
	@source $(VENV_DIR)/bin/activate \
		&& arduino-cli compile -b $(FQBN) --build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) &> /dev/null || true

process: build-dry process-clear $(BUILD_DIR)/process.sh
	@echo -e "\n --- $@"
	@test -e $(BUILD_DIR)/process.sh
	# compiling sketch objects using process.sh script...
	@bash _build/process.sh &> /dev/stdout \
		| tee _build/process-log.txt \
		| grep -v ' -E -o' \
	    | sed 's#^+ .*arm-none-eabi-g.*sketch/\(src/.*.cpp.o\)$$#building \1...#g'

build: process $(BUILD_DIR)/properties.txt
	@echo -e "\n --- $@"
	# checking if sketch object preprocessing was successful...
	@test -e $(BUILD_DIR)/.process-success || (echo '->' error happened! && false)
	# building project...
	@mkdir -p $(ARTIFACTS) $(OBJECTS_DIR) $(CACHE_DIR)
	@source $(VENV_DIR)/bin/activate \
		&& arduino-cli compile -b $(FQBN) -e -v --log-level=trace --log-file $(BUILD_DIR)/build-log.txt --output-dir $(ARTIFACTS) --build-path $(OBJECTS_DIR) --build-cache-path $(CACHE_DIR) \
		| tee $(BUILD_DIR)/verbose.txt \
		| grep -v "/bin/arm-none-eabi"
	# verifying if cached sketch objects were used...
	@(grep -q -E ' -o [^ ]*/_build/objs/sketch/src/[^ ]*.cpp.o' $(BUILD_DIR)/verbose.txt \
		&& echo -e "\n\nERROR: arduino-cli rebuilt sketch without c++17 support :'(\n\n" \
	) || echo '-> everything looks fine :)'
	# exporting artifacts...
	@cp $(OBJECTS_DIR)/*.ino* $(ARTIFACTS)/

clean: process-clear
	@echo -e "\n --- $@"
	rm -rf $(OBJECTS_DIR) $(CACHE_DIR) $(BUILD_DIR)/*.txt
	rm -rf $(BUILD_DIR)/.process-success $(BUILD_DIR)/.skip-clean

mr_proper:
	@echo -e "\n --- $@"
	rm -rf doc/*
	rm -rf _build
