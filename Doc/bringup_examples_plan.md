# Bring-up 示例与任务规划

返回：[工程文档总入口](about.md)

本文档回答：IWDG 定时喂狗、LED 闪烁、通信测试、协议解析、任务调度应该如何编写，以及如何避免为了一个简单 LED 闪烁而把代码写得过度分层。

## 1. 先给结论

不是所有代码都必须完整走四层。

本项目建议分成两条路径：

```text
Bring-up / 诊断路径：用于早期点灯、串口 echo、外设自测，可以适度走短路径
Product / 正式路径：用于产品功能，必须遵守 Application -> Service -> Module -> BSP
```

短路径只能用于以下场景：

- 新板子上电验证。
- 时钟、GPIO、USART、IWDG、TIM 等基础外设确认。
- 工程师临时排查硬件。
- HIL 或工厂诊断中明确隔离的测试入口。

短路径必须满足：

- 使用编译宏隔离，例如 `APP_ENABLE_BRINGUP`。
- 不参与 Release 正式功能。
- 不进入 Normal Mode 的业务流程。
- 文件名明确，例如 `app_bringup.c`、`bsp_selftest.c`。
- TODO 完成后要么删除，要么迁移到正式分层。

## 2. 推荐目录

```text
Application/
  Inc/
    app.h
    app_bringup.h
  Src/
    app.c
    app_bringup.c

Service/
  Inc/
    svc_system.h
    svc_watchdog.h
    svc_protocol.h
    svc_diag.h
  Src/
    svc_system.c
    svc_watchdog.c
    svc_protocol.c
    svc_diag.c

Module/
  Inc/
    mod_comm_port.h
    mod_indicator.h
  Src/
    mod_comm_port.c
    mod_indicator.c

BSP/
  Inc/
    bsp_gpio.h
    bsp_iwdg.h
    bsp_usart.h
    bsp_timer.h
  Src/
    bsp_gpio.c
    bsp_iwdg.c
    bsp_usart.c
    bsp_timer.c
```

说明：

- `app_bringup` 是早期调试入口，可以直接调用 BSP，但必须受 `APP_ENABLE_BRINGUP` 控制。
- `mod_indicator` 是正式产品路径中的指示灯模块，只有 LED 变成产品状态显示时才需要。
- `svc_diag` 是工厂诊断服务，不建议一开始就做很大。

## 3. LED 闪烁

### 3.1 初期 Bring-up 写法

目的：验证时钟、GPIO、主循环是否正常。

允许短路径：

```text
app_bringup_Poll()
  -> bsp_timer_GetMillis()
  -> bsp_gpio_Toggle(BOARD_PIN_LED_STATUS)
```

示例：

```c
#if APP_ENABLE_BRINGUP

#include "app_bringup.h"
#include "bsp_gpio.h"
#include "bsp_timer.h"
#include "board_config.h"

void app_bringup_Poll(void)
{
    static uint32_t last_ms = 0;
    uint32_t now_ms = bsp_timer_GetMillis();

    if ((now_ms - last_ms) >= 500U) {
        last_ms = now_ms;
        (void)bsp_gpio_Toggle(BOARD_PIN_LED_STATUS);
    }
}

#endif
```

如果暂时没有 `bsp_gpio_Toggle()`，可以先用 `bsp_gpio_Read()` + `bsp_gpio_Write()` 实现；但 BSP 层建议最终提供 Toggle，方便 LED、片选、电平翻转这类调试动作。

这个写法故意不创建 `svc_led -> mod_led -> bsp_gpio`。原因很简单：初期点灯只是确认硬件和主循环，强行分层会增加认知负担。

限制：

- 只能在 `APP_ENABLE_BRINGUP=1` 时编译。
- 不能作为正式状态灯方案。
- 不能在这里加入业务状态判断。

### 3.2 正式产品写法

当 LED 用于状态显示，例如故障闪烁、通信在线、老化运行，就迁移到正式路径：

```text
app_guardian_Poll()
  -> svc_diag_Poll()
      -> mod_indicator_SetPattern()
          -> bsp_gpio_Write()
```

推荐接口：

```c
typedef enum {
    MOD_INDICATOR_PATTERN_OFF = 0,
    MOD_INDICATOR_PATTERN_ON,
    MOD_INDICATOR_PATTERN_SLOW_BLINK,
    MOD_INDICATOR_PATTERN_FAST_BLINK,
    MOD_INDICATOR_PATTERN_FAULT,
} mod_indicator_pattern_t;

mod_status_t mod_indicator_Init(void);
mod_status_t mod_indicator_SetPattern(mod_indicator_pattern_t pattern);
void mod_indicator_Poll(void);
```

