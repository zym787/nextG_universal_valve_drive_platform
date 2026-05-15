# 设计原则

返回：[工程文档总入口](about.md)

本文档记录本项目长期遵守的工程原则。它不是额外架构层，也不替代各层开发指南。

## 1. 热路径不依赖动态内存

STM32F103C8T6 RAM 较小，通信、运动控制、急停、老化统计等热路径不使用 `malloc`/`free`。

推荐做法：

- 缓冲区、队列、状态机对象在初始化阶段静态分配。
- ringbuffer、event queue、log buffer 使用固定容量。
- 运行时如果容量不足，应返回错误或丢弃低优先级数据，不临时申请内存。

## 2. 中断只做有限交接

中断和回调中只做可界定、可估算耗时的工作：

```text
接收字节/DMA 事件 -> 写入 ringbuffer -> 设置事件标志
```

不在中断中做：

- 协议解析。
- EEPROM 写入。
- 日志格式化长字符串。
- 等待、延时、阻塞锁。
- 大段业务状态机。

但中断或 DMA 回调可以做硬实时边界动作：

- 记录 USART 接收时间戳或 IDLE 事件。
- TIM 比较/更新事件中完成脉冲计数。
- 光感急停触发后尽快关闭 PWM 或置位急停事件。
- USART `TC` 后释放 RS485 DE。

## 3. 电机脉冲使用硬件定时器

42 步进电机 STEP 脉冲使用 TIM/PWM 生成，不使用软件延时或主循环翻转 GPIO。

原因：

- 脉冲频率和占空比更稳定。
- 通信解析、看门狗、老化统计不会干扰脉冲时序。
- 光感急停可以更快停止输出。
- 后续扩展梯形加减速更自然。

目标步数停止和急停快速关断应在 Module/BSP 快速路径完成，Service 只负责目标、策略和故障判断。

## 4. 公共接口不暴露平台类型

Application、Service、Module 的公共接口不出现 STM32 LL/HAL 类型。

不推荐：

```c
GPIO_TypeDef *port;
TIM_TypeDef *timer;
LL_USART_InitTypeDef init;
```

推荐：

```c
uint8_t channel;
uint16_t pin_id;
uint32_t frequency_hz;
```

平台细节留在 BSP 和 CubeMX 生成区，便于 PC 单元测试和后续 MCU 替换。

## 5. 配置文件与设备存储分离

生产配置文件推荐 JSON，设备内部存储推荐 EEPROM 二进制结构。

```text
JSON: 上位机、人、产线工具读取
EEPROM binary: MCU 读取，带 magic/version/length/sequence/crc
```

MCU 不直接解析 JSON/INI，避免占用 Flash/RAM。

## 6. 可靠性放在状态机里

错位重走、急停、超时、故障保持不散落在 GPIO 或电机驱动代码中。

推荐归属：

- `svc_motion`：目标位置、运动状态、距离/速度换算。
- `svc_safety`：急停、超时、错位重走、故障保持。
- `app_guardian`：应用级降级、故障展示、喂狗条件。

## 7. 测试优先覆盖纯软件规则

优先把协议、参数、运动、安全和模式切换写成可在 PC 上测试的 C 代码。

测试优先级：

1. `svc_param`、`svc_protocol`、`svc_motion`、`svc_safety`。
2. `mod_eeprom`、`mod_stepper_driver`，通过 mock BSP 测试。
3. `app_mode`、`app_guardian`，通过 mock Service 测试。
4. BSP 主要依赖代码审查、示波器、HIL 和板级验证。

## 8. 小步演进，不中途换架构

当前规划期固定主架构，后续只做可控演进：

- 串口 RXNE 可升级到 DMA + IDLE。
- 恒速步进可升级到梯形加减速。
- 覆盖率从观察升级为门槛。
- 更大容量 MCU 后再加入 Bootloader + A/B 双分区。
- RTOS/RT-Thread 引入前先稳定 OSAL 接口。
