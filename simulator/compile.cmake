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

# Hack: compile depends directly
set(SRC_SYSTEM_DEPENDS
    ${LMBD_ROOT_DIR}/src/depends/arduinoFFT/src/arduinoFFT.cpp
    )

# Create simulator as library
set(SRC_SYSTEM_UTILS
    ${LMBD_ROOT_DIR}/src/system/utils/colorspace.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/cmd_parser.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/utils.cpp
    ${LMBD_ROOT_DIR}/src/system/utils/vector_math.cpp
    )

set(SRC_SYSTEM_BSP
    ${LMBD_ROOT_DIR}/src/system/bsp/pd/power_delivery.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/pd/task.c
    ${LMBD_ROOT_DIR}/src/system/bsp/pd/usb_pd_policy.c
    ${LMBD_ROOT_DIR}/src/system/bsp/pd/usb_pd_protocol.c

    ${LMBD_ROOT_DIR}/src/system/bsp/balancer.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/charging_ic.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/imu_wrapper.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/indicator.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/power_gates.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/text_in.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/text_out.cpp
    ${LMBD_ROOT_DIR}/src/system/bsp/threads.cpp
)

set(SRC_SYSTEM_COMPONENT
    ${LMBD_ROOT_DIR}/src/system/component/battery.cpp
    ${LMBD_ROOT_DIR}/src/system/component/button.cpp
    ${LMBD_ROOT_DIR}/src/system/component/charger.cpp
    ${LMBD_ROOT_DIR}/src/system/component/fileSystem.cpp
    ${LMBD_ROOT_DIR}/src/system/component/imu.cpp
    ${LMBD_ROOT_DIR}/src/system/component/output_power.cpp
    ${LMBD_ROOT_DIR}/src/system/component/sound.cpp
    ${LMBD_ROOT_DIR}/src/system/component/time_handling.cpp
)

set(SRC_SYSTEM_DRIVER
    ${LMBD_ROOT_DIR}/src/system/driver/pd/FUSB302.c
    ${LMBD_ROOT_DIR}/src/system/driver/pd/tcpm_driver.cpp
    ${LMBD_ROOT_DIR}/src/system/driver/pd/usb_pd_driver.c

    ${LMBD_ROOT_DIR}/src/system/driver/LSM6DS3.cpp
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
    ${LMBD_ROOT_DIR}/src/system/logic/brightness_handle.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/command_line_interface.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/inputs_bluetooth.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/inputs.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/power_handler.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/statistics_handler.cpp
    ${LMBD_ROOT_DIR}/src/system/logic/sunset_timer.cpp
)

set(SIMULATOR_HAL
    ${LMBD_ROOT_DIR}/simulator/hal/bluetooth_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/gpio_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/i2c_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/pdm_handle_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/serial_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/queues_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/registers_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/threads_mock.cpp
    ${LMBD_ROOT_DIR}/simulator/hal/time_mock.cpp
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
        ${SRC_SYSTEM_DEPENDS}
        ${SRC_SYSTEM_UTILS}
        ${SRC_SYSTEM_BSP}
        ${SRC_SYSTEM_COMPONENT}
        ${SRC_SYSTEM_DRIVER}
        ${SRC_SYSTEM_EXT}
        ${SRC_SYSTEM_LOGIC}
        ${SRC_SYSTEM_GLOBAL}
        ${SIMULATOR_HAL}
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
