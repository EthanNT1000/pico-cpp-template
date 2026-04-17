# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(app C CXX ASM)

#include Libraries
SET(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel" CACHE STRING "FreeRTOS Lib" FORCE)
SET(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include" CACHE STRING "FreeRTOS Local Config" FORCE)
include_directories("${FREERTOS_CONFIG_FILE_DIRECTORY}")
include(FreeRTOS_Kernel_import.cmake)

# --- micro-ROS Integration ---
set(MICROROS_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-cpp-sdk/libmicroros" CACHE STRING "micro-ROS Lib Path" FORCE)
link_directories(${MICROROS_PATH})
include_directories(app INTERFACE ${MICROROS_PATH}/include)
# Enable section garbage collection for micro-ROS (required for size optimization)
SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

# Override FLASH length: Original is 2M (0x200000 bytes)
# Reserve 1 × 4KB sector → subtract 0x1000
# 2048KB - 4KB = 2044KB
set(PICO_FLASH_SIZE_BYTES "0x1FF000")

# Add executable. Default name is the project name, version 0.1
add_executable(app
    app.cpp
    pico-cpp-sdk/source/dma.cpp
    pico-cpp-sdk/source/spi.cpp
    pico-cpp-sdk/source/mcp2515.cpp
    pico-cpp-sdk/source/taskinterface.cpp
    pico-cpp-sdk/source/udp_transport.cpp
    pico-cpp-sdk/source/posix_time.c
    source/system.cpp
    source/task/taskhandler.cpp
    source/task/cansendtask.cpp
    source/task/microrospublishtask.cpp
)

# Add the standard include files to the build
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-cpp-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/task
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add the standard library to the build
target_link_libraries(app PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    hardware_spi
    hardware_dma
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_iperf
    pico_stdlib
    microros
)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

pico_set_program_name(app "app")
pico_set_program_version(app "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(app 1)
pico_enable_stdio_usb(app 0)
pico_add_extra_outputs(app)
