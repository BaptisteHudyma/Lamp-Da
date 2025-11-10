find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.test.cpp")


add_executable(
    ${PROJECT_NAME}_utest
    ${TEST_SOURCES}
)


target_compile_definitions(${PROJECT_NAME}_utest PUBLIC LMBD_LAMP_TYPE__INDEXABLE)

target_link_libraries(
    ${PROJECT_NAME}_utest
    PRIVATE
    simulator_indexable
    PUBLIC
    gtest
    gmock
)

target_include_directories(
    ${PROJECT_NAME}_utest
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_NAME}/simulator
)


target_compile_options(
    ${PROJECT_NAME}_utest
    PRIVATE
)
