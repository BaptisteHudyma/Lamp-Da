SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
ROOT_DIR=${LMBD_ROOT_DIR}
BUILD_DIR=${SIMU_BUILD_DIR}
#
# Required environment variables:
# 	- LMBD_ROOT_DIR root directory of the repository
# 	- SIMU_BUILD_DIR target build directory (named "simulator")

check-deps:
	@echo; echo " --- $@"
	# building test program linked with SFML...
	@echo 'int main(){}' > '$(BUILD_DIR)/.main.cpp'
	@($(CXX) '$(BUILD_DIR)/.main.cpp' -o /dev/zero $(CXX_LDFLAGS)) || \
		(echo; echo\
		 ; echo 'ERROR: unable to link against the SFML library!' \
		 ; echo \
		 ; echo 'Troubleshooting:' \
		 ; echo ' - detected compiler is $(CXX_BIN)' \
		 ; echo ' - have you install the SFML development packages?'\
		 ; echo ' - for example, use:' \
		 ; echo '    - on Debian / Ubuntu: "apt-get install libsfml-dev"' \
		 ; echo '    - on Archlinux / Manjaro: "pacman -S sfml"' \
		 ; echo ' - listing /usr/lib for SFML is:' \
		 ; echo ; ls /usr/lib/libsfml-*.so \
		 ; echo \
		 ; false)

check-dirs:
	@echo; echo " --- $@"
	@mkdir -p $(BUILD_DIR)
	@test -d '$(BUILD_DIR)/../simulator' \
		|| (echo; echo 'Bad path: SIMU_BUILD_DIR='$$SIMU_BUILD_DIR \
			; echo 'Usage: SIMU_BUILD_DIR=../_build/simulator make' \
			; echo; false)
	@test -d '$(ROOT_DIR)/src/user' \
		|| (echo; echo 'Bad path: LMBD_ROOT_DIR='$$LMBD_ROOT_DIR \
			; echo 'Usage: LMBD_ROOT_DIR=../../LampColorControler make' \
			; echo; false)

.PRECIOUS: $(BUILD_DIR)/%-simulator

$(BUILD_DIR)/CMakeCache.txt:
	cd $(BUILD_DIR) && \
		cmake $(LMBD_ROOT_DIR)/simulator -DLMBD_ROOT_DIR=$(LMBD_ROOT_DIR)

$(BUILD_DIR)/%-simulator: $(BUILD_DIR)/CMakeCache.txt
	cd $(BUILD_DIR) && make -j $*-simulator
	@echo " --- ok: $*"

%-simulator: check-dirs check-deps $(BUILD_DIR)/%-simulator
	@echo " --- ok: $@$%"

build: indexable-simulator simple-simulator
	@echo " --- ok: $@"

verify-all: clean build
	@echo " --- ok: $@"

clean:
	rm -f $(BUILD_DIR)/*-simulator
	cd $(ROOT_DIR) && make clean

mr_proper:
	rm -rf $(BUILD_DIR)
