# BSP 开发指南

返回：[工程文档总入口](about.md)

本文档固定本项目 BSP、Module、Service、Application 的职责边界，并规定 BSP 层如何编写。相邻目录请配合阅读：[Module 开发指南](module_development_guide.md)、[Service 开发指南](service_development_guide.md)、[Application 开发指南](application_development_guide.md)、[BoardConfig 开发指南](boardconfig_development_guide.md)。

本次边界修订采用以下原则：

```text
BSP 只封装 MCU 外设能力
Module 封装板上器件、执行器和通信口适配
Service 封装协议、参数、阀门、运动和安全策略
Application 编排运行模式和业务流程
```

这意味着 EEPROM、电机驱动、光感传感器等“板上器件/功能器件”不放 BSP。BSP 只提供 GPIO、USART、TIM、PWM、ADC、DWT、soft I2C、CRC、IWDG 等 MCU 外设接口。

## 1. 分层结论

结论：BSP 只体现 MCU 外设；EEPROM、电机驱动、光感/限位等板上器件全部放 Module，并通过 BSP 接口访问硬件。

简要原因：

- EEPROM 是板上器件，不是 STM32F103 的片上外设。它的页大小、设备地址、写周期、双副本策略、CRC、参数布局都不应污染 BSP。
- 42 步进电机、驱动芯片、光感输入、限位输入组成的是执行器系统，不是单一 MCU 外设。它会涉及距离、速度、方向、急停、错位重走，应该放 Module。
- BSP 如果同时出现 `bsp_soft_i2c` 和 `bsp_eeprom`，会让“总线”和“总线上的器件”混在同一层。
- BSP 如果同时出现 `bsp_pwm` / `bsp_timer` 和 `bsp_stepper_pulse`，会让“MCU 定时器能力”和“步进电机执行器能力”混在同一层。
- Module 更容易在 PC 单元测试中 mock BSP，测试 EEPROM 页写、参数保存、电机状态机、急停策略。

推荐依赖：

```text
mod_eeprom
  -> bsp_soft_i2c 或 bsp_i2c
  -> bsp_time / bsp_dwt

mod_stepper_driver
  -> bsp_gpio
  -> bsp_timer / bsp_pwm
  -> bsp_time

svc_motion
  -> mod_stepper_driver
  -> svc_safety
```

## 2. 总体依赖方向

固定依赖方向：

```text
Application -> Service -> Module -> BSP -> CubeMX/LL/CMSIS
```

禁止依赖：

```text
BSP -> Module
BSP -> Application
Module -> Service
Module -> Application
Module -> CubeMX/LL/CMSIS
Service -> BSP
Application -> BSP
Application -> CubeMX/LL/CMSIS
```

说明：

- `Core/Src/main.c` 只调用 `app_Init()` 和 `app_MainLoop()`。
- `app_Init()` 可以间接触发 Service、Module 和 BSP 初始化，但不建议直接操作某个 `bsp_xxx` 外设。
- Service 可以调用 Module。
- Module 可以调用 BSP。
- BSP 不能调用 Module。

## 3. 分层职责边界

### 3.1 BSP 层

BSP 是 MCU Peripheral Abstraction Layer。虽然目录名仍叫 BSP，但在本项目中它只负责 STM32 片上外设和 CubeMX/LL 的封装。

BSP 负责：

- 封装 MCU GPIO、USART、TIM、PWM、ADC、CRC、IWDG、DWT、soft I2C 等外设能力。
- 隐藏 CubeMX 生成的句柄、LL 调用、寄存器操作和中断细节。
- 提供稳定、短小、可 mock 的 C 接口给 Module。
- 在 ISR 中只做快速硬件处理，例如读写寄存器、清中断、写 ringbuffer、设置 flag。

BSP 不负责：

- 不出现 EEPROM 设备型号、页大小、设备地址。
- 不出现步进电机、阀门、光感、限位、驱动芯片等板上器件抽象。
- 不解析 Modbus 或业务协议。
- 不保存参数结构。
- 不做错位重走、老化模式、工厂模式。
- 不直接调用 Module 或 Application。

BSP 判断标准：

```text
如果问题是“STM32 这个外设怎么用”，放 BSP。
如果问题是“板上这个器件怎么工作”，放 Module。
如果问题是“系统在某种模式下要做什么”，放 Application。
```

