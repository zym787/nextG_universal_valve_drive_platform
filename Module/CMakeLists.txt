# ===============================================================
# Module 层 CMakeLists.txt
#
# 职责：
#   - 板上器件驱动（外部 EEPROM、步进执行器、传感器输入、通信口）
#   - 把 BSP 的 MCU 外设能力组合成"器件级"接口，向 Service 暴露
#
# 依赖关系（架构规定，禁止反向）：
#   application -> service -> module -> bsp -> stm32cubemx/LL/CMSIS
#
# 维护规则（小白指引）：
#   1. 新增 mod_xxx.c 时，在 Src/ 与 Inc/ 下创建文件，
#      然后在下方"Module 源文件清单"取消注释或追加一行。
#   2. Module 只允许调用 BSP 层接口，不直接调用 LL/HAL。
#   3. Module 头文件不能出现 LL/HAL 类型（如 GPIO_TypeDef*）。
# ===============================================================

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------
# 1. 占位源（让空 STATIC 库可成功创建）
# ---------------------------------------------------------------
set(_MOD_PLACEHOLDER ${CMAKE_CURRENT_BINARY_DIR}/_module_placeholder.c)
if(NOT EXISTS ${_MOD_PLACEHOLDER})
    file(WRITE ${_MOD_PLACEHOLDER}
        "/* auto-generated placeholder, safe to keep or delete */\n")
endif()

# ---------------------------------------------------------------
# 2. 定义 Module 静态库
# ---------------------------------------------------------------
add_library(module STATIC ${_MOD_PLACEHOLDER})

# ---------------------------------------------------------------
# 3. Module 源文件清单
# ---------------------------------------------------------------
target_sources(module PRIVATE
    # Src/mod_eeprom.c            # 外部 EEPROM 器件驱动
    # Src/mod_stepper_driver.c    # 42 步进执行器（STEP/DIR/EN）
    # Src/mod_sensor_input.c      # 光感/限位/位置反馈
    # Src/mod_comm_port.c         # 通信口适配（USART/RS485/CAN）
    # Src/mod_rs485.c             # RS485 半双工方向控制
    # Src/mod_can_port.c          # CAN 通信口
)

# ---------------------------------------------------------------
# 4. 头文件搜索路径
# ---------------------------------------------------------------
target_include_directories(module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# ---------------------------------------------------------------
# 5. 依赖：Module 只允许依赖 BSP（以及未来的 BoardConfig）
#    使用 PRIVATE 严格执行单向依赖：
#      - module 自己编译时能看到 bsp_xxx.h
#      - 但 bsp 头文件路径不会传递给 Service / Application
#      - 强制 Service 只能通过 mod_xxx.h 间接使用板上器件
# ---------------------------------------------------------------
target_link_libraries(module PRIVATE
    bsp
    # 未来：selected_board_config 加在这里
)
