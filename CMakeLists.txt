# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Pico SDK setup ===
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(lpf_controller C CXX ASM)
pico_sdk_init()

add_executable(lpf_controller main.c)
pico_set_program_name(lpf_controller "lpf_controller")
pico_set_program_version(lpf_controller "0.1")

pico_enable_stdio_usb(lpf_controller 1)
pico_enable_stdio_uart(lpf_controller 0)

target_link_libraries(lpf_controller
    pico_stdlib
    hardware_i2c
)

pico_add_extra_outputs(lpf_controller)