### 3.2 Module 层

Module 是板上器件和硬件模块适配层，负责把 BSP 外设组合成板级可用的硬件模块。

Module 负责：

- `mod_eeprom`：外部 EEPROM 器件驱动，使用 `bsp_soft_i2c` 或后续 `bsp_i2c`。
- `mod_stepper_driver`：42 步进电机执行器驱动，使用 `bsp_gpio`、`bsp_pwm`、`bsp_timer`。
- `mod_sensor_input`：光感、限位、位置反馈硬件适配。
- `mod_comm_port`：USART/RS485/CAN 通信口适配。

Module 不负责：

- 不直接包含 `main.h`、`gpio.h`、`tim.h`、`usart.h`。
- 不直接调用 LL/HAL。
- 不修改 CubeMX 生成代码。
- 不编排工厂模式/老化模式完整流程。
- 不实现协议、参数、阀门、运动和安全策略。

### 3.3 Service 层

Service 是业务服务层，负责协议、参数、阀门、运动、安全等可测试规则。

Service 负责：

- `svc_protocol`：Modbus/生产协议解析和封包。
- `svc_param`：参数结构、双副本、CRC、版本迁移、默认值。
- `svc_motion`：初期统一承载两位阀/多位阀模型、距离、速度、步数换算、运动状态机和到位判断。
- `svc_safety`：急停、错位重走、超时、故障保持。
- `svc_system`：Service/Module/BSP 初始化和轮询聚合入口。

`svc_valve`、`svc_position` 是后续复杂化后的拆分项，不建议初期强制建立。

Service 不负责：

- 不直接调用 BSP。
- 不直接包含 LL/HAL/CubeMX 头文件。
- 不保存具体板卡 pin map。

### 3.4 Application 层

Application 是系统业务流程层。

Application 负责：

- `app_Init()`：应用初始化入口。
- `app_MainLoop()`：主循环入口。
- Normal Mode、Factory Mode、Aging Mode 的流程编排。
- 协议命令到 Service 功能的编排。
- 系统级故障处理和降级策略。

Application 不负责：

- 不直接操作 BSP 外设。
- 不直接调用 Module。
- 不直接读写 EEPROM。
- 不直接产生 STEP 脉冲。
- 不直接调用 LL/HAL。

## 4. 推荐目录结构

### 4.1 BSP 目录

```text
BSP/
  Inc/
    bsp.h
    bsp_status.h
    bsp_adc.h
    bsp_crc.h
    bsp_dwt.h
    bsp_gpio.h
    bsp_iwdg.h
    bsp_pwm.h
    bsp_soft_i2c.h
    bsp_timer.h
    bsp_usart.h
  Src/
    bsp.c
    bsp_adc.c
    bsp_crc.c
    bsp_dwt.c
    bsp_gpio.c
    bsp_iwdg.c
    bsp_pwm.c
    bsp_soft_i2c.c
    bsp_timer.c
    bsp_usart.c
  CMakeLists.txt
```

BSP 中不建立：

```text
bsp_eeprom.c / bsp_eeprom.h
bsp_stepper_pulse.c / bsp_stepper_pulse.h
bsp_motor.c / bsp_motor.h
bsp_opto.c / bsp_opto.h
bsp_valve.c / bsp_valve.h
```

### 4.2 Module 目录

```text
Module/
  Inc/
    mod_eeprom.h
    mod_stepper_driver.h
    mod_sensor_input.h
    mod_comm_port.h
    mod_rs485.h
    mod_can_port.h
  Src/
    mod_eeprom.c
    mod_stepper_driver.c
    mod_sensor_input.c
    mod_comm_port.c
    mod_rs485.c
    mod_can_port.c
  CMakeLists.txt
```

说明：

- `mod_eeprom` 是器件驱动。
- `mod_stepper_driver` 是执行器驱动。
- `mod_sensor_input` 是光感/限位等输入适配。
- `mod_comm_port` 是通信口适配。

### 4.3 Service 目录

```text
Service/
  Inc/
    svc_system.h
    svc_protocol.h
    svc_param.h
    svc_motion.h
    svc_safety.h
  Src/
    svc_system.c
    svc_protocol.c
    svc_param.c
    svc_motion.c
    svc_safety.c
  CMakeLists.txt
```

