# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

if("${IDF_TARGET}" STREQUAL "linux")
    set(COMPONENTS main)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(tcp_client)

# MJPEG frames
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0001.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0010.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0020.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0030.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0040.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0050.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0060.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0070.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0080.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0090.jpg" BINARY)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "../../frames/frame_0100.jpg" BINARY)
