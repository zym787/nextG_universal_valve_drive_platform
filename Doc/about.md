# 工程文档总入口

本文档是 `Doc` 目录的统一入口。项目级结论、阅读顺序和详细 TODO 以本文为准；各专题文档只保留设计约束、接口建议和局部检查点。

## 1. 当前架构结论

项目固定为 **4 个自有代码层 + CubeMX 生成区**：

```text
Application -> Service -> Module -> BSP -> CubeMX/LL/CMSIS
```

`ThirdParty` 和 `BoardConfig` 是辅助域，不作为业务分层；`Core`、`Drivers`、`cmake/stm32cubemx` 是 CubeMX 生成区。

职责一句话：

| 区域 | 职责 | 前缀 |
| --- | --- | --- |
| Application | 运行模式、任务入口、流程编排 | `app_` |
| Service | 协议、参数、运动、安全等系统能力 | `svc_` |
| Module | EEPROM、电机驱动、传感器、通信口等板上器件 | `mod_` |
| BSP | GPIO、USART、TIM、PWM、ADC、IWDG 等 MCU 外设 | `bsp_` |
| BoardConfig | 硬件版本、通道、引脚、器件配置 | `board_` |
| ThirdParty | 外部组件，保持上游命名 | 不约束 |

依赖规则：

```text
Application 的模式/业务文件只依赖 Service
Application 的组合根 app.c/app_system.c 可负责初始化顺序
Service 只依赖 Module 和纯软件 ThirdParty
Module 只依赖 BSP、BoardConfig 和必要 ThirdParty
BSP 只依赖 CubeMX/LL/CMSIS、BoardConfig 和必要 ThirdParty
```

## 2. 文档地图

推荐按下面顺序阅读：

1. [分层架构决策记录](layered_architecture_decision.md)：为什么固定为 Application/Service/Module/BSP。
2. [软件架构工程文档](software_architecture_valve_drive_platform.md)：系统目标、关键约束、总体架构。
3. [CMake、CI/CD 与测试方案](architecture_cmake_ci_evolution_plan.md)：如何组织构建、测试、覆盖率和发布。
4. [Bring-up 示例与任务规划](bringup_examples_plan.md)：LED、IWDG、通信测试、协议解析和调度如何落地。
5. [Application 开发指南](application_development_guide.md)：模式编排、主循环入口、应用边界。
6. [Service 开发指南](service_development_guide.md)：协议、参数、运动、安全服务。
7. [Module 开发指南](module_development_guide.md)：EEPROM、电机、传感器、通信口适配。
8. [BSP 开发指南](bsp_development_guide.md)：MCU 外设封装、`bsp_status_t`、初始化入口。
9. [BoardConfig 开发指南](boardconfig_development_guide.md)：多板卡差异与 CubeMX 工程关系。
10. [ThirdParty 管理指南](thirdparty_management_guide.md)：第三方组件来源、命名、vendor/submodule 策略。
11. [软件版本与发布管理指南](software_version_release_guide.md)：Git tag、版本头文件、Release 产物。
12. [设计原则](concept.md)：实时性、内存、ISR、接口边界等项目原则。

## 3. 目录基线

```text
Application/
  Inc/
  Src/
  CMakeLists.txt

Service/
  Inc/
  Src/
  CMakeLists.txt

Module/
  Inc/
  Src/
  CMakeLists.txt

BSP/
  Inc/
  Src/
  bsp.h
  bsp.c
  CMakeLists.txt

BoardConfig/
  CMakeLists.txt
  hw_rev_a/

ThirdParty/
  CMakeLists.txt
  ringbuffer/
  event/
  modbus/
  osal/
  log/
```

## 4. 可执行步骤与详细 TODO

### Phase 0: 工程基线

- [ ] 确认 CubeMX 生成的 LL + CMake 工程 Debug/Release 均可编译。
- [ ] 保持 `Core/`、`Drivers/`、`cmake/stm32cubemx/` 为 CubeMX 生成区，不放业务逻辑。
- [ ] 建立 `Application`、`Service`、`Module`、`BSP`、`BoardConfig`、`ThirdParty` 的 `CMakeLists.txt`。
- [ ] 在 `main.c` 的 USER CODE 区域只保留 `app_Init()` 和 `app_MainLoop()` 调用。
- [ ] 明确 `app.c/app_system.c` 是组合根，允许编排 BSP、Module、Service 初始化；模式和业务文件仍只调用 Service。
- [ ] 建立 `CMakePresets.json`，至少支持 `Debug`、`Release`、`Release-hw-rev-a`。
- [ ] 建立 `APP_ENABLE_BRINGUP`，用于隔离 LED 闪烁、USART echo、IWDG 基础验证等早期短路径代码。