## 5. BSP 外设取舍表

| BSP 模块 | 首版建议 | 边界说明 |
| --- | --- | --- |
| `bsp_status` | 必须 | BSP 统一返回值 |
| `bsp` | 必须 | BSP 初始化和轮询入口 |
| `bsp_gpio` | 必须 | 只提供 MCU GPIO 读写、中断配置，不表达光感/电机 EN/DIR 业务名 |
| `bsp_usart` | 必须 | 只提供串口收发、DMA/中断、ringbuffer 接入，不解析协议 |
| `bsp_time` | 必须 | 毫秒 tick 和超时计算 |
| `bsp_dwt` | 建议 | 微秒延时、soft I2C 时序、短耗时测量 |
| `bsp_soft_i2c` | 视硬件决定 | 只提供 I2C 总线 start/stop/read/write，不理解 EEPROM |
| `bsp_timer` | 建议 | 通用 TIM 周期、计数、比较能力 |
| `bsp_pwm` | 建议 | 通用 PWM 输出能力，不理解 STEP 脉冲语义 |
| `bsp_adc` | 可选 | 只提供 ADC 通道采样，不理解电流/电压业务含义 |
| `bsp_crc` | 可选 | STM32 硬件 CRC-32 或软件 CRC 包装；Modbus CRC-16 不强放 BSP |
| `bsp_iwdg` | 必须 | 独立看门狗初始化和喂狗接口 |
| `bsp_wwdg` | 暂缓 | 初期不加入，等系统周期稳定后再评估 |

首版最小 BSP 集合：

```text
bsp_status
bsp
bsp_gpio
bsp_usart
bsp_time
bsp_dwt
bsp_soft_i2c
bsp_timer
bsp_pwm
bsp_iwdg
```

如果第一版暂时不使用 ADC/CRC/WWDG，可以不加入 `bsp_Init()` 和 `BSP/CMakeLists.txt`。

## 6. BSP 状态码与错误处理

`bsp_status_t` 用于统一 BSP 层函数返回值，让 Module 不直接依赖 STM32 LL/HAL、国产 MCU SDK 或寄存器错误位。

推荐规则：

- BSP 对外函数返回 `bsp_status_t`。
- `BSP_OK = 0`，错误值全部为负数。
- 返回值用于流程控制，详细外设错误用于诊断。
- 不把 `HAL_StatusTypeDef`、LL flag、寄存器位暴露给 Module/Service。

推荐首版定义：

```c
typedef enum {
    BSP_OK = 0,
    BSP_ERR_PARAM = -1,
    BSP_ERR_NOT_INIT = -2,
    BSP_ERR_BUSY = -3,
    BSP_ERR_TIMEOUT = -4,
    BSP_ERR_UNSUPPORTED = -5,
    BSP_ERR_IO = -6,
    BSP_ERR_OVERFLOW = -7,
    BSP_ERR_HW = -8,
} bsp_status_t;
```

常见映射：

| 底层现象 | 对外返回 |
| --- | --- |
| 空指针、非法通道、非法频率 | `BSP_ERR_PARAM` |
| 外设未初始化 | `BSP_ERR_NOT_INIT` |
| USART/I2C/SPI 忙 | `BSP_ERR_BUSY` |
| 等待标志位超时 | `BSP_ERR_TIMEOUT` |
| 当前板卡未接该外设或通道 | `BSP_ERR_UNSUPPORTED` |
| I2C NACK、USART 帧错误、SPI 传输错误 | `BSP_ERR_IO` |
| ringbuffer 满、DMA 缓冲溢出 | `BSP_ERR_OVERFLOW` |
| 无法细分的硬件异常 | `BSP_ERR_HW` |

如果某个外设需要诊断细节，提供外设级 `GetLastError()`：

```c
typedef enum {
    BSP_USART_ERR_NONE = 0,
    BSP_USART_ERR_OVERRUN,
    BSP_USART_ERR_FRAME,
    BSP_USART_ERR_NOISE,
    BSP_USART_ERR_DMA,
} bsp_usart_error_t;

bsp_usart_error_t bsp_usart_GetLastError(void);
```

调用关系建议：

```text
BSP 返回 bsp_status_t
Module 将 bsp_status_t 转换为 mod_status_t
Service 将 mod_status_t 转换为 svc_status_t 或故障码
Application 只处理模式、告警和降级流程
```

