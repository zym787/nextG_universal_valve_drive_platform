# BoardConfig 开发指南

返回：[工程文档总入口](about.md)

本文档说明 `BoardConfig` 如何处理多板卡差异。`BoardConfig` 不是业务层，也不是驱动层；它只是编译期选择的硬件配置域。

## 1. 定位

一句话：

```text
BoardConfig 描述当前硬件长什么样，Service/Module/BSP 根据配置选择通道、引脚和器件差异。
```

适合放在 BoardConfig：

- 硬件版本号。
- 通道数量、半通道能力。
- 光感数量、极性、逻辑通道映射。
- STEP/DIR/EN 逻辑通道映射。
- EEPROM I2C 地址、页大小、容量。
- RS485 DE/RE 控制引脚逻辑 ID。
- 默认波特率、默认速度等出厂默认值。

不放在 BoardConfig：

- 协议解析。
- 运动状态机。
- EEPROM 页写流程。
- GPIO/TIM/USART 的 LL 初始化。
- 大量运行时状态。

结论：初期使用一个 CubeMX 基线工程，通过 `BoardConfig`、CMake preset 和编译宏处理轻量板卡差异；当 MCU、外设实例或时钟树明显变化时，再拆独立 CubeMX 工程。

资源约束：STM32F103C8T6 RAM 较小，BoardConfig 初期只使用 `static const` 配置表，不做运行时动态配置系统，不复制大表到 RAM。

## 2. 单 CubeMX 还是多 CubeMX

初期推荐：

```text
一个 CubeMX 基线工程 + BoardConfig + CMake presets + 编译宏
```

适用条件：

- MCU 仍是 STM32F103C8T6 或同系列兼容型号。
- 时钟树一致。
- USART/TIM/ADC/I2C 等外设实例基本一致。
- 主要差异是 IO、通道数、传感器数量、驱动芯片接法。

需要拆多 CubeMX 工程的条件：

- MCU 型号或封装变化明显。
- 外设实例变化明显，例如 TIM1 改 TIM3、USART1 改 USART2。
- 时钟树、DMA、中断资源差异明显。
- 链接脚本、启动文件或 SDK 体系变化。
- 迁移到国产 MCU，CubeMX/LL 不再适用。

## 3. 目录结构

```text
BoardConfig/
  CMakeLists.txt
  hw_rev_a/
    board_config.h
    board_config.c
  hw_rev_b/
    board_config.h
    board_config.c
```

初期可以只有一个硬件版本：

```text
BoardConfig/
  CMakeLists.txt
  hw_rev_a/
    board_config.h
    board_config.c
```

## 4. 配置内容

示例：

```c
typedef enum {
    BOARD_STEPPER_DRIVER_PULSE_DIR = 0,
    BOARD_STEPPER_DRIVER_UART_CFG,
} board_stepper_driver_type_t;

typedef struct {
    uint8_t channel_count;
    uint8_t half_channel_enabled;
    uint8_t sensor_count;
    uint8_t sensor_active_level;
    uint8_t eeprom_i2c_address;
    uint16_t eeprom_page_size;
    uint16_t eeprom_size_bytes;
    board_stepper_driver_type_t stepper_driver_type;
} board_config_t;

const board_config_t *board_config_Get(void);
```

实现建议：

```c
static const board_config_t s_board_config = {
    .channel_count = 4,
    .half_channel_enabled = 1,
    .sensor_count = 4,
    .sensor_active_level = 0,
    .eeprom_i2c_address = 0x50,
    .eeprom_page_size = 16,
    .eeprom_size_bytes = 4096,
    .stepper_driver_type = BOARD_STEPPER_DRIVER_PULSE_DIR,
};

const board_config_t *board_config_Get(void)
{
    return &s_board_config;
}
```

不要在 `board_config_Get()` 中动态生成配置，也不要把配置复制到全局 RAM。

逻辑引脚建议使用项目自己的 ID，不把 `GPIO_TypeDef *` 暴露给 Service/Module：

```c
typedef enum {
    BOARD_PIN_STEPPER0_STEP = 0,
    BOARD_PIN_STEPPER0_DIR,
    BOARD_PIN_STEPPER0_EN,
    BOARD_PIN_SENSOR0,
    BOARD_PIN_RS485_DE,
} board_pin_id_t;
```

`BSP` 可以把 `board_pin_id_t` 映射到真实 GPIO port/pin；`Module` 只使用逻辑 ID。

## 5. CMake presets

推荐为板卡建立 preset：

```json
{
  "name": "Release-hw-rev-a",
  "inherits": "Release",
  "cacheVariables": {
    "BOARD_HW": "hw_rev_a"
  }
}
```

`BoardConfig/CMakeLists.txt` 示例：

```cmake
set(BOARD_HW "hw_rev_a" CACHE STRING "Board hardware revision")
string(TOUPPER "${BOARD_HW}" BOARD_HW_UPPER)

add_library(selected_board_config OBJECT
    ${BOARD_HW}/board_config.c
)

target_include_directories(selected_board_config PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/${BOARD_HW}
)

target_compile_definitions(selected_board_config PUBLIC
    BOARD_CONFIG_${BOARD_HW_UPPER}=1
)
```

## 6. 与 CubeMX 生成代码的关系

原则：

- CubeMX 继续负责时钟、外设实例、中断、基础 GPIO 初始化。
- BoardConfig 负责逻辑硬件差异和参数表。
- BSP 负责把 BoardConfig 的逻辑 ID 映射到真实 MCU 外设。
- 不在 BoardConfig 里手写大量 LL 初始化代码。
- 不在 BoardConfig 里做复杂运行时决策。

如果某个硬件版本必须改变 CubeMX 外设实例，应升级为独立 CubeMX target，而不是在 BoardConfig 中硬凑。

## 7. 测试

BoardConfig 可做轻量 PC 测试：

- 通道数量不超过最大值。
- EEPROM 地址、页大小、容量合法。
- 光感数量与通道映射一致。
- 每个 STEP/DIR/EN 逻辑引脚不重复。
- 默认参数在允许范围内。

## 8. 执行计划

项目级详细 TODO 统一维护在 [about.md](about.md)。BoardConfig 本层开发只保留以下检查重点：

- BoardConfig 只描述硬件差异，不写 LL 初始化。
- 轻量 IO、通道、传感器差异用 `BOARD_HW` 和配置表处理。
- MCU、时钟树、外设实例明显变化时再拆独立 CubeMX 工程。
- 固件产物文件名必须包含硬件版本。
- 新板卡加入 CI matrix 后再视为可维护目标。
