# Module 开发指南

返回：[工程文档总入口](about.md)

本文档说明 `Module` 层如何编写。`Module` 不是业务层，也不是 MCU 外设层；它只负责“板子上有什么器件，以及这些器件如何被驱动”。

## 1. 定位

推荐边界：

```text
Service -> Module -> BSP
```

一句话：

```text
Module 定义板上器件能力，BSP 提供 MCU 外设能力，Service 决定系统规则。
```

适合放在 Module：

- `mod_eeprom`：外部 4KB EEPROM 器件驱动。
- `mod_stepper_driver`：STEP/DIR/EN 步进驱动器适配。
- `mod_sensor_input`：光感、限位、位置反馈输入适配。
- `mod_comm_port`：USART/RS485/CAN 通信口适配。
- `mod_rs485`：RS485 DE/RE 方向控制。
- `mod_can_port`：后续 CAN 通信口适配。

不放在 Module：

- 协议解析、阀门模型、运动策略、安全状态机，这些放 `Service`。
- GPIO、TIM、PWM、USART、ADC、IWDG、soft I2C 这些 MCU 外设放 `BSP`。
- 第三方库源码放 `ThirdParty`。

结论：Module 和 BSP 保持分开。BSP 只表达 MCU 外设能力，Module 只表达板上器件能力，避免一个大 BSP 同时混入 EEPROM、电机、传感器和业务规则。

## 2. 目录结构

```text
Module/
  Inc/
    mod_eeprom.h
    mod_stepper_driver.h
    mod_sensor_input.h
    mod_comm_port.h
    mod_rs485.h
  Src/
    mod_eeprom.c
    mod_stepper_driver.c
    mod_sensor_input.c
    mod_comm_port.c
    mod_rs485.c
  CMakeLists.txt
```

初期可以只实现：

```text
mod_eeprom
mod_stepper_driver
mod_sensor_input
mod_comm_port
```

## 3. 头文件规则

Module 公共头文件允许包含：

```c
#include <stdint.h>
#include <stdbool.h>
#include "mod_status.h"
#include "board_config.h"
```

Module 公共头文件不允许包含：

```c
#include "main.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_tim.h"
#include "bsp_private_xxx.h"
```

Module 源文件可以包含 BSP 公共头文件：

```c
#include "bsp_gpio.h"
#include "bsp_pwm.h"
#include "bsp_timer.h"
#include "bsp_soft_i2c.h"
```

## 4. 接口规划

### 4.1 状态码

建议初期建立统一 `mod_status_t`：

```c
typedef enum {
    MOD_OK = 0,
    MOD_ERR_PARAM = -1,
    MOD_ERR_TIMEOUT = -2,
    MOD_ERR_BUSY = -3,
    MOD_ERR_HW = -4,
} mod_status_t;
```

### 4.2 EEPROM

`mod_eeprom` 只处理 EEPROM 器件读写，不理解业务参数结构。

```c
mod_status_t mod_eeprom_Init(void);
mod_status_t mod_eeprom_Read(uint16_t address, uint8_t *data, uint16_t len);
mod_status_t mod_eeprom_Write(uint16_t address, const uint8_t *data, uint16_t len);
mod_status_t mod_eeprom_IsReady(void);
```

边界：

- `mod_eeprom` 可以处理页写、ACK polling、写入校验。
- `mod_eeprom` 不定义通道数、波特率、速度等参数结构。
- 参数结构、双副本、CRC、版本迁移放 `svc_param`。

### 4.3 步进驱动器

`mod_stepper_driver` 只适配驱动器和板上 STEP/DIR/EN 接法。

```c
typedef enum {
    MOD_STEPPER_DIR_FORWARD = 0,
    MOD_STEPPER_DIR_REVERSE,
} mod_stepper_dir_t;

mod_status_t mod_stepper_driver_Init(void);
mod_status_t mod_stepper_driver_Enable(uint8_t channel, bool enable);
mod_status_t mod_stepper_driver_SetDirection(uint8_t channel, mod_stepper_dir_t dir);
mod_status_t mod_stepper_driver_StartPulse(uint8_t channel, uint32_t freq_hz);
mod_status_t mod_stepper_driver_StopPulse(uint8_t channel);
mod_status_t mod_stepper_driver_EmergencyStop(uint8_t channel);
```