Service 决定显示什么，Module 决定 LED 怎么闪，BSP 只负责 GPIO。

## 4. IWDG 定时喂狗

### 4.1 初期写法

目的：先确认 IWDG 初始化和主循环能正常喂狗。

```text
bsp_iwdg_Init(timeout_ms)
app_bringup_Poll()
  -> 每 100ms 调用 bsp_iwdg_Feed()
```

初期直接喂狗也应放在 `APP_ENABLE_BRINGUP` 下的 `app_bringup.c`。正式路径不要让 `app_guardian` 直接调用 `bsp_iwdg_Feed()`，而是迁移到 `svc_watchdog`。

### 4.2 正式写法

正式产品中不建议“定时无脑喂狗”。看门狗应该证明关键任务都活着：

```text
svc_protocol_Poll() -> svc_watchdog_ReportAlive(SVC_WATCHDOG_TASK_COMM)
svc_motion_Poll()   -> svc_watchdog_ReportAlive(SVC_WATCHDOG_TASK_MOTION)
svc_safety_Poll()   -> svc_watchdog_ReportAlive(SVC_WATCHDOG_TASK_SAFETY)
svc_watchdog_Poll() -> 条件满足才调用 bsp_iwdg_Feed()
```

推荐接口：

```c
typedef enum {
    SVC_WATCHDOG_TASK_COMM = 0,
    SVC_WATCHDOG_TASK_MOTION,
    SVC_WATCHDOG_TASK_SAFETY,
    SVC_WATCHDOG_TASK_COUNT,
} svc_watchdog_task_t;

svc_status_t svc_watchdog_Init(void);
void svc_watchdog_ReportAlive(svc_watchdog_task_t task);
void svc_watchdog_Poll(void);
```

`svc_watchdog_Poll()` 逻辑：

```text
在一个喂狗窗口内：
  - 通信任务至少运行过
  - 运动任务未卡死
  - 安全任务未卡死
  - 系统没有进入不可恢复故障
满足才喂 IWDG
```

BSP 接口保持简单：

```c
bsp_status_t bsp_iwdg_Init(uint32_t timeout_ms);
bsp_status_t bsp_iwdg_Feed(void);
```

## 5. 通信测试

### 5.1 初期串口 echo

目的：验证 USART、波特率、引脚、收发方向。

允许短路径：

```text
app_bringup_CommEchoPoll()
  -> bsp_usart_Read()
  -> bsp_usart_Write()
```

示例：

```c
#if APP_ENABLE_BRINGUP

void app_bringup_CommEchoPoll(void)
{
    uint8_t byte = 0;

    while (bsp_usart_ReadByte(BSP_USART_DEBUG, &byte) == BSP_OK) {
        (void)bsp_usart_Write(BSP_USART_DEBUG, &byte, 1U);
    }
}

#endif
```

限制：

- 只用于确认硬件通信。
- 不做协议解析。
- 不进入正式协议服务。

### 5.2 正式通信口

正式路径：

```text
svc_protocol_Poll()
  -> mod_comm_port_Read()
      -> bsp_usart_Read()
  -> svc_protocol_Parse()
  -> svc_protocol_Dispatch()
  -> mod_comm_port_Write()
      -> bsp_usart_Write()
```

`mod_comm_port` 负责 USART/RS485/CAN 物理通道适配；`svc_protocol` 负责 Modbus 或生产协议。

Modbus RTU 注意：

- 3.5 字符帧间隔不能只靠 1ms 任务轮询判断。
- BSP/Module 层应提供接收时间戳、USART IDLE 事件或定时器事件。
- `svc_protocol_Poll()` 只消费“已到达边界的数据”或做增量解析。
- RS485 发送完成必须等 USART `TC`，不能只等 TXE，否则 DE 过早释放会截断最后一个字节。

## 6. 协议解析

协议解析属于 Service，不属于 Application、Module、BSP。

推荐拆法：

```text
svc_protocol.c        协议主流程和命令分发
svc_protocol_frame.c  帧解析状态机
svc_protocol_map.c    寄存器/命令到 Service API 的映射
```

初期可以先合在 `svc_protocol.c`，文件超过约 500 行或命令数量明显增加后再拆。

推荐流程：

