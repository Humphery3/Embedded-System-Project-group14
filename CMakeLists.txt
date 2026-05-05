cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Unit_4_1_Pong)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_SOURCE_DIR}/ST7789V2_Driver_STM32L4/Core/Src/LCD.c
    ${CMAKE_SOURCE_DIR}/ST7789V2_Driver_STM32L4/Core/Src/ST7789V2_Driver.c
    ${CMAKE_SOURCE_DIR}/Joystick/Joystick.c
    ${CMAKE_SOURCE_DIR}/Buzzer/Buzzer.c
    ${CMAKE_SOURCE_DIR}/PWM/PWM.c
    ${CMAKE_SOURCE_DIR}/Player/Player.c
    ${CMAKE_SOURCE_DIR}/Obstacle/Obstacle.c
    ${CMAKE_SOURCE_DIR}/Coin/Coin.c
    ${CMAKE_SOURCE_DIR}/Game/Game.c
    ${CMAKE_SOURCE_DIR}/Renderer/Renderer.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ST7789V2_Driver_STM32L4/Core/Inc
    ${CMAKE_SOURCE_DIR}/Joystick
    ${CMAKE_SOURCE_DIR}/Buzzer
    ${CMAKE_SOURCE_DIR}/PWM
    ${CMAKE_SOURCE_DIR}/Player
    ${CMAKE_SOURCE_DIR}/Obstacle
    ${CMAKE_SOURCE_DIR}/Coin
    ${CMAKE_SOURCE_DIR}/Game
    ${CMAKE_SOURCE_DIR}/Renderer
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Enable floating point printf/scanf support
# Note: This adds ~7-10KB to the binary size (ROM), but the STM32L476 has 1MB flash
# so this is not a concern. The main constraint for the projects is the 128KB RAM.
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -u _printf_float
    #-u _scanf_float
)

# Create .bin from the ELF using the toolchain objcopy
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    COMMENT "Generating .bin from ELF"
)