### Phase 1: BoardConfig

- [ ] 建立 `BoardConfig/hw_rev_a/board_config.h` 和 `board_config.c`。
- [ ] 固定 `BOARD_HW` 编译选项，并生成 `selected_board_config` target。
- [ ] 描述通道数、半通道、光感数量/极性、STEP/DIR/EN 映射、EEPROM 参数、RS485 控制脚。
- [ ] BoardConfig 初期只使用 `static const` 配置表，不做运行时动态配置系统，不占用不必要 RAM。
- [ ] 约定：轻量 IO 差异用 BoardConfig；MCU、时钟树、外设实例明显变化时再拆独立 CubeMX 工程。

### Phase 2: BSP

- [ ] 建立 `bsp_status.h`，统一 BSP 层返回值。
- [ ] 建立 `bsp.h` / `bsp.c`，提供 `bsp_Init()` 和必要 `bsp_Poll()`，不做万能头文件。
- [ ] 实现 `bsp_gpio`、`bsp_timer`、`bsp_dwt`。
- [ ] 实现 `bsp_usart`，优先中断或 DMA + ringbuffer，不在中断中解析协议。
- [ ] Modbus RTU 帧边界不要只靠 1ms Poll，优先使用 USART IDLE、定时器或接收时间戳辅助判断。
- [ ] 实现 `bsp_timer` / `bsp_pwm`，用于稳定生成 STEP 脉冲。
- [ ] 实现 `bsp_soft_i2c` 或后续 `bsp_i2c`，只表达 I2C 总线能力。
- [ ] 实现 `bsp_iwdg`，`bsp_wwdg` 暂缓。
- [ ] 为需要诊断的外设提供 `bsp_xxx_GetLastError()`，但普通流程只判断 `bsp_status_t`。
- [ ] 用 `app_bringup.c` 验证 LED 闪烁、USART echo 和 IWDG 基础喂狗，验证完成后迁移或关闭。

### Phase 3: Module

- [ ] 建立 `mod_status.h`，不要向 Service 透传 `bsp_status_t`。
- [ ] 实现 `mod_eeprom`，通过 BSP I2C 能力操作 4KB EEPROM。
- [ ] 实现 `mod_stepper_driver`，通过 BSP TIM/PWM/GPIO 操作 STEP/DIR/EN，目标步数停止和急停要在 Module/BSP 快速路径完成。
- [ ] 实现 `mod_sensor_input`，适配光感/限位/位置反馈。
- [ ] 实现 `mod_comm_port`，适配 USART/RS485，后续预留 CAN；RS485 方向控制必须等待发送完成 `TC` 后再释放 DE。
- [ ] 确认 Module 不出现协议解析、参数结构、阀门状态机和安全策略。

### Phase 4: Service

- [ ] 建立 `svc_status.h` 和必要 `svc_fault_t`。
- [ ] 实现 `svc_system_Init()` / `svc_system_Poll()`，只聚合 Service 内部初始化和轮询，不再负责 BSP/Module 初始化。
- [ ] 实现 `svc_param`：默认值、范围校验、EEPROM 双副本、CRC、版本迁移。
- [ ] 实现 `svc_protocol`：初期优先 Modbus RTU slave，支持写参、读状态、动作测试、读版本。
- [ ] 实现 `svc_motion`：两位阀/多位阀统一模型、距离/速度换算、固定速度运动。
- [ ] 实现 `svc_safety`：急停、超时、错位重走、故障保持。
- [ ] 实现 `svc_watchdog`，由关键任务上报 alive，满足条件才喂 IWDG。
- [ ] 后续复杂后再拆 `svc_valve`、`svc_position`、`svc_upgrade`。

### Phase 5: Application

