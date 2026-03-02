cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(SensorCal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO: remove once image2wx is restructured as a proper cmake package
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vendor/image2wx)

add_subdirectory(vendor)
add_subdirectory(src)

add_custom_target(
    run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)