这样未来切换 MCU 或 SDK 时，只需要在 BSP 内部重新映射底层错误。

## 7. EEPROM 分层方案

### 7.1 为什么 EEPROM 放 Module

外部 EEPROM 是板上器件，不是 MCU 外设。它依赖 I2C/SPI/其他总线，但它本身的行为包含：

- 器件地址。
- 页大小。
- 写周期。
- ACK polling。
- 跨页写。
- 写后校验。
- 容量范围。

这些都属于器件驱动，应该放 `mod_eeprom`。

### 7.2 推荐依赖

```text
svc_param
  -> mod_eeprom
      -> bsp_soft_i2c
          -> bsp_gpio / bsp_dwt
```

### 7.3 接口建议

`bsp_soft_i2c`：

```c
bsp_status_t bsp_soft_i2c_Init(void);
bsp_status_t bsp_soft_i2c_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len);
bsp_status_t bsp_soft_i2c_Read(uint8_t dev_addr, uint8_t *data, uint16_t len);
```

`mod_eeprom`：

```c
mod_status_t mod_eeprom_Init(void);
mod_status_t mod_eeprom_Read(uint16_t addr, uint8_t *data, uint16_t len);
mod_status_t mod_eeprom_Write(uint16_t addr, const uint8_t *data, uint16_t len);
mod_status_t mod_eeprom_IsReady(void);
```

`svc_param`：

```c
svc_status_t svc_param_Load(void);
svc_status_t svc_param_Save(void);
svc_status_t svc_param_GetConfig(svc_param_config_t *config);
svc_status_t svc_param_SetConfig(const svc_param_config_t *config);
```

边界：

- `bsp_soft_i2c` 不知道 EEPROM。
- `mod_eeprom` 不知道参数结构。
- `svc_param` 不知道 I2C 时序。

## 8. 电机驱动分层方案

### 8.1 为什么电机驱动放 Module

电机驱动不是单一 MCU 外设。它至少组合了：

- TIM/PWM 输出 STEP。
- GPIO 输出 DIR/EN。
- GPIO/EXTI 读取光感或限位。
- 时间超时。
- 步数统计。
- 急停和状态机。

这些已经是板上执行器驱动，应放 `mod_stepper_driver`，运动控制策略放 `svc_motion`，都不放 BSP。

### 8.2 推荐依赖

```text
svc_motion
  -> mod_stepper_driver
  -> mod_sensor_input
      -> bsp_pwm / bsp_timer / bsp_gpio / bsp_adc
```

如果需要安全策略：

```text
svc_safety
  -> svc_motion
  -> event / log
```

### 8.3 接口建议

`bsp_pwm`：

```c
bsp_status_t bsp_pwm_Start(uint8_t channel, uint32_t frequency_hz, uint16_t duty_permille);
bsp_status_t bsp_pwm_Stop(uint8_t channel);
```

`bsp_gpio`：

```c
bsp_status_t bsp_gpio_Write(uint16_t pin_id, bool level);
bsp_status_t bsp_gpio_Read(uint16_t pin_id, bool *level);
```

`mod_stepper_driver`：

```c
typedef enum {
    MOD_STEPPER_DIR_FORWARD = 0,
    MOD_STEPPER_DIR_REVERSE,
} mod_stepper_dir_t;

mod_status_t mod_stepper_driver_Init(void);
mod_status_t mod_stepper_driver_Enable(uint8_t channel, bool enable);
mod_status_t mod_stepper_driver_SetDirection(uint8_t channel, mod_stepper_dir_t dir);
mod_status_t mod_stepper_driver_StartPulse(uint8_t channel, uint32_t steps, uint32_t frequency_hz);
mod_status_t mod_stepper_driver_Stop(uint8_t channel);
mod_status_t mod_stepper_driver_EmergencyStop(uint8_t channel);
uint32_t mod_stepper_driver_GetDoneSteps(uint8_t channel);
```

`svc_motion`：

```c
svc_status_t svc_motion_MoveDistance(uint8_t channel, int32_t distance_um, uint32_t speed);
svc_status_t svc_motion_Stop(uint8_t channel);
void svc_motion_Poll(void);
```

边界：

