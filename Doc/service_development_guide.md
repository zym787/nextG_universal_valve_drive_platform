# Service 开发指南

返回：[工程文档总入口](about.md)

本文档说明 `Service` 层如何编写。`Service` 是系统能力层，负责协议、参数、运动、安全等可测试逻辑；它不是 RTOS 任务框架，也不是“大而全中间件”。

## 1. 定位

推荐边界：

```text
Application -> Service -> Module
```

一句话：

```text
Application 决定当前模式做什么，Service 决定系统规则怎么执行。
```

适合放在 Service：

- `svc_system`：Service 内部初始化和轮询聚合，不负责 BSP/Module 初始化。
- `svc_protocol`：协议解析、命令分发、响应封包。
- `svc_param`：参数校验、默认值、EEPROM 双副本、CRC、版本迁移。
- `svc_motion`：距离、速度、步数换算、运动状态机。
- `svc_safety`：急停、超时、错位重走、故障保持。
- `svc_watchdog`：关键任务 alive 上报和条件喂狗策略。

后续复杂后再拆：

- `svc_valve`：两位阀/多位阀模型。
- `svc_position`：闭环位置判断、滤波。
- `svc_upgrade`：Bootloader/A-B 分区升级服务。

结论：初期只保留少量 Service。`svc_motion` 先承载阀门模型、距离换算和运动状态机，复杂度出现后再拆 `svc_valve`、`svc_position`。

## 2. Application 与 Service 边界

Application 薄是合理的。它应更像模式和任务编排：

```text
app_Init()
app_MainLoop()
app_normal_Poll()
app_factory_Poll()
app_aging_Poll()
app_guardian_Poll()
```

Service 承载可测试规则：

```text
svc_protocol_Poll()
svc_motion_Poll()
svc_safety_Poll()
svc_param_Save()
```

后续引入 FreeRTOS/RT-Thread 时，Application 的 poll 可以自然迁移为任务入口。

## 3. 目录结构

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

初期推荐不要超过 5 个 Service 文件。

## 4. 头文件规则

Service 公共头文件允许包含：

```c
#include <stdint.h>
#include <stdbool.h>
#include "svc_status.h"
```

Service 源文件可以包含 Module 头文件：

```c
#include "mod_eeprom.h"
#include "mod_stepper_driver.h"
#include "mod_sensor_input.h"
#include "mod_comm_port.h"
```

Service 不允许包含：

```c
#include "main.h"
#include "bsp_gpio.h"
#include "stm32f1xx_ll_gpio.h"
```

## 5. 接口规划

### 5.1 状态码

```c
typedef enum {
    SVC_OK = 0,
    SVC_ERR_PARAM = -1,
    SVC_ERR_TIMEOUT = -2,
    SVC_ERR_BUSY = -3,
    SVC_ERR_STATE = -4,
    SVC_ERR_HW = -5,
} svc_status_t;
```

### 5.2 系统聚合

`svc_system` 是 Service 内部初始化和轮询入口：

```c
svc_status_t svc_system_Init(void);
void svc_system_Poll(void);
```

推荐初始化链路由 Application 组合根编排：

```text
main.c
  -> app_Init()
      -> app_system_Init()
          -> bsp_Init()
          -> mod_eeprom_Init()
          -> mod_stepper_driver_Init()
          -> mod_comm_port_Init()
          -> svc_system_Init()
              -> svc_param_Init()
              -> svc_protocol_Init()
              -> svc_motion_Init()
              -> svc_safety_Init()
              -> svc_watchdog_Init()
```

这样 `Service` 不需要知道 BSP 初始化顺序，也不会变成“万能系统层”。`svc_system` 只管理 Service 自己的子服务。

### 5.3 参数服务

```c
typedef struct {
    uint8_t modbus_address;
    uint32_t baudrate;
    uint8_t channel_count;
    uint8_t half_channel_enabled;
    uint16_t default_speed;
    uint8_t max_retry_count;
    uint16_t move_timeout_ms;
} svc_param_config_t;

svc_status_t svc_param_Init(void);
svc_status_t svc_param_Load(void);
svc_status_t svc_param_Save(void);
svc_status_t svc_param_GetConfig(svc_param_config_t *config);
svc_status_t svc_param_SetConfig(const svc_param_config_t *config);
```

规则：

