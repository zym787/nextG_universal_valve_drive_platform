# ===============================================================
# ThirdParty 层 CMakeLists.txt
#
# 职责：
#   - 管理外部组件（ringbuffer / event / modbus / osal / log ...）
#   - 每个组件 = 一个独立的 CMake target，源码尽量保持上游原样
#   - 上层（Service / Module / BSP）按需链接独立 target
#
# 重要约定（来自 thirdparty_management_guide.md）：
#   1. 第三方源码保持上游命名（文件、函数、类型、宏不强制 svc_/mod_/bsp_）
#   2. 不在顶层 CMakeLists.txt 直接列第三方源码
#   3. 不把 ThirdParty 整个目录暴露为 include path —— 必须通过各组件
#      自己的 target_include_directories(... PUBLIC ...)
#   4. 每个组件目录建议包含 LICENSE / SOURCE.txt / VERSION.txt
#
# 上层使用方式（按需挑选）：
#   target_link_libraries(service PRIVATE ringbuffer modbus event log)
#   target_link_libraries(bsp     PRIVATE ringbuffer)
#   #include "ringbuffer.h"
#
# 维护规则（小白指引）：
#   1. 引入新组件：
#        a) 在 ThirdParty/<name>/ 下放置上游源码 + LICENSE + SOURCE.txt
#        b) 取消下方对应的 add_subdirectory(<name>) 注释，或新增一段
#           独立 add_library(<name> STATIC ...) 定义
#   2. 暂未启用的组件保持注释，不会进入构建
#   3. 修改第三方源码时，必须在 SOURCE.txt 内记录原因 / patch
# ===============================================================

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------
# 推荐的子目录入口（每个组件维护自己的 CMakeLists.txt）
# 启用时取消注释；未引入的组件保持注释即可。
# ---------------------------------------------------------------
# add_subdirectory(ringbuffer)   # 环形队列：USART RX / 协议输入 / 日志缓冲
# add_subdirectory(event)        # 轻量事件 / 消息（固定长度队列）
# add_subdirectory(modbus)       # Modbus RTU slave（vendor 固定版本）
# add_subdirectory(osal)         # 轻量调度抽象（tick / timeout / poll）
# add_subdirectory(log)          # 轻量日志（Release 可裁剪）

# ---------------------------------------------------------------
# 备选写法：在本文件内直接定义小型组件（仅当组件源文件极少时使用）
# 推荐还是放到子目录维护，避免本文件过长。
#
# 示例（ringbuffer 启用时取消注释）：
# ---------------------------------------------------------------
# add_library(ringbuffer STATIC
#     ringbuffer/ringbuffer.c
# )
# target_include_directories(ringbuffer PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/ringbuffer
# )

# ---------------------------------------------------------------
# 示例（modbus 启用时取消注释）：
# ---------------------------------------------------------------
# add_library(modbus STATIC
#     modbus/mb.c
#     modbus/mbcrc.c
# )
# target_include_directories(modbus PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/modbus
# )

# ---------------------------------------------------------------
# 注意：
#   - ThirdParty 不创建任何聚合 target，避免上层"一键全链接"
#     掩盖真实依赖。需要哪个组件就 link 哪个组件。
#   - 若组件本身需要 OS 抽象，可让其链接 osal 而不是直接调用 BSP。
#   - 第三方源码的静态分析告警建议在 CI 中按目录排除。
# ---------------------------------------------------------------
