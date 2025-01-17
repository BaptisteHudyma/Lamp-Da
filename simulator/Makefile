SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
ROOT_DIR=${LMBD_ROOT_DIR}
BUILD_DIR=${SIMU_BUILD_DIR}
#
# Required environment variables:
# 	- LMBD_ROOT_DIR root directory of the repository
# 	- SIMU_BUILD_DIR target build directory (named "simulator")

CXX=$(shell which c++)
CXX_INCLUDE_DIRECTORIES=-I$(ROOT_DIR) -I$(SRC_DIR)include
CXX_LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system -pthread
CXX_LAMP_FLAGS=-DLMBD_MISSING_DEFINE -DLMBD_SIMULATION -DLMBD_LAMP_TYPE__INDEXABLE
CXX_EXTRA_FLAGS=-fconcepts -DLMBD_CPP17 -ftemplate-backtrace-limit=1 #-Wall -Wextra -pedantic
CXX_BUILD_FLAGS=--std=c++17 -O3 -g -fsanitize=address -fno-omit-frame-pointer -DCMAKE_EXPORT_COMPILE_COMMANDS=1 $(CXX_LAMP_FLAGS) $(CXX_EXTRA_FLAGS) $(CXX_INCLUDE_DIRECTORIES)


CPP_SRC_DIR = $(SRC_DIR)../src
#exclude platform specific folder
EXCLUDE_DIR = $(CPP_SRC_DIR)/system/platform
MOCK_DIR = $(SRC_DIR)mocks

INCLUDES = -I$(CPP_SRC_DIR)

SRC_CPPs = $(shell find $(CPP_SRC_DIR) -name "*.cpp" -not -path "$(EXCLUDE_DIR)/*")
SRC_Cs = $(shell find $(CPP_SRC_DIR) -name "*.c" -not -path "$(EXCLUDE_DIR)/*")
OBJ = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRC_CPPs)) $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_Cs))
SRC_MOCKS_CPPs = $(shell find $(MOCK_DIR) -name "*.cpp")
MOCK_OBJ = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRC_MOCKS_CPPs))

DEP = $(MOCK_OBJ:.o=.d) $(OBJ:.o=.d)  # Fichiers de dépendances (.d)

ALL_TARGETS=$(shell ls $(SRC_DIR)/src|grep -Eo '^[^-]*-simulator.cpp'|sed 's/.cpp$$//g')

all: build

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


$(BUILD_DIR)/%-simulator: $(SRC_DIR)src/%-simulator.cpp $(MOCK_OBJ) $(OBJ)
	@echo; echo " --- $@"
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_BUILD_FLAGS) -c $< -o $@.o $(CXX_LDFLAGS)
	$(CXX) $(CXX_BUILD_FLAGS) -o $@ $(MOCK_OBJ) $(OBJ) $@.o $(CXX_LDFLAGS)

# generer les fichiers objets
$(BUILD_DIR)/%.o: %.cpp %.d
	@echo; echo building $<
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_BUILD_FLAGS) -c $(INCLUDES) $< -o $@ $(CXX_LDFLAGS)

# generer les fichiers objets
$(BUILD_DIR)/%.o: %.c %.d
	@echo; echo building $<
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_BUILD_FLAGS) -c $(INCLUDES) $< -o $@ $(CXX_LDFLAGS)

# Générer des fichiers de dépendances
%.d: %.cpp
	@echo; echo linking $<
	@mkdir -p $(dir $@)
	g++ -MM -MG -MT $(CXX_LDFLAGS) $(CXX_BUILD_FLAGS) $(INCLUDES) $< > $@

# Générer des fichiers de dépendances
%.d: %.c
	@echo; echo linking $<
	@mkdir -p $(dir $@)
	g++ -MM -MG -MT $(CXX_LDFLAGS) $(CXX_BUILD_FLAGS) $(INCLUDES) $< > $@

# Inclure les fichiers de dépendances générés
-include $(DEP)


%-simulator: check-dirs check-deps $(BUILD_DIR)/%-simulator
	@echo " --- ok: $@$%"

build: indexable-simulator
	@echo " --- ok: $@"

verify-all: clean $(ALL_TARGETS)
	@echo " --- ok: $@"

clean:
	rm -f $(BUILD_DIR)/*-simulator
	rm -rf $(BUILD_DIR)
