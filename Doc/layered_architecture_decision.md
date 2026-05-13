# 分层架构决策记录

返回：[工程文档总入口](about.md)

本文档记录当前架构选择的结论和边界。项目仍处规划期，允许重构；但主分层一旦开始落地，应避免中途大改。

## 1. 最终结论

固定为 **4 个自有代码层 + CubeMX 生成区**：

```text
Application -> Service -> Module -> BSP -> CubeMX/LL/CMSIS
```

辅助域：

```text
BoardConfig: 多硬件版本静态配置
ThirdParty: 外部组件源码或适配
```

## 2. 为什么保留 Service 层

如果只有 Application、Module、BSP，`Module` 很容易同时塞入：

- EEPROM、电机驱动、传感器等板上器件。
- 协议、参数、运动、安全等业务规则。
- 后续 CAN、RTOS、升级、诊断等系统能力。

这会让硬件变化、协议变化和业务变化互相影响。保留 Service 后边界更清楚：

```text
Application: 当前模式要做什么
Service: 系统规则怎么执行
Module: 板上器件怎么工作
BSP: MCU 外设怎么操作
```

## 3. 各层边界

### Application

职责：

- Normal / Factory / Aging 模式编排。
- 主循环入口。
- 应用级守护、故障保持和降级策略。

不负责：

- 不直接调用 Module/BSP。
- 不解析底层协议帧。
- 不操作 EEPROM、电机、GPIO、USART。

### Service

职责：

- `svc_protocol`：协议解析、命令分发、响应封包。
- `svc_param`：参数、默认值、EEPROM 双副本、CRC、版本迁移。
- `svc_motion`：阀门模型、距离/速度/步数换算、运动状态机。
- `svc_safety`：急停、超时、错位重走、故障保持。
- `svc_system`：初始化和轮询聚合。

不负责：

- 不直接调用 BSP/LL/HAL。
- 不关心具体 GPIO、TIM、USART 实例。
- 不关心 EEPROM 是 I2C 还是 SPI。

### Module

职责：

- `mod_eeprom`：外部 EEPROM 器件驱动。
- `mod_stepper_driver`：STEP/DIR/EN 步进驱动器适配。
- `mod_sensor_input`：光感、限位、反馈输入适配。
- `mod_comm_port`：USART/RS485/CAN 通信口适配。

不负责：

- 不写协议解析。
- 不写参数结构。
- 不写阀门状态机和错位重走策略。
- 不调用 LL/HAL。

### BSP

职责：

- GPIO、USART、CAN、TIM、PWM、ADC、CRC、IWDG、DWT、I2C/SPI 等 MCU 外设。
- 使用 CubeMX 初始化结果和 LL/CMSIS。
- 对上提供稳定、少平台类型泄漏的接口。

不负责：

- 不出现 EEPROM 设备语义。
- 不出现 STEP/DIR/EN 业务语义。
- 不出现阀门、协议、参数、安全策略。

### BoardConfig

职责：

- 硬件版本。
- 通道数、半通道能力。
- 引脚逻辑 ID。
- 电机驱动芯片类型。
- 传感器数量、类型、极性。
- EEPROM 地址、页大小、容量。

结论：轻量板卡差异集中到 BoardConfig；MCU、时钟树、外设实例明显变化时再拆独立 CubeMX 工程。

### ThirdParty

职责：

- ringbuffer。
- event/message。
- Modbus。
- OSAL。
- log。

第三方源码保持原始命名，不套用 `app_`、`svc_`、`mod_`、`bsp_` 规则。

## 4. 被否定方案

| 方案 | 结论 |
| --- | --- |
| Module 和 BSP 合成一个大 BSP | 不采用，EEPROM/电机/传感器会和 MCU 外设混在一起 |
| 不设 Service | 不采用，协议/参数/运动/安全会塞进 Module |
| 每块轻微差异 PCB 都建 CubeMX 工程 | 不采用，初期维护成本高 |
| MCU 端解析 JSON/INI | 不采用，Flash/RAM 不划算 |
| 软件延时生成 STEP 脉冲 | 不采用，实时性和急停响应不可控 |

## 5. 命名结论

使用小写前缀 + 下划线：

| 区域 | 前缀 |
| --- | --- |
| Application | `app_` |
| Service | `svc_` |
| Module | `mod_` |
| BSP | `bsp_` |

示例：

```text
app_mode.c
svc_motion.c
mod_eeprom.c
bsp_usart.c
```

`Server` 不作为层级名，避免和 Modbus server、CANopen server 等协议角色混淆。

## 6. 演进判断

保持当前架构，直到出现明确触发条件。

可以小步演进：

- RXNE -> DMA + IDLE。
- 恒速步进 -> 梯形加减速。
- vendor Modbus -> submodule。
- Ceedling 覆盖率观察 -> 覆盖率门槛。

需要重新评估架构：

- MCU/SDK 体系完全变化。
- 引入 RTOS 后任务边界和 OSAL 不再满足需求。
- 支持在线升级需要分区、镜像校验、回滚。
- 多协议复杂到需要插件式协议适配。

## 7. 迁移步骤

当前仍处规划期，建议直接按目标结构创建：

1. 建立 `BoardConfig`、`ThirdParty`、`BSP`、`Module`、`Service`、`Application` 的 CMake target。
2. `main.c` 只调用 `app_Init()` 和 `app_MainLoop()`。
3. BSP 先完成 GPIO、USART、TIM/PWM、I2C、IWDG、DWT。
4. Module 先完成 EEPROM、步进驱动、传感器、通信口。
5. Service 先完成参数、协议、运动、安全。
6. Application 先完成 Normal、Factory、Aging 模式入口。
7. Ceedling 优先覆盖 Service，再覆盖 Module 和 Application。

最终边界一句话：

```text
BSP 提供 MCU 会什么；Module 定义板子有什么；Service 定义系统能力怎么工作；Application 决定当前模式做什么。
```
