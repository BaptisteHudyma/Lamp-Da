cmake_minimum_required(VERSION 3.12)
project(LampSimulator CXX C)

set(LMBD_ROOT_DIR $ENV{LMBD_ROOT_DIR} CACHE PATH "Root directory for the project")
if(LMBD_ROOT_DIR STREQUAL "")
    set(LMBD_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
endif()

set(SFML_VERSION 3.0.1)
set(SFML_FETCH 0 CACHE PATH "If not set, try to use local install of SFML")
if (NOT SFML_FETCH)
    find_package(SFML ${SFML_VERSION} COMPONENTS System Window Graphics Audio)
endif()

if (NOT SFML_FOUND)
    set(SFML_FETCH 1)
    message(WARNING "No local installation of SFML ${SFML_VERSION} found!\n... cmake will try to build SFML locally :)")
endif()

if (SFML_FETCH)
    include(FetchContent)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG ${SFML_VERSION}
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(SFML)
endif()


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

# Create simulator as library
set(SRC_SYSTEM_UTILS
    ${LMBD_ROOT_DIR}/src/system/utils/print.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/coordinates.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/colorspace.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/brightness_handle.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/vector_math.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/utils.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/serial.cpp
)

set(SRC_SYSTEM_COLORS
    ${LMBD_ROOT_DIR}/src/system/colors/imuAnimations.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/animations.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/wipes.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/colors.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/soundAnimations.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/text.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/palettes.cpp
)

set(SRC_SYSTEM_POWER
    ${LMBD_ROOT_DIR}/src/system/power/power_handler.cpp
    ${LMBD_ROOT_DIR}/src/system/power/power_gates.cpp
    ${LMBD_ROOT_DIR}/src/system/power/balancer.cpp
    ${LMBD_ROOT_DIR}/src/system/power/PDlib/power_delivery.cpp
    ${LMBD_ROOT_DIR}/src/system/power/PDlib/drivers/tcpm_driver.cpp
    ${LMBD_ROOT_DIR}/src/system/power/charging_ic.cpp
    ${LMBD_ROOT_DIR}/src/system/power/charger.cpp
)

set(SRC_SYSTEM_PDLIB_C
    ${LMBD_ROOT_DIR}/src/system/power/PDlib/usb_pd_policy.c
    ${LMBD_ROOT_DIR}/src/system/power/PDlib/task.c
    ${LMBD_ROOT_DIR}/src/system/power/PDlib/usb_pd_protocol.c
    ${LMBD_ROOT_DIR}/src/system/power/PDlib/drivers/usb_pd_driver.c
    ${LMBD_ROOT_DIR}/src/system/power/PDlib/drivers/FUSB302.c
)

set(SRC_SYSTEM_PHYSICAL
    ${LMBD_ROOT_DIR}/src/system/physical/fileSystem.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/LSM6DS3/LSM6DS3.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/LSM6DS3/imu_wrapper.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/button.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/battery.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/output_power.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/sound.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/imu.cpp
    ${LMBD_ROOT_DIR}/src/system/physical/indicator.cpp
)

set(SRC_SYSTEM_EXT
    ${LMBD_ROOT_DIR}/src/system/ext/noise.cpp
)

set(SRC_SYSTEM_GLOBAL
    ${LMBD_ROOT_DIR}/src/system/alerts.cpp
    ${LMBD_ROOT_DIR}/src/system/behavior.cpp
    ${LMBD_ROOT_DIR}/src/system/global.cpp
)

set(SIMULATOR_MOCKS
    ${LMBD_ROOT_DIR}/simulator/mocks/print_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/mocks/pdm_handle_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/mocks/registers_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/mocks/time_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/mocks/i2c_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/mocks/gpio_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/mocks/threads.cpp
    ${LMBD_ROOT_DIR}/simulator/mocks/bluetooth_mock.cpp
)

# Simulator Targets
function(create_simulator_target SIM_NAME)
    string(TOUPPER "${SIM_NAME}" UPPER_SIM_NAME)

    set(TARGET_NAME ${SIM_NAME}-simulator)
    add_executable(${TARGET_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET_NAME}.cpp
    )
    target_compile_definitions(${TARGET_NAME} PUBLIC LMBD_LAMP_TYPE__${UPPER_SIM_NAME})

    add_library(simulator_${SIM_NAME} OBJECT
        ${SRC_SYSTEM_UTILS}
        ${SRC_SYSTEM_COLORS}
        ${SRC_SYSTEM_POWER}
        ${SRC_SYSTEM_PHYSICAL}
        ${SRC_SYSTEM_PDLIB_C}
        ${SRC_SYSTEM_EXT}
        ${SRC_SYSTEM_GLOBAL}
        ${SIMULATOR_MOCKS}
        ${LMBD_ROOT_DIR}/src/user/${SIM_NAME}_functions.cpp
    )
    target_compile_definitions(simulator_${SIM_NAME} PUBLIC LMBD_LAMP_TYPE__${UPPER_SIM_NAME})

    if (SFML_FETCH)
        target_link_libraries(${TARGET_NAME}
            simulator_${SIM_NAME}
            SFML::System
            SFML::Window
            SFML::Graphics
            SFML::Audio
            pthread
        )
    else()
        target_link_libraries(${TARGET_NAME}
            simulator_${SIM_NAME}
            sfml-graphics
            sfml-window
            sfml-audio
            sfml-system
            pthread
        )
    endif()

endfunction()

# Create simulator targets dynamically
create_simulator_target(indexable)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulation_parameters.txt
    ${CMAKE_CURRENT_BINARY_DIR}/simulation_parameters.txt
    COPYONLY
)
