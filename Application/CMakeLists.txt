# ===============================================================
# Application 层 CMakeLists.txt
#
# 职责：
#   - 系统业务入口（app_Init / app_MainLoop）
#   - 控制流程、运动流程、可靠性守护、协议任务编排
#
# 依赖关系（架构规定，禁止反向）：
#   application -> service -> module -> bsp -> stm32cubemx/LL/CMSIS
#
# 上层使用方式（顶层 CMakeLists.txt 中）：
#   target_link_libraries(<elf_target> PRIVATE application)
#   通过 PUBLIC 链路自动带出 service/module/bsp
#
# 维护规则（小白指引）：
#   1. 新增一个应用文件 app_xxx.c：
#        在 Src/ 下新增 app_xxx.c，在 Inc/ 下新增 app_xxx.h，
#        然后在下方"Application 源文件清单"取消对应注释或追加一行。
#   2. Application 只允许调用 Service 层接口，不直接调用 Module/BSP/LL。
#   3. 不要在本文件中写业务实现，只做编译/链接配置。
# ===============================================================

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------
# 1. 占位源（小白可忽略其原理）
#    目的：当 Src/ 下尚未有任何 .c 时，让 add_library(STATIC) 不报错。
#    它只是一个空 C 文件，写到构建目录里，不会污染源代码目录。
#    后续添加真实源后保留它也无副作用，可以一直留着。
# ---------------------------------------------------------------
set(_APP_PLACEHOLDER ${CMAKE_CURRENT_BINARY_DIR}/_application_placeholder.c)
if(NOT EXISTS ${_APP_PLACEHOLDER})
    file(WRITE ${_APP_PLACEHOLDER}
        "/* auto-generated placeholder, safe to keep or delete */\n")
endif()

# ---------------------------------------------------------------
# 2. 定义 Application 静态库
# ---------------------------------------------------------------
add_library(application STATIC ${_APP_PLACEHOLDER})

# ---------------------------------------------------------------
# 3. Application 源文件清单
#    新增 app_xxx.c 时，按字母序追加一行；不需要的保持注释即可。
# ---------------------------------------------------------------
target_sources(application PRIVATE
    Src/app.c                # 系统入口：app_MainLoop（后续加 app_Init）
    # Src/app_control.c      # 阀门控制业务流程
    # Src/app_movement.c     # 动作流程编排
    # Src/app_guardian.c     # 系统级可靠性守护（含喂狗策略）
    # Src/app_agreement.c    # 协议任务和命令分发
)

# ---------------------------------------------------------------
# 4. 头文件搜索路径
#    PUBLIC：链接 application 的 target 也能直接 #include "app.h"
# ---------------------------------------------------------------
target_include_directories(application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# Add project symbols (macros)
target_compile_definitions(application PUBLIC
    # Add user defined symbols
    APP_ENABLE_BRINGUP  # 初启时允许app直接调用bsp
)

# ---------------------------------------------------------------
# 5. 依赖：Application 只允许依赖 Service
#    使用 PRIVATE 严格执行单向依赖：
#      - application 自己编译时能看到 svc_xxx.h
#      - 但 svc 头文件路径不会传递给顶层 ELF（main.c）
#      - main.c 只能通过 app.h 调用 app_Init / app_MainLoop
# ---------------------------------------------------------------
target_link_libraries(application PRIVATE
    bsp
    service
)