```text
mod_comm_port_Read()
  -> ringbuffer
  -> svc_protocol_FeedByte()
  -> frame complete
  -> validate crc
  -> dispatch command
  -> build response
  -> mod_comm_port_Write()
```

协议解析必须可 PC 单元测试：

- 正常帧。
- CRC 错误。
- 长度错误。
- 非法地址。
- 非法命令。
- 半包、多包、粘包。

## 7. 任务调度

初期不引入完整 RTOS。建议使用轻量 cooperative scheduler：

```text
app_MainLoop()
  -> osal_SchedulerPoll()
```

任务表示例：

```c
typedef void (*osal_task_fn_t)(void);

typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t last_run_ms;
    osal_task_fn_t run;
} osal_task_t;
```

调度逻辑：

```c
void osal_SchedulerPoll(void)
{
    uint32_t now_ms = osal_GetTickMs();

    for (uint8_t i = 0; i < task_count; i++) {
        if ((now_ms - tasks[i].last_run_ms) >= tasks[i].period_ms) {
            tasks[i].last_run_ms = now_ms;
            tasks[i].run();
        }
    }
}
```

推荐任务：

| 任务 | 周期 | 说明 |
| --- | --- | --- |
| `svc_protocol_Poll` | 1ms 或每轮 | 增量解析，不阻塞 |
| `svc_motion_Poll` | 1ms | 运动状态机、超时判断 |
| `svc_safety_Poll` | 1ms | 急停、故障保持 |
| `mod_indicator_Poll` | 10ms | 正式状态灯刷新 |
| `svc_param_Poll` | 50ms | 参数保存状态、低频任务 |
| `svc_watchdog_Poll` | 100ms | 条件满足才喂狗 |

不要让单个任务长时间阻塞。EEPROM 写入、日志输出、协议发送都应拆成短步骤。

注意：调度周期不是硬实时保证。脉冲计数、到步停止、光感急停、RS485 TC 等需要更快响应的动作，应放在 BSP/Module 的中断、定时器事件或快速状态路径中完成。

## 8. 分阶段规划

### 阶段 A：上电 Bring-up

- 建立 `APP_ENABLE_BRINGUP`。
- LED 500ms 闪烁，验证时钟和主循环。
- USART echo，验证通信引脚和波特率。
- IWDG 初始化和最简单喂狗。
- 所有代码集中在 `app_bringup.c`，便于之后删除或迁移。

### 阶段 B：BSP 稳定

- `bsp_gpio`、`bsp_timer`、`bsp_usart`、`bsp_iwdg` 接口稳定。
- LED 闪烁仍可留在 bring-up，但正式代码不依赖它。
- USART 从轮询收发升级为中断或 DMA + ringbuffer。

### 阶段 C：正式 Module

- `mod_comm_port` 接管通信口。
- `mod_indicator` 接管状态 LED。
- `mod_stepper_driver` 接管 STEP/DIR/EN。
- `mod_eeprom` 接管 EEPROM。

### 阶段 D：正式 Service

- `svc_protocol` 替代 echo。
- `svc_watchdog` 替代无脑喂狗。
- `svc_motion` / `svc_safety` 替代简单动作测试。
- 建立 Ceedling 单元测试。

### 阶段 E：调度和产线模式

- 引入轻量 OSAL scheduler。
- Normal / Factory / Aging 模式接入调度。
- 工厂模式提供通信测试、LED 测试、传感器测试、电机测试命令。
- 老化模式记录动作统计和故障统计。

## 9. 判断标准

功能是否需要完整分层，可以按下面判断：

| 问题 | 建议 |
| --- | --- |
| 只是验证 GPIO 能不能翻转？ | `app_bringup` 直调 BSP，宏隔离 |
| LED 是正式状态显示吗？ | `svc_diag/status -> mod_indicator -> bsp_gpio` |
| 只是验证 USART 波特率？ | `app_bringup` echo，宏隔离 |
| 通信要承载协议命令吗？ | `svc_protocol -> mod_comm_port -> bsp_usart` |
| 只是确认 IWDG 会复位？ | 初期简单喂狗或故意不喂 |
| 产品可靠性要靠看门狗吗？ | `svc_watchdog` 条件喂狗 |
| 代码需要 PC 单元测试吗？ | 放 Service 或 Module，不直接依赖 BSP |

最终原则：

```text
点亮板子可以短，产品功能必须稳。
Bring-up 允许少量跨层，Release 正式路径必须回到单向依赖。
```
