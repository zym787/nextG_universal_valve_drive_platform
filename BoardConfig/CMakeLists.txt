# ===============================================================
# BoardConfig 层 CMakeLists.txt
#
# 职责：
#   - 集中描述"板卡硬件差异"：通道数、引脚映射、EEPROM 容量、
#     RS485 方向脚、光感数量/极性、STEP/DIR/EN 引脚等。
#   - 同一颗 MCU 的多块单板在这里维护多份目录（hw_rev_a / hw_rev_b ...），
#     由 CMake cache 变量 BOARD_HW 选择当前编译哪一份。
#
# 上层使用方式（BSP / Module / Service 都可以链接）：
#   target_link_libraries(<lower_layer> PRIVATE selected_board_config)
#   #include "board_config.h"
#
# 维护规则（小白指引）：
#   1. 新增硬件版本：在本目录下复制一份子目录 hw_rev_x/，
#        放 board_config.h、board_config.c。
#   2. 切换硬件版本：构建时加 -DBOARD_HW=hw_rev_x，
#        或在 CMakePresets.json 的 cacheVariables 里设置。
#   3. 仅放"硬件差异"，不要放业务逻辑或 MCU 寄存器操作。
# ===============================================================

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------
# 1. 选择当前编译的硬件版本
#    可通过 -DBOARD_HW=hw_rev_b 在命令行覆盖；
#    也可在 CMakePresets.json 的 cacheVariables 中固定。
# ---------------------------------------------------------------
set(BOARD_HW "hw_rev_a" CACHE STRING "Board hardware revision (subdirectory name)")
message(STATUS "BoardConfig: BOARD_HW=${BOARD_HW}")

set(_BOARD_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD_HW})
set(_BOARD_SRC    ${_BOARD_DIR}/board_config.c)

# ---------------------------------------------------------------
# 2. 占位源（即使 hw_rev_x/board_config.c 还没写，也能通过编译）
#    一旦真实源文件创建后会优先使用真实源；占位源只是兜底。
# ---------------------------------------------------------------
set(_BOARD_PLACEHOLDER ${CMAKE_CURRENT_BINARY_DIR}/_board_placeholder.c)
if(NOT EXISTS ${_BOARD_PLACEHOLDER})
    file(WRITE ${_BOARD_PLACEHOLDER}
        "/* auto-generated placeholder for selected_board_config, safe to keep */\n")
endif()

# ---------------------------------------------------------------
# 3. 定义板配置静态库
#    target 名固定为 selected_board_config，方便上层无脑链接。
# ---------------------------------------------------------------
add_library(selected_board_config STATIC ${_BOARD_PLACEHOLDER})

# ---------------------------------------------------------------
# 4. 真实源文件（如果对应硬件目录已经准备好）
# ---------------------------------------------------------------
if(EXISTS ${_BOARD_SRC})
    target_sources(selected_board_config PRIVATE ${_BOARD_SRC})
    message(STATUS "BoardConfig: using ${_BOARD_SRC}")
else()
    message(STATUS "BoardConfig: ${_BOARD_SRC} not found, using placeholder only")
endif()

# ---------------------------------------------------------------
# 5. 头文件搜索路径
#    PUBLIC：链接 selected_board_config 的 target 都能 #include "board_config.h"
#    若硬件目录尚未创建，跳过 include path（避免 CMake 报警告）。
# ---------------------------------------------------------------
if(EXISTS ${_BOARD_DIR})
    target_include_directories(selected_board_config PUBLIC ${_BOARD_DIR})
endif()

# ---------------------------------------------------------------
# 6. 依赖
#    BoardConfig 是"叶子层"，不依赖任何业务层；
#    它只描述常量、宏、引脚、容量等，不做寄存器操作。
#    若 board_config.c 中需要写 LL 代码（不推荐），可以再考虑链接 stm32cubemx。
# ---------------------------------------------------------------
# target_link_libraries(selected_board_config PRIVATE stm32cubemx)
