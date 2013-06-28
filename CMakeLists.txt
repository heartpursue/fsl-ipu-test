cmake_minimum_required(VERSION 2.6)

project(fsl-ipu-test)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_path(IPU_INCLUDE_DIRS linux/ipu.h HINTS ${CMAKE_FIND_ROOT_PATH} PATH_SUFFIXES /usr/src/kernel/include)

include_directories(
	include
    ${IPU_INCLUDE_DIRS}
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
file(GLOB fsl-ipu-test_SOURCES include/*.h source/*.c)

add_definitions(-Wall)

add_executable(fsl-ipu-test ${fsl-ipu-test_SOURCES})
install(TARGETS fsl-ipu-test RUNTIME DESTINATION /usr/bin)