边界：

- 脉冲由 TIM/PWM 生成，不用软件延时模拟。
- 距离到步数、速度曲线、错位重走放 `svc_motion` / `svc_safety`。
- 不在 Module 中写阀门状态机。

### 4.4 传感输入

```c
typedef enum {
    MOD_SENSOR_STATE_INACTIVE = 0,
    MOD_SENSOR_STATE_ACTIVE,
} mod_sensor_state_t;

mod_status_t mod_sensor_input_Init(void);
mod_status_t mod_sensor_input_Read(uint8_t channel, mod_sensor_state_t *state);
mod_status_t mod_sensor_input_ReadAll(uint32_t *bitmap);
```

光感数量、极性、是否半通道来自 `BoardConfig`。

### 4.5 通信口适配

```c
typedef void (*mod_comm_port_rx_callback_t)(const uint8_t *data, uint16_t len);

mod_status_t mod_comm_port_Init(void);
mod_status_t mod_comm_port_Send(const uint8_t *data, uint16_t len);
mod_status_t mod_comm_port_RegisterRxCallback(mod_comm_port_rx_callback_t callback);
```

`mod_comm_port` 适配 USART/RS485/CAN 物理通道；协议解析放 `svc_protocol`。

## 5. CMake

```cmake
add_library(module OBJECT)

target_sources(module PRIVATE
    Src/mod_eeprom.c
    Src/mod_stepper_driver.c
    Src/mod_sensor_input.c
    Src/mod_comm_port.c
)

target_include_directories(module PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Inc
)

target_link_libraries(module PUBLIC
    bsp
    selected_board_config
)
```

规则：

- `module` 可以依赖 `bsp` 和 `selected_board_config`。
- `module` 不直接依赖 `stm32cubemx`。
- 如果需要 ringbuffer 等第三方组件，依赖对应 ThirdParty target。

## 6. 错误处理

Module 对上返回 `mod_status_t`，对下调用 BSP 时接收 `bsp_status_t`。不要把 BSP 状态码直接继续抛给 Service。

推荐映射：

```text
BSP_ERR_PARAM       -> MOD_ERR_PARAM
BSP_ERR_BUSY        -> MOD_ERR_BUSY
BSP_ERR_TIMEOUT     -> MOD_ERR_TIMEOUT
BSP_ERR_UNSUPPORTED -> MOD_ERR_HW
BSP_ERR_IO          -> MOD_ERR_HW
BSP_ERR_OVERFLOW    -> MOD_ERR_HW
BSP_ERR_HW          -> MOD_ERR_HW
```

如果需要保留诊断信息，Module 可以保存最近一次底层错误：

```c
bsp_status_t mod_eeprom_GetLastBspStatus(void);
```

该接口只用于调试、日志或工厂诊断，正常控制流程仍使用 `mod_status_t`。

## 7. 测试

Module 测试优先 mock BSP：

```text
test_mod_eeprom.c
  -> mock_bsp_soft_i2c

test_mod_stepper_driver.c
  -> mock_bsp_pwm
  -> mock_bsp_gpio
  -> mock_bsp_timer
```

优先覆盖：

- EEPROM 页边界。
- EEPROM 超时和 NACK。
- STEP 启停顺序。
- 急停是否立即停止脉冲。
- 光感极性配置是否正确。

## 8. 执行计划

项目级详细 TODO 统一维护在 [about.md](about.md)。Module 本层开发只保留以下检查重点：

- 公共头文件不包含 LL/HAL/CubeMX 类型。
- Module 不向 Service 透传 `bsp_status_t`。
- EEPROM、电机、传感器、通信口只表达板上器件能力。
- 协议、参数、阀门模型、运动策略、安全策略不放在 Module。
- Module 测试优先通过 CMock mock BSP。
