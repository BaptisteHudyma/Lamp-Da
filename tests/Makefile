SRC_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
ROOT_DIR=${LMBD_ROOT_DIR}
BUILD_DIR=${TEST_BUILD_DIR}

check-dirs:
	@echo; echo " --- $@"
	@mkdir -p $(BUILD_DIR)
	@test -d '$(BUILD_DIR)/../tests' \
		|| (echo; echo 'Bad path: SIMU_BUILD_DIR='$$SIMU_BUILD_DIR \
			; echo 'Usage: SIMU_BUILD_DIR=../_build/tests make' \
			; echo; false)
	@test -d '$(ROOT_DIR)/src/user' \
		|| (echo; echo 'Bad path: LMBD_ROOT_DIR='$$LMBD_ROOT_DIR \
			; echo 'Usage: LMBD_ROOT_DIR=../../LampColorControler make' \
			; echo; false)


$(BUILD_DIR)/CMakeCache:
	@echo; echo " --- $@"
	cd $(BUILD_DIR) && cmake $(LMBD_ROOT_DIR)/tests -DLMBD_ROOT_DIR=$(LMBD_ROOT_DIR)

$(BUILD_DIR)/tests: $(BUILD_DIR)/CMakeCache
	@echo; echo " --- $@"
	cd $(BUILD_DIR) && make -j
	@echo " --- ok: $*"

tests: check-dirs $(BUILD_DIR)/tests
	@echo " --- ok: $@"

clean:
	rm -f $(BUILD_DIR)/utest/tests_utest
	cd $(ROOT_DIR) && make clean

mr_proper:
	rm -rf $(BUILD_DIR)
