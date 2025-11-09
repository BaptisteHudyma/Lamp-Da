function(install_sfml)
    # use local SFML2 install, if not available, use FetchContent to get 2.6.1
    set(SFML_VERSION 2)
    set(SFML_FETCH 0 CACHE PATH "If not set, try to use local install of SFML")
    if (NOT SFML_FETCH)
        find_package(SFML ${SFML_VERSION} COMPONENTS system window graphics audio)
    endif()

    # (not all SFML2 can't be rebuild on recent cmake)
    set(SFML_VERSION_LOCAL_BUILD 2.6.1)
    if (NOT SFML_FOUND)
        set(SFML_FETCH 1)
        message(WARNING "No local installation of SFML ${SFML_VERSION} found!\n... cmake will try to build SFML ${SFML_VERSION_LOCAL_BUILD} locally :)")
    endif()

    if (SFML_FETCH)
        set(BUILD_SHARED_LIBS OFF)
        include(FetchContent)
        FetchContent_Declare(sfml
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG ${SFML_VERSION_LOCAL_BUILD}
            GIT_SHALLOW ON
            SYSTEM
            EXCLUDE_FROM_ALL
            OVERRIDE_FIND_PACKAGE
        )
        FetchContent_MakeAvailable(SFML)

        find_package(SFML ${SFML_VERSION} COMPONENTS System Window Graphics Audio Main)
    endif()

endfunction()

# Create simulator as library
set(SRC_SYSTEM_UTILS
    ${LMBD_ROOT_DIR}/src/system/utils/coordinates.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/colorspace.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/brightness_handle.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/serial.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/sunset_timer.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/utils.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/vector_math.cpp
    )

set(SRC_SYSTEM_COLORS
    ${LMBD_ROOT_DIR}/src/system/colors/imuAnimations.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/animations.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/wipes.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/colors.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/text.cpp
    ${LMBD_ROOT_DIR}/src/system/colors/soundAnimations.cpp
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
    ${LMBD_ROOT_DIR}/src/system/global.cpp
)

set(SRC_SYSTEM_LOGIC
    ${LMBD_ROOT_DIR}/src/system/logic/alerts.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/behavior.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/inputs.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/statistics_handler.cpp
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

set(SIMULATOR_STATE
    ${LMBD_ROOT_DIR}/simulator/src/simulator_state.cpp
)

# Simulator Targets
function(create_simulator_target SIM_NAME)
    string(TOUPPER "${SIM_NAME}" UPPER_SIM_NAME)

    set(TARGET_NAME ${SIM_NAME}-simulator)
    add_executable(${TARGET_NAME}
        ${LMBD_ROOT_DIR}/simulator/src/${TARGET_NAME}.cpp
    )
    target_compile_definitions(${TARGET_NAME} PUBLIC LMBD_LAMP_TYPE__${UPPER_SIM_NAME})

    add_library(simulator_${SIM_NAME} OBJECT
        ${SRC_SYSTEM_UTILS}
        ${SRC_SYSTEM_COLORS}
        ${SRC_SYSTEM_POWER}
        ${SRC_SYSTEM_PHYSICAL}
        ${SRC_SYSTEM_PDLIB_C}
        ${SRC_SYSTEM_EXT}
        ${SRC_SYSTEM_LOGIC}
        ${SRC_SYSTEM_GLOBAL}
        ${SIMULATOR_MOCKS}
        ${SIMULATOR_STATE}
        ${LMBD_ROOT_DIR}/src/user/${SIM_NAME}_functions.cpp
    )
    target_compile_definitions(simulator_${SIM_NAME} PUBLIC LMBD_LAMP_TYPE__${UPPER_SIM_NAME})

    target_link_libraries(simulator_${SIM_NAME}
        sfml-graphics
        sfml-window
        sfml-audio
        sfml-system
    )

    target_link_libraries(${TARGET_NAME}
        simulator_${SIM_NAME}
        pthread
    )

endfunction()
