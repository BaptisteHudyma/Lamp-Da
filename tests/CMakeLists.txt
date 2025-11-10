cmake_minimum_required(VERSION 3.16)
project(tests)

set(LMBD_ROOT_DIR $ENV{LMBD_ROOT_DIR} CACHE PATH "Root directory for the project")
if(LMBD_ROOT_DIR STREQUAL "")
    set(LMBD_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
endif()


set(TEST_BUILD_FLAGS "-O3 -g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TEST_BUILD_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TEST_BUILD_FLAGS} -fconcepts --template-backtrace-limit=1")


# Compiler and Flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler Definitions
add_compile_definitions(
    LMBD_MISSING_DEFINE
    LMBD_SIMULATION
    LMBD_IN_SIMULATION
    LMBD_CPP17
)

include_directories(
    ${LMBD_ROOT_DIR}/simulator/include
    ${LMBD_ROOT_DIR}
)

# include build tools
include(${LMBD_ROOT_DIR}/simulator/compile.cmake)

install_sfml()
create_simulator_target(indexable)


# Unit tests
add_subdirectory("utest")
