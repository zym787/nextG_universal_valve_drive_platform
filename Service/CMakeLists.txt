# ===============================================================
# Service 层 CMakeLists.txt
#
# 职责：
#   - 协议解析/封包（svc_protocol）
#   - 阀门模型/运动控制（svc_motion）
#   - 错位重走/故障处理（svc_safety）
#   - 参数服务（svc_param）
#   - 服务初始化与轮询调度（svc_system）
#
# 依赖关系（架构规定，禁止反向）：
#   application -> service -> module -> bsp -> stm32cubemx/LL/CMSIS
#
# 维护规则（小白指引）：
#   1. 新增 svc_xxx.c 时，在 Src/ 与 Inc/ 下创建文件，
#      然后在下方"Service 源文件清单"取消注释或追加一行。
#   2. Service 只允许调用 Module 层接口，不直接调用 BSP/LL。
#   3. Service 是项目最值得做单元测试的层（协议/状态机/参数）。
# ===============================================================

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------
# 1. 占位源（让空 STATIC 库可成功创建）
# ---------------------------------------------------------------
set(_SVC_PLACEHOLDER ${CMAKE_CURRENT_BINARY_DIR}/_service_placeholder.c)
if(NOT EXISTS ${_SVC_PLACEHOLDER})
    file(WRITE ${_SVC_PLACEHOLDER}
        "/* auto-generated placeholder, safe to keep or delete */\n")
endif()

# ---------------------------------------------------------------
# 2. 定义 Service 静态库
# ---------------------------------------------------------------
add_library(service STATIC ${_SVC_PLACEHOLDER})

# ---------------------------------------------------------------
# 3. Service 源文件清单
# ---------------------------------------------------------------
target_sources(service PRIVATE
    # Src/svc_system.c       # Service/Module/BSP 初始化与轮询聚合
    # Src/svc_protocol.c     # 多协议解析、命令封包、校验
    # Src/svc_param.c        # 参数结构、双副本、CRC、版本迁移
    # Src/svc_motion.c       # 阀门模型、运动状态机、距离/速度换算
    # Src/svc_safety.c       # 错位重走、超时、故障保持
    # Src/svc_upgrade.c      # 后续 Bootloader/A-B 升级服务
)

# ---------------------------------------------------------------
# 4. 头文件搜索路径
# ---------------------------------------------------------------
target_include_directories(service PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# ---------------------------------------------------------------
# 5. 依赖：Service 只允许依赖 Module（以及未来的 ThirdParty 组件）
#    使用 PRIVATE 严格执行单向依赖：
#      - service 自己编译时能看到 mod_xxx.h
#      - 但 mod 头文件路径不会传递给 Application
#      - 强制 Application 只能通过 svc_xxx.h 间接调用业务服务
# ---------------------------------------------------------------
target_link_libraries(service PRIVATE
    module
    # 未来：ringbuffer / modbus / event / log 等组件加在这里
)