- `bsp_pwm` 不知道 STEP。
- `bsp_gpio` 不知道 DIR/EN/光感。
- `mod_stepper_driver` 知道 STEP/DIR/EN 和通道映射。
- `svc_motion` 知道距离、速度、运动状态。
- `svc_safety` 知道急停后的重走和故障保持。

## 9. GPIO 命名与板级映射

BSP 不表达具体板上器件名，但总要知道哪个 MCU 引脚被操作。推荐使用项目内的 GPIO ID 映射表。

`bsp_gpio.h` 对外暴露通用 pin id：

```c
typedef uint16_t bsp_gpio_id_t;

bsp_status_t bsp_gpio_Write(bsp_gpio_id_t id, bool level);
bsp_status_t bsp_gpio_Read(bsp_gpio_id_t id, bool *level);
```

具体 ID 可由 Module 配置文件管理：

```c
typedef struct {
    bsp_gpio_id_t step_pin;
    bsp_gpio_id_t dir_pin;
    bsp_gpio_id_t en_pin;
    bsp_gpio_id_t opto_pin;
    uint8_t pwm_channel;
} mod_stepper_driver_channel_config_t;
```

这样：

- BSP 只负责 id 到真实端口/引脚的映射。
- Module 负责说明这个 id 是 STEP、DIR、EN 还是光感。
- 换板时优先改 BSP pin map 或 Module 配置表，不改控制逻辑。

## 10. `bsp.c` / `bsp.h` 的职责

建议保留 `bsp.c` / `bsp.h`，但它只统一初始化 MCU 外设抽象，不初始化 EEPROM、电机驱动等板上器件。

`bsp.h`：

```c
#ifndef BSP_H
#define BSP_H

#include "bsp_status.h"

bsp_status_t bsp_Init(void);
void bsp_Poll(void);

#endif
```

`bsp.c`：

```c
#include "bsp.h"

#include "bsp_dwt.h"
#include "bsp_gpio.h"
#include "bsp_iwdg.h"
#include "bsp_pwm.h"
#include "bsp_soft_i2c.h"
#include "bsp_time.h"
#include "bsp_timer.h"
#include "bsp_usart.h"

bsp_status_t bsp_Init(void)
{
    if (bsp_gpio_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    if (bsp_dwt_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    if (bsp_timer_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    if (bsp_timer_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    if (bsp_pwm_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    if (bsp_usart_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    if (bsp_soft_i2c_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    if (bsp_iwdg_Init() != BSP_OK) {
        return BSP_ERR_HW;
    }

    return BSP_OK;
}

void bsp_Poll(void)
{
    bsp_usart_Poll();
}
```

注意：

- `bsp.h` 不是万能头文件。
- `bsp.c` 不包含 `mod_eeprom.h`、`mod_stepper_driver.h`。
- 可选外设未启用时，不放进 `bsp_Init()`。

## 11. 初始化顺序与调用者

推荐启动顺序：

```text
main()
  -> SystemClock_Config()
  -> MX_GPIO_Init()
  -> MX_DMA_Init()
  -> MX_USARTx_Init()
  -> MX_TIMx_Init()
  -> MX_ADC_Init()
  -> app_Init()
      -> svc_system_Init()
          -> bsp_Init()
          -> mod_eeprom_Init()
          -> mod_stepper_driver_Init()
          -> svc_param_Load()
          -> svc_protocol_Init()
          -> svc_motion_Init()
          -> svc_safety_Init()
      -> app mode init
  -> while (1)
      -> app_MainLoop()
          -> svc_system_Poll()
          -> app mode poll
```

推荐由 `svc_system_Init()` 调用 `bsp_Init()`。

原因：

- `Application` 不直接依赖 BSP，分层更干净。
- `svc_system` 是 Service 层系统能力聚合入口，负责初始化 Service、Module 及其依赖的 BSP。
- 初学者仍然只需要从 `app_Init()` 进入阅读。

示例：

```c
void app_Init(void)
{
    if (svc_system_Init() != SVC_OK) {
        app_EnterFaultMode();
        return;
    }

    app_mode_Init();
}
```

```c
svc_status_t svc_system_Init(void)
{
    if (bsp_Init() != BSP_OK) {
        return SVC_ERR_HW;
    }

    if (mod_eeprom_Init() != MOD_OK) {
        return SVC_ERR_HW;
    }

    if (svc_param_Load() != SVC_OK) {
        return SVC_ERR_CONFIG;
    }

    return MOD_OK;
}
```