- [ ] 建立 `app.h`、`app.c`，提供 `app_Init()` 和 `app_MainLoop()`。
- [ ] 建立 `app_mode`，统一 Normal/Factory/Aging 模式切换。
- [ ] 实现 Normal Mode：正常协议命令、故障读取、动作控制。
- [ ] 实现 Factory Mode：写参、保存 EEPROM、校准、单通道测试、回读校验。
- [ ] 实现 Aging Mode：自动循环动作、统计成功/失败/重走/错位/最大动作时间。
- [ ] 建立 `app_guardian`，处理应用级故障保持、看门狗喂狗条件和降级策略。
- [ ] 确认 Application 不包含 `mod_xxx.h`、`bsp_xxx.h`、`stm32f1xx_ll_xxx.h`。

### Phase 6: ThirdParty

- [ ] 第一批只集成 ringbuffer、最小 event、最小 Modbus RTU slave。
- [ ] 第二批再集成轻量 log，Release 下必须可裁剪。
- [ ] 第三批再集成轻量 OSAL，只提供 tick、timeout、poll scheduler、event post/fetch。
- [ ] 每个第三方组件记录 `LICENSE`、`SOURCE.txt`、版本或提交号。
- [ ] 建立最小 cooperative scheduler，先调度协议、运动、安全、状态灯、看门狗等短任务。

### Phase 7: 测试与覆盖率

- [ ] 先用 Unity + 手写 fake 跑通第一个测试，再引入 Ceedling 统一管理 Unity/CMock/gcov。
- [ ] 建立 `test/support`，提供 fake BSP、fake Module、公共测试数据。
- [ ] 优先测试 ringbuffer、event、`svc_param`、`svc_protocol`、`svc_motion`、`svc_safety`。
- [ ] Module 测试 mock BSP；Service 测试 mock Module；Application 测试 mock Service。
- [ ] CI 生成覆盖率报告，初期只观察，稳定后给 Service 设置最低覆盖率门槛。
- [ ] 后续加入 HIL，验证真实 EEPROM、STEP 脉冲、光感急停和协议回归。

### Phase 8: CI/CD 与版本发布

- [ ] GitHub Actions 支持 `push` / `pull_request` 自动构建 Debug/Release。
- [ ] CI 运行 clang-format 检查、flawfinder/cppcheck/CodeQL、Ceedling 单元测试、覆盖率。
- [ ] 软件版本以 Git tag `vMAJOR.MINOR.PATCH` 为唯一发布版本来源。
- [ ] CMake 从 Git tag、commit、`BOARD_HW`、构建时间生成 `app_version.h`。
- [ ] 固件内提供 `app_version_GetInfo()`，协议可读取版本、commit、硬件版本、协议版本。
- [ ] tag 触发 Release，自动上传 `.elf`、`.hex`、`.bin`、`.map`、`firmware-info`、`sha256sums`。

### Phase 9: 持续演进

- [ ] USART RXNE 接收可演进到 DMA + IDLE。
- [ ] 步进电机恒速运动可演进到梯形加减速。
- [ ] 更大容量 MCU 后再评估 Bootloader + A/B 双分区。
- [ ] 后续加入 CAN 时新增 `bsp_can`、`mod_can_port`、`svc_protocol_can` 或协议适配器。
- [ ] 后续引入 FreeRTOS/RT-Thread 时，优先通过 OSAL 迁移，不改业务核心接口。
- [ ] 国产 MCU 替代时新增 Vendor/BSP 适配，保持 Service 和大部分 Module 不变。

## 5. 冲突检查规则

- README 只保留项目摘要和重大节点；详细 TODO 放在本文。
- CubeMX/LL/CMSIS 只是生成区底座，不作为项目自有代码层。
- `Application` 不直接调用 Module/BSP。
- 例外：`app.c/app_system.c` 作为组合根可编排初始化；`app_bringup.c` 可在 `APP_ENABLE_BRINGUP` 下做板级验证。
- `Service` 不直接调用 BSP/LL/HAL。
- `Module` 不直接调用 LL/HAL。
- EEPROM、电机驱动、光感等板上器件属于 Module，不属于 BSP。
- BSP 只封装 MCU 外设，不出现阀门、EEPROM、STEP/DIR/EN、协议等业务或器件语义。
- ThirdParty 原始文件和函数保持上游命名，不套用项目命名规则。
- Bring-up 短路径必须使用 `APP_ENABLE_BRINGUP` 隔离，Release 正式路径仍遵守单向依赖。
