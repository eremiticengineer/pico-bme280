cmake_minimum_required(VERSION 3.13)

set(LIBRARY pico-bme280)
set(PROJECT pico-bme280-test)

set(PICO_BOARD pico2 CACHE STRING "Board type")

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${PROJECT} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

pico_sdk_init()

message(STATUS "PICO_PLATFORM=${PICO_PLATFORM}")

# ---------------------------------------------------------
# Library
# ---------------------------------------------------------
add_library(${LIBRARY}
    src/BME280.cpp
)

target_include_directories(${LIBRARY} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(${LIBRARY} PUBLIC
    pico_stdlib
    hardware_i2c
)

# ---------------------------------------------------------
# Standalone test
# ---------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
    set(FREERTOS_KERNEL_PATH
        ${CMAKE_CURRENT_LIST_DIR}/lib/FreeRTOS-Kernel
    )

    include(
        ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake
    )

    add_executable(${PROJECT}
        examples/main.cpp
    )

    # PRIVATE = only needed to build this executable
    # it isn’t part of the public interface of the library
    target_include_directories(${PROJECT} PRIVATE
        include/
    )

    target_link_libraries(${PROJECT}
        pico_stdlib
        hardware_i2c
        FreeRTOS-Kernel-Heap4
        ${LIBRARY}
    )
endif()

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 1)

pico_add_extra_outputs(${PROJECT})
