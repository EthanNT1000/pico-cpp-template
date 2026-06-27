# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Override FLASH length: 2MB - 4KB sector = 2044KB
set(PICO_FLASH_SIZE_BYTES "0x1FF000")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(app C CXX ASM)

# Initialise the Raspberry Pi Pico SDK (must be after project, before targets)
pico_sdk_init()

# FreeRTOS
set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel" CACHE STRING "FreeRTOS Lib" FORCE)
set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include" CACHE STRING "FreeRTOS Local Config" FORCE)
include(FreeRTOS_Kernel_import.cmake)

# micro-ROS
set(MICROROS_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-cpp-sdk/libmicroros" CACHE STRING "micro-ROS Lib Path" FORCE)
link_directories(${MICROROS_PATH})
# Required for micro-ROS size optimization
add_compile_options(-ffunction-sections -fdata-sections)

add_executable(app
    app.cpp
    pico-cpp-sdk/source/dma.cpp
    pico-cpp-sdk/source/spi.cpp
    pico-cpp-sdk/source/uart.cpp
    pico-cpp-sdk/source/mcp2515.cpp
    pico-cpp-sdk/source/taskinterface.cpp
    pico-cpp-sdk/source/udp_transport.cpp
    pico-cpp-sdk/source/posix_time.c
    source/system.cpp
    source/task/taskhandler.cpp
    source/task/cansendtask.cpp
    source/task/microrospublishtask.cpp
    source/task/uarttesttask.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-cpp-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/task
    ${CMAKE_CURRENT_LIST_DIR}
    ${FREERTOS_CONFIG_FILE_DIRECTORY}
    ${MICROROS_PATH}/include
)

target_link_libraries(app PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    hardware_spi
    hardware_uart
    hardware_dma
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_iperf
    pico_stdlib
    microros
)

pico_set_program_name(app "app")
pico_set_program_version(app "0.1")

pico_enable_stdio_uart(app 1)
pico_enable_stdio_usb(app 0)
pico_add_extra_outputs(app)
