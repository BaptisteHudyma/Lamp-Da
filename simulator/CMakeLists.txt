cmake_minimum_required(VERSION 3.16)
project(LampSimulator CXX C)

set(LMBD_ROOT_DIR $ENV{LMBD_ROOT_DIR} CACHE PATH "Root directory for the project")
if(LMBD_ROOT_DIR STREQUAL "")
    set(LMBD_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/compile.cmake)

install_sfml()

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

set(SIM_BUILD_FLAGS "-O3 -g -fsanitize=address -fno-omit-frame-pointer")
set(SIM_LINK_FLAGS "-fsanitize=address")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SIM_BUILD_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIM_BUILD_FLAGS} -fconcepts --template-backtrace-limit=1")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LMBD_ROOT_DIR}
)

# Create simulator targets dynamically
create_simulator_target(indexable)
create_simulator_target(simple)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/simulation_parameters.txt
    ${CMAKE_CURRENT_BINARY_DIR}/simulation_parameters.txt
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/DejaVuSansMono.ttf
    ${CMAKE_CURRENT_BINARY_DIR}/DejaVuSansMono.ttf
    COPYONLY
)
