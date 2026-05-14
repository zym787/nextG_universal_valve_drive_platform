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
set(CMAKE_PROJECT_NAME nextG_universal_valve_drive_platform)

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

# 业务分层：必须按依赖从底到顶的顺序 add_subdirectory
#   BoardConfig / ThirdParty -> bsp -> module -> service -> application
#
# 辅助域优先加入，以便 BSP / Module / Service 能看到 target：
#   - BoardConfig: 提供 selected_board_config
#   - ThirdParty : 提供 ringbuffer / modbus / event / log / osal 等独立 target
#                  （当前均为注释占位，启用后才会出现在构建中）
# 顶层 ELF 只需链接 application，其他层通过链接闭包自动传递。
add_subdirectory(BoardConfig)
add_subdirectory(ThirdParty)
add_subdirectory(BSP)
add_subdirectory(Module)
add_subdirectory(Service)
add_subdirectory(Application)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
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
    application      # 链接闭包自动带出 service -> module -> bsp

    # Add user defined libraries
)

# ---------------------------------------------------------------
# 顶层额外强制带入辅助域 target。
# 原因：bsp/module 内部已改为 PRIVATE 链接，selected_board_config
# 不会透到顶层；在这里显式补上以保证进入链接闭包。
# 不使用时 selected_board_config 为纯占位库，不产生任何代码。
# ---------------------------------------------------------------
target_link_libraries(${CMAKE_PROJECT_NAME}
    selected_board_config
)

# 生成hex和bin文件 elf2hex and elf2bin
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_COMMAND} -E echo "Firmware SIZE:"
    COMMAND ${CMAKE_SIZE} -B -d ${PROJECT_NAME}.hex
    COMMENT "Building hex & bin file, and reporting firmware size..."
)
