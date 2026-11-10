find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.test.cpp")


add_executable(
    ${PROJECT_NAME}_utest_simple
    ${TEST_SOURCES}
)


target_compile_definitions(${PROJECT_NAME}_utest_simple PUBLIC LMBD_LAMP_TYPE__SIMPLE)

target_link_libraries(
    ${PROJECT_NAME}_utest_simple
    PRIVATE
    simulator_simple
    PUBLIC
    gtest
    gmock
    etl::etl
)

target_include_directories(
    ${PROJECT_NAME}_utest_simple
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_NAME}/simulator
)


target_compile_options(
    ${PROJECT_NAME}_utest_simple
    PRIVATE
)