- EEPROM 读写通过 `mod_eeprom`。
- 参数保存使用 RAM shadow + 显式保存 EEPROM。
- MCU 端不解析 JSON/INI。

### 5.4 运动服务

```c
typedef enum {
    SVC_MOTION_STATE_IDLE = 0,
    SVC_MOTION_STATE_MOVING,
    SVC_MOTION_STATE_STOPPING,
    SVC_MOTION_STATE_FAULT,
} svc_motion_state_t;

svc_status_t svc_motion_Init(void);
svc_status_t svc_motion_MoveSteps(uint8_t channel, uint32_t steps, uint16_t speed);
svc_status_t svc_motion_MoveToPosition(uint8_t channel, uint8_t position);
svc_status_t svc_motion_Stop(uint8_t channel);
svc_motion_state_t svc_motion_GetState(uint8_t channel);
void svc_motion_Poll(void);
```

规则：

- 不生成软件延时脉冲。
- 不直接访问 GPIO/TIM。
- 通过 `mod_stepper_driver` 执行 STEP/DIR/EN。
- 不在 1ms `Poll()` 中做微秒级脉冲计数；目标步数停止、急停关断由 Module/BSP 快速路径完成。

### 5.5 安全服务

```c
typedef enum {
    SVC_FAULT_NONE = 0,
    SVC_FAULT_EMERGENCY_STOP,
    SVC_FAULT_MOVE_TIMEOUT,
    SVC_FAULT_POSITION_MISMATCH,
    SVC_FAULT_RETRY_EXCEEDED,
} svc_fault_t;

svc_status_t svc_safety_Init(void);
void svc_safety_Poll(void);
svc_fault_t svc_safety_GetFault(void);
svc_status_t svc_safety_ClearFault(void);
```

## 6. CMake

```cmake
add_library(service OBJECT)

target_sources(service PRIVATE
    Src/svc_system.c
    Src/svc_protocol.c
    Src/svc_param.c
    Src/svc_motion.c
    Src/svc_safety.c
)

target_include_directories(service PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Inc
)

target_link_libraries(service PUBLIC
    module
    ringbuffer
    modbus
    event
    log
)
```

规则：

- `service` 可以依赖 `module` 和纯软件 ThirdParty。
- `service` 不直接依赖 `bsp`、`stm32cubemx`。
- `application` 的模式/业务代码只依赖 `service`；`app_system.c` 作为组合根可例外编排初始化。

## 7. 错误处理

Service 对上返回 `svc_status_t` 或业务故障码，对下调用 Module 时接收 `mod_status_t`。

推荐规则：

- 参数、协议、运动、安全等普通 API 返回 `svc_status_t`。
- 可恢复硬件问题映射为 `SVC_ERR_HW` 或 `SVC_ERR_TIMEOUT`。
- 阀门错位、急停、重走超限等运行故障映射为 `svc_fault_t`，供 Application 和协议读取。
- Service 不直接读取 `bsp_xxx_GetLastError()`，需要诊断时通过 Module 提供的接口间接读取。

示例：

```text
mod_stepper_driver_EmergencyStop() -> MOD_OK
svc_safety_GetFault()              -> SVC_FAULT_EMERGENCY_STOP
```

这样流程控制和故障诊断可以分开，避免业务层到处判断底层外设错误。

## 8. 测试

Service 测试优先 mock Module：

```text
test_svc_param.c
  -> mock_mod_eeprom

test_svc_motion.c
  -> mock_mod_stepper_driver
  -> mock_mod_sensor_input

test_svc_protocol.c
  -> ringbuffer / fake command input
```

优先覆盖：

- 参数 CRC、版本、默认值、非法范围。
- 协议帧长度、CRC、非法命令。
- 运动距离换算、状态迁移、超时。
- 急停、错位重走、故障保持。

## 9. 执行计划

项目级详细 TODO 统一维护在 [about.md](about.md)。Service 本层开发只保留以下检查重点：

- Service 不包含 `bsp_xxx.h`、LL/HAL/CubeMX 头文件。
- Service 不直接读取 BSP last error。
- 参数、协议、运动、安全逻辑尽量写成 PC 可测试的纯 C。
- 硬件访问必须通过 Module。
- 后续复杂后再拆 `svc_valve`、`svc_position`、`svc_upgrade`。
