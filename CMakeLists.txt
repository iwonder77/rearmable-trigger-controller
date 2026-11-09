# ----- SETTINGS BEFORE `project()` -----

# minimum supported CMake version
cmake_minimum_required(VERSION 3.13)

# generate compile_commands.json for clangd / Neovim
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# import the Pico SDK before configuring the compiler
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# ----- PROJECT CONFIGURATION AND SDK SETUP -----

# define project name, enabled languages and their standards
project(rearmable-trigger-controller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# require an exact Pico SDK version string
set(REQUIRED_PICO_SDK_VERSION "2.3.1")

if(NOT PICO_SDK_VERSION_STRING STREQUAL REQUIRED_PICO_SDK_VERSION)
  message(FATAL_ERROR
    "Pico SDK version mismatch.\n"
    "Expected: ${REQUIRED_PICO_SDK_VERSION}\n"
    "Actual: ${PICO_SDK_VERSION_STRING}"
  )
endif()

# initialize the Pico SDK
pico_sdk_init()

# ----- APPLICATION TARGET SETTINGS -----

# define the application executable and its source files
add_executable(${PROJECT_NAME} src/main.cpp)

# generate additional firmware output formats (map/bin/hex/uf2 files)
pico_add_extra_outputs(${PROJECT_NAME})

# enable printf out the UART pins (already enabled by default, just making it specific)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
# also send printf over the chip's USB port
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# link to pico_stdlib library (for gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib)

# embed application identity and version metadata
pico_set_program_name(${PROJECT_NAME} "${PROJECT_NAME}")
pico_set_program_version(${PROJECT_NAME} "${PROJECT_VERSION}")
