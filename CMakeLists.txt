cmake_minimum_required(VERSION 3.13)

set(LIBRARY pico_bme280)

# ---------------------------------------------------------
# Are we being built directly or as a submodule?
# ---------------------------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(PICO_LORA_STANDALONE TRUE)
else()
    set(PICO_LORA_STANDALONE FALSE)
endif()

# ---------------------------------------------------------
# Standalone project setup
# ---------------------------------------------------------
if(PICO_LORA_STANDALONE)
    set(PROJECT pico_bme280_test)

    if(DEVICE STREQUAL "seeed_xiao_rp2040")
        set(PICO_BOARD seeed_xiao_rp2040 CACHE STRING "Board type" FORCE)
    elseif(DEVICE STREQUAL "pico")
        set(PICO_BOARD pico CACHE STRING "Board type" FORCE)
    elseif(DEVICE STREQUAL "pico2")
        set(PICO_BOARD pico2 CACHE STRING "Board type" FORCE)
    else()
        message(FATAL_ERROR
            "Unknown DEVICE='${DEVICE}'. Use pico, pico2 or seeed_xiao_rp2040."
        )
    endif()

    include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

    project(${LIBRARY} C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 23)

    pico_sdk_init()

    message(STATUS "DEVICE=${DEVICE}")
    message(STATUS "PICO_BOARD=${PICO_BOARD}")
    message(STATUS "PICO_PLATFORM=${PICO_PLATFORM}")
endif()

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
# Standalone example
# ---------------------------------------------------------
if(PICO_LORA_STANDALONE)
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
        FreeRTOS-Kernel-Heap4
        ${LIBRARY}
    )

  pico_enable_stdio_usb(${PROJECT} 1)
  pico_enable_stdio_uart(${PROJECT} 1)

  pico_add_extra_outputs(${PROJECT})
endif()
