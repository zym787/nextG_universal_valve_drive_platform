# ===============================================================
# BSP 层 CMakeLists.txt
#
# 职责：
#   - 把 BSP/Src 下所有 bsp_*.c 编译为名为 'bsp' 的静态库
#   - 通过 BSP/Inc 向上层暴露 bsp_*.h 头文件
#   - 仅依赖 STM32 LL（由 stm32cubemx target 提供）
#
# 上层使用方式：
#   target_link_libraries(<your_target> PRIVATE bsp)
#   #include "bsp_iwdg.h"   // 不需要写完整路径
#
# 维护规则（小白指引）：
#   1. 新增一个 BSP 驱动时：
#        在 Src/ 下新增 bsp_xxx.c，在 Inc/ 下新增 bsp_xxx.h，
#        然后在下方"BSP 源文件清单"里追加一行：Src/bsp_xxx.c
#   2. BSP 内部可以使用 LL 库
#      （#include "stm32f1xx_ll_xxx.h"）。
#   3. BSP 头文件不要暴露 LL/HAL 类型给 Module/Service。
# ===============================================================

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------
# 1. 定义 BSP 静态库
# ---------------------------------------------------------------
add_library(bsp STATIC)

# ---------------------------------------------------------------
# 2. BSP 源文件清单
#    新增 bsp_xxx.c 时，按字母序在下面追加一行即可。
#    暂不需要的可以注释，构建时不会编译它。
# ---------------------------------------------------------------
target_sources(bsp PRIVATE
    Src/bsp_iwdg.c
    # Src/bsp_usart.c       # TODO: 串口 + ringbuffer
    # Src/bsp_timer.c       # TODO: 定时器基础能力
    # Src/bsp_pwm.c         # TODO: PWM 输出（步进 STEP 脉冲）
    # Src/bsp_adc.c         # TODO: ADC 采样
    # Src/bsp_gpio.c        # TODO: 通用 GPIO 读写
    # Src/bsp_soft_i2c.c    # TODO: 软件 I2C（驱动外部 EEPROM）
    # Src/bsp_dwt.c         # TODO: DWT 微秒计时
)

# ---------------------------------------------------------------
# 3. 头文件搜索路径
#    PUBLIC 表示：链接 bsp 的 target 也能直接看到 BSP/Inc，
#    因此上层 #include "bsp_iwdg.h" 不需要写完整相对路径。
# ---------------------------------------------------------------
target_include_directories(bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# ---------------------------------------------------------------
# 4. 依赖
#    bsp 需要使用 STM32 LL（IWDG/RCC/USART 等寄存器封装），
#    它由 cmake/stm32cubemx 子项目导出的 stm32cubemx target 提供。
#
#    使用 PRIVATE 严格执行单向依赖：
#      - bsp 自己编译时能看到 LL 头文件
#      - 但 LL 头文件路径不会传递给 Module / Service / Application
#      - 强制上层只能通过 bsp_xxx.h 间接使用 MCU 外设
#    最终 ELF 链接时，stm32cubemx.a 仍会通过链接闭包被串入，不会缺符号。
# ---------------------------------------------------------------
target_link_libraries(bsp PRIVATE
    stm32cubemx
)
