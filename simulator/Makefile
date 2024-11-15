SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
ROOT_DIR=${LMBD_ROOT_DIR}
BUILD_DIR=${SIMU_BUILD_DIR}
#
# Required environment variables:
# 	- LMBD_ROOT_DIR root directory of the repository
# 	- SIMU_BUILD_DIR target build directory (named "simulator")

CXX=$(shell which c++)
CXX_INCLUDE_DIRECTORIES=-I$(ROOT_DIR) -I$(SRC_DIR)include
CXX_LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system
CXX_EXTRA_FLAGS=-fconcepts -DLMBD_CPP17 -ftemplate-backtrace-limit=1 #-Wall -Wextra -pedantic
CXX_BUILD_FLAGS=--std=c++17 -O3 $(CXX_EXTRA_FLAGS) $(CXX_INCLUDE_DIRECTORIES)

all: build

check-deps:
	@echo; echo " --- $@"
	# building test program linked with SFML...
	@echo 'int main(){}' > '$(BUILD_DIR)/.main.cpp'
	@($(CXX) $(CXX_LDFLAGS) '$(BUILD_DIR)/.main.cpp' -o /dev/zero) || \
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
	@test -d '$(BUILD_DIR)/../simulator' \
		|| (echo; echo 'Bad path: SIMU_BUILD_DIR='$$SIMU_BUILD_DIR \
			; echo 'Usage: SIMU_BUILD_DIR=../_build/simulator make' \
			; echo; false)
	@test -d '$(ROOT_DIR)/src/user' \
		|| (echo; echo 'Bad path: LMBD_ROOT_DIR='$$LMBD_ROOT_DIR \
			; echo 'Usage: LMBD_ROOT_DIR=../../LampColorControler make' \
			; echo; false)

.PRECIOUS: $(BUILD_DIR)/%-simulator

$(BUILD_DIR)/%-simulator: $(SRC_DIR)src/%-simulator.cpp
	@echo; echo " --- $@"
	$(CXX) $(CXX_LDFLAGS) $(CXX_BUILD_FLAGS) $< -o $@

%-simulator: check-dirs check-deps $(BUILD_DIR)/%-simulator
	@echo " --- ok: $@$%"

build: fire-simulator
	@echo " --- ok: $@"

clean:
	rm -f $(BUILD_DIR)/*-simulator