可接受的过渡方案：

- 项目早期也可以暂时由 `app_Init()` 调 `bsp_Init()`。
- 但最终文档推荐收敛到 `app_Init() -> svc_system_Init() -> bsp_Init()`。

## 12. 头文件引用规则

BSP public header 允许包含：

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bsp_status.h"
```

BSP public header 不建议包含：

```c
#include "main.h"
#include "gpio.h"
#include "usart.h"
#include "tim.h"
#include "stm32f1xx_ll_gpio.h"
```

Module 可以包含：

```c
#include "bsp_gpio.h"
#include "bsp_pwm.h"
#include "bsp_soft_i2c.h"
#include "bsp_usart.h"
```

Module 不允许包含：

```c
#include "main.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_tim.h"
```

Application 可以包含：

```c
#include "svc_system.h"
#include "svc_protocol.h"
#include "svc_motion.h"
```

Application 不建议包含：

```c
#include "bsp_gpio.h"
#include "bsp_usart.h"
```

## 13. CMake 组织建议

`BSP/CMakeLists.txt`：

```cmake
add_library(bsp OBJECT
    Src/bsp.c
    Src/bsp_adc.c
    Src/bsp_crc.c
    Src/bsp_dwt.c
    Src/bsp_gpio.c
    Src/bsp_iwdg.c
    Src/bsp_pwm.c
    Src/bsp_soft_i2c.c
    Src/bsp_timer.c
    Src/bsp_usart.c
)

target_include_directories(bsp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Inc
)

target_link_libraries(bsp
    PUBLIC
        stm32cubemx
)
```

`Module/CMakeLists.txt`：

```cmake
add_library(module OBJECT
    Src/mod_eeprom.c
    Src/mod_stepper_driver.c
    Src/mod_sensor_input.c
    Src/mod_comm_port.c
)

target_include_directories(module
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Inc
)

target_link_libraries(module
    PUBLIC
        bsp
)
```

`Service/CMakeLists.txt`：

```cmake
add_library(service OBJECT
    Src/svc_system.c
    Src/svc_protocol.c
    Src/svc_param.c
    Src/svc_motion.c
    Src/svc_safety.c
)

target_include_directories(service
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Inc
)

target_link_libraries(service
    PUBLIC
        module
        ringbuffer
        modbus
)
```

`Application/CMakeLists.txt`：

```cmake
add_library(application OBJECT
    Src/app.c
    Src/app_mode.c
    Src/app_guardian.c
    Src/app_version.c
)

target_include_directories(application
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Inc
)

target_link_libraries(application
    PUBLIC
        service
)
```

## 14. 执行计划

项目级详细 TODO 统一维护在 [about.md](about.md)。BSP 本层开发只保留以下检查重点：

- BSP 只封装 MCU 外设，不新增 `bsp_eeprom`、`bsp_stepper` 等板上器件语义。
- `bsp.h` / `bsp.c` 只提供 BSP 聚合初始化和必要轮询，不作为万能包含头。
- BSP 对外函数统一返回 `bsp_status_t`。
- 需要诊断细节的外设提供 `bsp_xxx_GetLastError()`，普通业务流程不依赖底层错误位。
- `bsp_wwdg` 初期暂缓，等主循环周期和喂狗策略稳定后再评估。

## 15. 检查清单

新增代码时检查：

- 是否把 MCU 外设能力放 BSP？
- 是否把板上器件驱动放 Module？
- 是否把业务规则放 Service？
- BSP public header 是否没有 LL/HAL 类型？
- Module public header 是否没有 STM32 寄存器类型？
- Application 是否没有包含 `bsp_xxx.h` 和 `mod_xxx.h`？
- `bsp.c` 是否没有包含 `mod_xxx.h`？
- `svc_system_Init()` 是否是 BSP 初始化的唯一入口？
- BSP 对外接口是否统一返回 `bsp_status_t`？
- 是否没有把 HAL/LL/SDK 错误码暴露给 Module？
- 单元测试是否可以通过 mock BSP 覆盖 Module？

最终边界一句话：

```text
BSP 提供 MCU 会什么；Module 定义板子有什么；Service 定义系统能力怎么工作；Application 决定当前模式做什么。
```
