# STM32 CMake 项目分层架构与工程化演进方案

本文档用于指导 `nextG_universal_valve_drive_platform` 从 CubeMX 生成的 STM32F103C8T6 CMake 工程，逐步演进为结构清晰、便于初学者理解、便于集成第三方组件、便于持续维护和自动化验证的嵌入式项目。

当前项目特点：

- 目标芯片：STM32F103C8T6 / STM32F103xB
- 工程来源：STM32CubeMX 生成的 CMake 工程
- 外设初始化：主要使用 STM32 LL 库
- 板载存储：4KB EEPROM，建议用于保存设备参数、工厂配置和少量运行统计
- 当前已有目录：`Core`、`Drivers`、`cmake/stm32cubemx`、`BSP`、`Module`、`Application`、`Doc`
- 当前已有 CI：GitHub Actions 编译 workflow、flawfinder 安全扫描 workflow

## 1. 总体目标

本项目的长期工程目标不是只让代码“能编译”，而是让项目具备以下能力：

1. 初学者能快速理解：从 `main.c` 到应用逻辑，再到模块和底层外设，依赖路径清晰。
2. CubeMX 可继续使用：CubeMX 重新生成代码时，不破坏业务代码。
3. 业务代码可维护：业务逻辑不散落在 `Core/Src/main.c` 或中断文件中。
4. 模块易复用：协议、电机、存储、调度、环形缓冲等组件可以独立编译、独立测试。
5. CI/CD 可持续运行：每次提交自动编译、静态检查、测试，并产出固件文件。
6. 测试和覆盖率可演进：先从纯 C 模块单元测试开始，再逐步扩展到接口模拟、硬件在环测试。
7. 自动化工具能接入：格式化、静态分析、依赖检查、文档生成、版本发布都能逐步自动化。

## 1.1 阶段性架构基线

为避免项目在规划期反复切换方向，以下内容作为阶段性基线固定：

```text
Application -> Module -> BSP -> CubeMX/LL/CMSIS
CMake 构建 STM32 固件
Ceedling 管理 PC 单元测试
EEPROM 保存设备参数
TIM 定时器生成步进电机 STEP 脉冲
ThirdParty 原始源码保持上游命名
```

不建议初期切换：

- 不引入完整 RTOS。
- 不把业务逻辑写入 CubeMX 生成区。
- 不让 Module 直接调用 LL/HAL。
- 不让 MCU 端解析 JSON/INI。
- 不使用软件延时模拟步进电机脉冲。
- 不优先使用片内 Flash 保存生产参数。

## 2. 推荐分层

推荐保留 README 中已经设计的 4 层结构，同时把第三方组件作为独立横向能力层。

```text
Application
    |
    v
Module
    |
    v
BSP
    |
    v
CubeMX / HAL / LL / CMSIS

ThirdParty 作为独立组件层，被 BSP、Module 或 Application 按需依赖
```

### 2.1 CubeMX / HAL / LL / CMSIS 层

对应目录：

```text
Core/
Drivers/
cmake/stm32cubemx/
startup_stm32f103xb.s
STM32F103XX_FLASH.ld
```

职责：

- 保存 CubeMX 生成的外设初始化代码。
- 保存 STM32 LL/HAL/CMSIS 驱动。
- 提供 `MX_GPIO_Init()`、`MX_USARTx_UART_Init()`、`MX_TIMx_Init()` 等初始化入口。
- 提供芯片启动文件、链接脚本、系统时钟配置。

设计原则：

- 尽量不在该层写业务逻辑。
- `Core/Src/main.c` 只作为系统启动入口。
- 用户代码只写在 CubeMX 的 `USER CODE BEGIN/END` 区域。
- 不建议手动频繁修改 `cmake/stm32cubemx/CMakeLists.txt`，因为它属于 CubeMX 生成链路的一部分。

### 2.2 BSP 层

对应目录：

```text
BSP/
  Inc/
  Src/
  CMakeLists.txt
```

职责：

- 封装 MCU 片上外设。
- 允许直接调用 STM32 LL API。
- 允许使用 CubeMX 生成的外设初始化结果。
- 向 Module 层提供稳定、简洁、板级相关的接口。

适合放在 BSP 的内容：

- `bsp_usart`
- `bsp_adc`
- `bsp_gpio`
- `bsp_pwm`
- `bsp_dwt`
- `bsp_timer`
- 软件 I2C，如果它依赖具体 GPIO 和时序，也可以放在 BSP。

不适合放在 BSP 的内容：

- 电机控制策略。
- 阀门开关业务流程。
- 通信协议解析。
- 存储数据结构。
- 业务状态机。
- PCB 上的某个功能模块抽象，如果它已经带有业务含义，应放在 Module。

### 2.3 Module 层

对应目录：

```text
Module/
  Inc/
  Src/
  CMakeLists.txt
```

职责：

- 封装独立业务能力。
- 不直接调用 LL/HAL。
- 通过 BSP 访问硬件。
- 可以依赖 ThirdParty 中的纯软件组件。

适合放在 Module 的内容：

- `mod_protocol`
- `mod_motor`
- `mod_storage`
- `mod_pulse`
- `mod_dlci`
- `mod_opto`
- `mod_scheduler` 或 `mod_osal`

设计原则：

- Module 的头文件不要暴露 STM32 寄存器、`GPIO_TypeDef`、`TIM_TypeDef` 等硬件细节。
- Module 的接口应尽量使用普通 C 类型，例如 `uint8_t`、`uint16_t`、枚举、结构体。
- 如果确实需要硬件能力，应通过 BSP 的抽象接口传入。

### 2.4 Application 层

对应目录：

```text
Application/
  Inc/
  Src/
  CMakeLists.txt
```

职责：

- 组织业务流程。
- 管理系统初始化顺序。
- 管理主循环任务。
- 组合多个 Module 完成完整功能。

适合放在 Application 的内容：

- `app`
- `app_control`
- `app_movement`
- `app_guardian`
- `app_agreement`

设计原则：

- Application 不直接调用 BSP。
- Application 不直接调用 STM32 LL/HAL。
- Application 只通过 Module 完成业务功能。
- `Core/Src/main.c` 中只调用 `app_Init()` 和 `app_MainLoop()`。

### 2.5 ThirdParty 层

推荐新增目录：

```text
ThirdParty/
  ringbuffer/
  tinyprintf/
  event/
  modbus/
  osal/
  log/
  CMakeLists.txt
```

职责：

- 管理第三方纯软件组件。
- 方便单独升级、替换和审查许可证。
- 避免第三方代码散落在 BSP 或 Module 内。

设计原则：

- 每个第三方组件独立一个子目录。
- 每个组件保留 `LICENSE` 或来源说明。
- 尽量不要直接修改第三方源码；如果必须修改，记录 patch 原因。
- 使用 CMake target 暴露依赖，而不是全局 include。

## 3. 推荐目录结构

建议逐步整理为：

```text
nextG_universal_valve_drive_platform/
  Application/
    Inc/
      app.h
      app_control.h
      app_movement.h
      app_guardian.h
    Src/
      app.c
      app_control.c
      app_movement.c
      app_guardian.c
    CMakeLists.txt

  Module/
    Inc/
      mod_protocol.h
      mod_motor.h
      mod_storage.h
      mod_pulse.h
    Src/
      mod_protocol.c
      mod_motor.c
      mod_storage.c
      mod_pulse.c
    CMakeLists.txt

  BSP/
    Inc/
      bsp.h
      bsp_usart.h
      bsp_adc.h
      bsp_pwm.h
      bsp_dwt.h
    Src/
      bsp.c
      bsp_usart.c
      bsp_adc.c
      bsp_pwm.c
      bsp_dwt.c
    CMakeLists.txt

  ThirdParty/
    ringbuffer/
    tinyprintf/
    event/
    modbus/
    osal/
    log/
    CMakeLists.txt

  Core/
  Drivers/
  cmake/
  Doc/
```

## 4. CMake 设计原则

### 4.1 为什么每层一个 CMakeLists.txt

推荐在 `BSP`、`Module`、`Application`、`ThirdParty` 各放一个 `CMakeLists.txt`。

原因：

1. 初学者容易理解：一个目录对应一个构建单元。
2. 依赖关系直观：`application` 依赖 `module`，`module` 依赖 `bsp`，`bsp` 依赖 `stm32cubemx`。
3. 便于后续测试：可以单独把 `module` 编译到 PC 单元测试工程中。
4. 便于集成第三方组件：每个组件都可以以 CMake target 的形式接入。
5. 避免顶层 CMakeLists 过长：顶层只负责组织，不负责列出所有源文件。

不建议一开始每个小模块都建立独立 `CMakeLists.txt`，因为这会增加初学者理解成本。等某个模块变大、需要独立复用或独立测试时，再拆成更细的子 target。

### 4.2 推荐使用 target 管理依赖

不推荐使用全局的 `include_directories()`、`add_definitions()`。

推荐使用：

```cmake
target_sources()
target_include_directories()
target_compile_definitions()
target_link_libraries()
```

原因：

- 依赖范围清晰。
- 哪个模块需要哪个 include，一眼可见。
- 后续单元测试更容易复用某个 target。
- 避免一个模块错误地依赖另一个模块的私有头文件。

### 4.3 OBJECT library 与 STATIC library 的选择

对于当前裸机固件工程，推荐先使用 `OBJECT` library：

```cmake
add_library(bsp OBJECT)
```

原因：

- 适合嵌入式固件最终链接成一个 ELF 的场景。
- 不容易遇到静态库链接顺序问题。
- 初学者更容易理解：这些源文件最终会进入固件。

如果后续需要发布模块库、跨项目复用，或需要更标准的库边界，可以逐步改成：

```cmake
add_library(bsp STATIC)
```

## 5. CMake 落地步骤

### 阶段 1：修正和建立基础 CMake 结构

执行步骤：

1. 确认 `BSP/CmakeLists.txt` 文件名是否为标准 `BSP/CMakeLists.txt`。
2. 在 `Module` 下新增 `CMakeLists.txt`。
3. 在 `Application` 下新增 `CMakeLists.txt`。
4. 后续引入第三方组件时，新增 `ThirdParty/CMakeLists.txt`。
5. 在顶层 `CMakeLists.txt` 中增加：

```cmake
add_subdirectory(BSP)
add_subdirectory(Module)
add_subdirectory(Application)
```

6. 顶层最终只链接 `application`：

```cmake
target_link_libraries(${CMAKE_PROJECT_NAME}
    application
)
```

7. 由各层 CMakeLists 自己描述依赖链。

### 阶段 2：BSP CMakeLists.txt 模板

```cmake
add_library(bsp OBJECT)

target_sources(bsp PRIVATE
    Src/bsp.c
    Src/bsp_usart.c
    Src/bsp_adc.c
    Src/bsp_pwm.c
    Src/bsp_dwt.c
)

target_include_directories(bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(bsp PUBLIC
    stm32cubemx
)
```

说明：

- `bsp` 依赖 `stm32cubemx`，因为 BSP 需要使用 `main.h`、`gpio.h`、`usart.h`、LL 驱动头文件。
- `BSP/Inc` 使用 `PUBLIC`，因为 Module 需要包含 BSP 的公共头文件。
- BSP 的私有头文件可以放到 `BSP/Src` 或 `BSP/PrivateInc`，不要暴露给上层。

### 阶段 3：Module CMakeLists.txt 模板

```cmake
add_library(module OBJECT)

target_sources(module PRIVATE
    Src/mod_protocol.c
    Src/mod_motor.c
    Src/mod_storage.c
    Src/mod_pulse.c
)

target_include_directories(module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(module PUBLIC
    bsp
)
```

说明：

- `module` 依赖 `bsp`。
- Module 不能直接依赖 `stm32cubemx`。
- 如果 Module 需要 ringbuffer、tinyprintf、Modbus、event 等组件，应依赖对应 ThirdParty target。

示例：

```cmake
target_link_libraries(module PUBLIC
    bsp
    ringbuffer
    tinyprintf
)
```

### 阶段 4：Application CMakeLists.txt 模板

```cmake
add_library(application OBJECT)

target_sources(application PRIVATE
    Src/app.c
    Src/app_control.c
    Src/app_movement.c
    Src/app_guardian.c
)

target_include_directories(application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(application PUBLIC
    module
)
```

说明：

- Application 只依赖 Module。
- Application 不直接链接 BSP。
- Application 不直接链接 `stm32cubemx`。

### 阶段 5：ThirdParty CMakeLists.txt 模板

```cmake
add_library(ringbuffer STATIC
    ringbuffer/ringbuffer.c
)

target_include_directories(ringbuffer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ringbuffer
)

add_library(tinyprintf STATIC
    tinyprintf/tinyprintf.c
)

target_include_directories(tinyprintf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyprintf
)
```

说明：

- 第三方组件建议一个组件一个 target。
- 不要把所有第三方源码堆到一个 `third_party` target 中。
- 这样后续可以单独升级、替换、测试某个组件。

## 6. main.c 落地方式

`Core/Src/main.c` 是 CubeMX 管理的入口文件，不建议承载业务逻辑。

推荐只在 USER CODE 区域加入：

```c
/* USER CODE BEGIN Includes */
#include "app.h"
/* USER CODE END Includes */
```

外设初始化完成后：

```c
/* USER CODE BEGIN 2 */
app_Init();
/* USER CODE END 2 */
```

主循环中：

```c
/* USER CODE BEGIN 3 */
app_MainLoop();
/* USER CODE END 3 */
```

这样做的原因：

1. CubeMX 重新生成时，USER CODE 区域可保留。
2. `main.c` 保持简洁。
3. 应用入口固定，初学者只需要从 `app_Init()` 和 `app_MainLoop()` 开始阅读。
4. 业务逻辑可以完全离开 `Core` 目录。

## 7. 代码解耦规则

### 7.1 依赖方向规则

必须遵守：

```text
Application -> Module -> BSP -> CubeMX/LL
```

禁止：

```text
BSP -> Module
BSP -> Application
Module -> Application
Application -> BSP
Module -> LL/HAL
```

### 7.2 头文件暴露规则

公共头文件：

- 放在各层 `Inc` 目录。
- 只暴露上层需要调用的 API。
- 尽量不要暴露内部状态变量。

私有头文件：

- 放在 `Src` 或 `PrivateInc`。
- 不通过 `target_include_directories(... PUBLIC ...)` 暴露。

### 7.3 硬件类型隔离规则

Module 和 Application 的公共接口中尽量不要出现：

```c
GPIO_TypeDef *
USART_TypeDef *
TIM_TypeDef *
DMA_Channel_TypeDef *
```

推荐使用普通 C 类型：

```c
typedef enum {
    MOD_MOTOR_DIR_FORWARD,
    MOD_MOTOR_DIR_REVERSE,
} mod_motor_dir_t;

int mod_motor_SetSpeed(uint16_t speed);
int mod_motor_SetDirection(mod_motor_dir_t dir);
```

### 7.4 回调和事件解耦

当 BSP 需要通知 Module 数据到达时，不建议 BSP 直接调用具体 Module 函数。

推荐方式：

```c
typedef void (*bsp_usart_rx_callback_t)(const uint8_t *data, uint16_t len);

int bsp_usart_RegisterRxCallback(bsp_usart_rx_callback_t callback);
```

Module 初始化时注册回调：

```c
bsp_usart_RegisterRxCallback(mod_protocol_OnRxData);
```

这样 BSP 不知道 Module 的存在，只知道一个回调函数。

### 7.5 错误码统一

建议逐步建立统一错误码，例如：

```c
typedef enum {
    APP_OK = 0,
    APP_ERR_PARAM = -1,
    APP_ERR_TIMEOUT = -2,
    APP_ERR_BUSY = -3,
    APP_ERR_HW = -4,
} app_status_t;
```

也可以分层定义：

- `bsp_status_t`
- `mod_status_t`
- `app_status_t`

初期为了简单，可以先统一使用 `int` 返回值，并在头文件注释中说明含义。

### 7.6 命名规则

本项目自有代码仅约束 `Application`、`Module`、`BSP` 三部分，不修改 CubeMX 生成的 `Core`、`Drivers`、`cmake/stm32cubemx` 中的 HAL/LL/CubeMX 命名，也不修改 `ThirdParty` 内第三方源码的原始命名。

统一采用 `xxx_Xxxx` 风格，不采用纯 `XxxXxxx` 风格。

选择原因：

1. C 语言没有命名空间，必须依靠前缀表达层级和模块归属。
2. `app_`、`mod_`、`bsp_` 前缀能直接体现依赖边界。
3. 动作部分使用大驼峰，和 STM32 `HAL_GPIO_WritePin()`、`LL_GPIO_ResetOutputPin()` 风格接近，便于初学者阅读。
4. 纯 `XxxXxxx` 容易和类型名、CubeMX 生成函数、第三方 API 混淆。
5. ThirdParty 原始源码保持上游命名；若需要项目自有适配层，应放到 Module 或 BSP 中，并遵守 `mod_` / `bsp_` 规范。

公共函数命名：

```text
app_Xxxx
app_domain_Xxxx
mod_xxx_Xxxx
bsp_xxx_Xxxx
```

示例：

```c
void app_Init(void);
void app_MainLoop(void);
void app_control_Poll(void);

int mod_protocol_Poll(void);
int mod_valve_SetTargetPosition(uint8_t position);
int mod_valve_GetCurrentPosition(uint8_t *position);

int bsp_usart_Send(const uint8_t *data, uint16_t len);
int bsp_usart_RegisterRxCallback(bsp_usart_rx_callback_t callback);
```

文件命名：

```text
app_control.c / app_control.h
mod_valve.c / mod_valve.h
bsp_usart.c / bsp_usart.h
```

类型命名：

```c
typedef struct {
    uint8_t target_position;
    uint8_t current_position;
} mod_valve_context_t;
```

枚举值和宏命名：

```c
typedef enum {
    MOD_VALVE_STATE_IDLE,
    MOD_VALVE_STATE_MOVING,
    MOD_VALVE_STATE_FAULT,
} mod_valve_state_t;

#define MOD_PROTOCOL_MAX_FRAME_SIZE 64U
```

静态函数命名：

```c
static int parse_frame_header(const uint8_t *data, uint16_t len);
```

ThirdParty 命名：

- 原始第三方源码不强制改名。
- ThirdParty 原始源码保持上游命名；项目自有适配层放到 `Module` 或 `BSP`，使用 `mod_` 或 `bsp_` 前缀。
- CMake target 使用组件名，例如 `ringbuffer`、`tinyprintf`、`modbus`。

## 8. CI/CD 方案

当前已有：

- `.github/workflows/build.yml`
- `.github/workflows/flawfinder.yml`

建议逐步扩展为以下流水线。

### 8.1 Build CI

目标：

- 每次 push / pull request 自动编译固件。
- 生成 `.elf`、`.hex`、`.bin`。
- 上传构建产物。

当前项目已经基本具备该能力。

建议优化方向：

1. 使用 CMake Presets，减少 CI 与本地构建命令差异。
2. 增加 Debug 和 Release 两种构建。
3. 增加固件大小输出。
4. 固件大小超过阈值时给出警告。

推荐命令：

```bash
cmake --preset Release
cmake --build --preset Release
```

### 8.2 Static Analysis CI

目标：

- 每次提交自动检查潜在安全风险和代码质量问题。

当前已有 flawfinder。

建议继续增加：

- `cppcheck`
- `clang-tidy`，若工具链和 compile_commands 支持稳定
- CodeQL，当前 README 中已有 badge，可继续保留

建议扫描范围：

```text
Application/
Module/
BSP/
Core/
```

第三方组件可以视情况排除，避免外部代码噪声过大。

### 8.3 Format Check CI

目标：

- 保证代码风格稳定。
- 避免无意义格式差异。

当前项目已有 `.clang-format`。

建议增加格式检查 workflow：

```bash
find Application Module BSP Core -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

Windows PowerShell 本地可使用：

```powershell
Get-ChildItem Application,Module,BSP,Core -Recurse -Include *.c,*.h |
    ForEach-Object { clang-format --dry-run --Werror $_.FullName }
```

### 8.4 Unit Test CI

目标：

- 在 PC 上测试纯软件逻辑。
- 不依赖 STM32 硬件即可验证协议解析、环形缓冲、状态机、数据结构。

推荐使用 Ceedling 统一管理 PC 单元测试。Ceedling 集成 Unity、CMock 和 gcov，适合嵌入式 C 项目，比手动维护 Unity + CTest 更适合初学者起步。

推荐优先测试：

1. `mod_protocol`
2. `mod_storage` 中的纯数据处理
3. ringbuffer
4. 状态机逻辑
5. 参数校验和错误路径

建议工具：

- Ceedling：测试构建和任务入口。
- Unity：断言框架，由 Ceedling 集成。
- CMock：自动生成 mock，由 Ceedling 集成。
- gcov/gcovr：覆盖率统计，由 Ceedling 插件或 CI 调用。

初学者友好目录：

```text
test/
  test_ringbuffer.c
  test_mod_protocol.c
  test_mod_motor.c
  support/
    fake_bsp_stepper.c
    fake_bsp_eeprom.c
project.yml
```

测试时不要直接链接真实 BSP，而是链接 fake BSP。

术语说明：

- fake BSP：Fake Board Support Package，假的板级支持包。它在 PC 测试中模拟真实 `bsp_usart`、`bsp_adc`、`bsp_pwm` 等接口，不访问真实硬件。
- Mock：测试替身，用于验证函数调用次数、调用参数和返回路径。CMock 是 Unity 生态中的 C 语言 mock 工具。
- HIL：Hardware In the Loop，硬件在环测试，指自动化脚本连接真实板卡进行烧录、通信和外设行为验证。

最小命令：

```bash
ceedling test:all
ceedling gcov:all
```

### 8.5 Coverage CI

目标：

- 统计 PC 单元测试覆盖率。
- 找到未测试的协议分支、错误分支、状态机分支。

推荐工具：

- GCC/gcov
- lcov
- gcovr

建议先只对 PC 单元测试统计覆盖率，不对交叉编译固件统计覆盖率。

推荐指标演进：

```text
阶段 1：只生成覆盖率报告，不设置门槛
阶段 2：核心纯软件模块行覆盖率达到 50%
阶段 3：核心纯软件模块行覆盖率达到 70%
阶段 4：协议解析、状态机分支覆盖率重点补齐
```

不建议一开始给整个固件设置高覆盖率门槛，因为 BSP 和硬件相关代码很难在 PC 上完整覆盖。

### 8.6 Release / Artifact CD

目标：

- 打 tag 后自动构建 Release 固件。
- 上传 `.elf`、`.hex`、`.bin`、map 文件。
- 记录固件版本、提交号、构建时间。

建议阶段性实现：

1. PR 阶段只编译和测试。
2. merge 到 `main` 后上传短期 artifact。
3. 创建 tag，例如 `v0.1.0` 后，自动创建 GitHub Release。
4. Release 中包含固件、map、变更说明。

## 9. 测试策略

### 9.1 测试金字塔

推荐测试分层：

```text
硬件在环测试       少量，验证真实外设
集成测试           中等，验证 Module + Fake BSP
单元测试           大量，验证纯 C 逻辑
静态分析           每次提交运行
编译测试           每次提交运行
```

### 9.2 第一阶段测试目标

先不要追求所有代码都能测试。第一阶段只测纯软件逻辑：

- 协议帧解析。
- 环形缓冲。
- 参数范围检查。
- 状态机迁移。
- 数据打包和解包。
- 校验和 / CRC 包装逻辑，如果存在自定义算法。

### 9.3 第二阶段测试目标

引入 fake BSP：

```text
Module -> fake_bsp
```

例如：

- fake USART 收集发送数据，用于验证协议回复是否正确。
- fake ADC 返回指定采样值，用于验证上层判断逻辑。
- fake PWM 记录占空比，用于验证电机控制策略。

### 9.4 第三阶段测试目标

引入硬件在环测试：

- 使用 ST-Link 烧录固件。
- 使用串口脚本发送命令。
- 使用逻辑分析仪或串口回读验证结果。
- 使用 GitHub Actions self-hosted runner 或本地 Jenkins 执行。

硬件在环测试不建议一开始就做进公共 GitHub Actions，因为公共 runner 无法连接本地硬件。

## 10. 覆盖率策略

覆盖率应服务于风险控制，而不是成为数字游戏。

推荐规则：

1. 覆盖率只统计 `Application`、`Module`、`ThirdParty` 中可在 PC 上运行的部分。
2. BSP 层不强制覆盖率，优先通过硬件测试和代码审查保证质量。
3. 协议解析和状态机应重点关注分支覆盖率。
4. 错误路径必须有测试，例如空指针、长度错误、超时、非法状态。
5. 覆盖率报告作为 PR artifact 上传，方便审查。

推荐工具链：

```text
Ceedling + Unity + CMock + gcov/gcovr
```

推荐输出：

```text
coverage.xml
coverage.html
coverage-summary.txt
```

## 11. 自动化工具支援

### 11.1 构建工具

推荐：

- CMake Presets
- Ninja
- ARM GNU Toolchain
- STM32CubeCLT

本地命令：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Release：

```bash
cmake --preset Release
cmake --build --preset Release
```

### 11.2 格式化工具

推荐：

- clang-format

执行范围：

```text
Application/
Module/
BSP/
Core/
```

建议第三方组件默认不自动格式化，避免不必要的第三方源码改动。

### 11.3 静态分析工具

推荐按阶段引入：

1. flawfinder：当前已有。
2. cppcheck：适合 C 项目，配置简单。
3. clang-tidy：依赖 compile_commands，收益高但配置成本稍高。
4. CodeQL：适合安全扫描和长期质量追踪。

### 11.4 单元测试工具

推荐：

- Ceedling：优先采用，统一管理 Unity、CMock、gcov 和测试任务。
- Unity：轻量，适合嵌入式 C。
- CMock：适合 mock BSP 接口。
- CTest：可保留给后续特殊集成测试，不作为初期主测试框架。

初期建议：

```text
Ceedling
```

原因：

- 命令简单，`ceedling test:all` 即可运行所有测试。
- 自动生成测试 runner。
- CMock 可从 BSP 头文件生成 mock。
- gcov 插件可生成覆盖率。
- 比手动维护 CTest、Unity、mock 和覆盖率脚本更适合初学者。

### 11.5 文档工具

推荐：

- Markdown：当前文档主格式。
- Doxygen：后续生成 API 文档。
- Mermaid：用于架构图、状态机图、流程图。

建议文档目录：

```text
Doc/
  architecture_cmake_ci_evolution_plan.md
  coding_standard.md
  module_design.md
  ci_cd.md
  test_strategy.md
  release_process.md
```

### 11.6 版本与发布工具

推荐：

- Git tag：管理固件版本。
- GitHub Release：发布固件产物。
- changelog：记录变更。

固件版本建议包含：

```text
语义版本号
Git commit hash
构建时间
构建类型 Debug/Release
```

## 12. 持续演进路线

### 阶段 0：当前状态固化

目标：

- 确认当前工程可在本地和 CI 中稳定编译。
- 保留 CubeMX 工程能力。
- 不急于大规模移动代码。

执行：

1. 本地执行 Debug 构建。
2. 本地执行 Release 构建。
3. 确认 GitHub Actions build 成功。
4. 确认 flawfinder workflow 能运行。

### 阶段 1：建立分层 CMake

目标：

- 建立 `BSP`、`Module`、`Application` 的 CMake target。
- 保持固件编译通过。

执行：

1. 补齐 `BSP/CMakeLists.txt`。
2. 新增 `Module/CMakeLists.txt`。
3. 新增 `Application/CMakeLists.txt`。
4. 顶层 `CMakeLists.txt` 增加 `add_subdirectory()`。
5. 顶层链接 `application`。
6. 构建验证。

验收标准：

- Debug 构建通过。
- Release 构建通过。
- `.elf`、`.hex`、`.bin` 正常生成。

### 阶段 2：建立最小应用入口

目标：

- 让 `main.c` 只调用应用层入口。

执行：

1. 新增 `Application/Inc/app.h`。
2. 新增 `Application/Src/app.c`。
3. 实现：

```c
void app_Init(void);
void app_MainLoop(void);
```

4. 在 `Core/Src/main.c` 的 USER CODE 区域包含 `app.h`。
5. 在外设初始化后调用 `app_Init()`。
6. 在 while 循环中调用 `app_MainLoop()`。

验收标准：

- 固件编译通过。
- `main.c` 中没有新增业务逻辑。

### 阶段 3：建立 BSP 入口

目标：

- 统一 BSP 初始化入口。

执行：

1. 新增 `BSP/Inc/bsp.h`。
2. 新增 `BSP/Src/bsp.c`。
3. 实现：

```c
void bsp_Init(void);
```

4. 在 `bsp_Init()` 中注册或初始化 BSP 内部组件。
5. 在 `app_Init()` 中调用 `bsp_Init()`，或由 Module 初始化间接触发。为了初期清晰，推荐先由 `app_Init()` 调用。

说明：

- 严格分层下，Application 不应直接依赖 BSP。
- 但在项目早期，为了初始化路径简单，可以暂时由 `app_Init()` 调用 `bsp_Init()`。
- 当 Module 初始化体系稳定后，再把 BSP 初始化下沉到 Module 或平台初始化入口。

更严格的替代方案：

```text
main.c -> platform_Init() -> app_Init()
```

其中 `platform_Init()` 负责 BSP 初始化，`app_Init()` 只负责应用初始化。

### 阶段 4：迁移业务逻辑

目标：

- 将业务代码逐步从 `Core` 移出。

执行顺序：

1. 先迁移与协议解析相关的纯软件逻辑到 `Module`。
2. 再迁移电机、脉冲、阀门控制策略到 `Module`。
3. 再迁移业务流程和状态机到 `Application`。
4. 最后清理 `main.c` 中残留逻辑。

验收标准：

- `Core/Src/main.c` 只保留启动、CubeMX 初始化和应用入口调用。
- `Module` 中没有 LL/HAL 调用。
- `Application` 中没有 BSP/LL/HAL 调用。

### 阶段 5：引入 ThirdParty

目标：

- 规范第三方组件管理。

执行：

1. 新增 `ThirdParty` 目录。
2. 每个第三方组件一个子目录。
3. 每个第三方组件保留许可证和来源说明。
4. 在 `ThirdParty/CMakeLists.txt` 中为每个组件建立 target。
5. Module 或 BSP 通过 `target_link_libraries()` 依赖具体组件。

验收标准：

- 第三方组件不散落在业务目录中。
- 替换或升级某个组件时，影响范围可控。

### 阶段 6：引入单元测试

目标：

- PC 上可测试纯软件模块。

执行：

1. 新增 `test` 目录和 `project.yml`。
2. 使用 Ceedling 作为 PC 单元测试入口。
3. 为 ringbuffer 或 protocol 建立第一个测试。
4. 使用 CMock 或 fake BSP 隔离硬件依赖。
5. CI 中增加 `ceedling test:all`。
6. PR 中要求测试通过。

验收标准：

- 本地能执行 `ceedling test:all`。
- CI 能执行 `ceedling test:all`。
- 至少一个核心纯软件模块有测试。

### 阶段 7：引入覆盖率

目标：

- 对可测试模块生成覆盖率报告。

执行：

1. 为 PC 测试构建增加 coverage 选项。
2. 使用 gcovr 生成报告。
3. CI 上传 HTML 或 XML 覆盖率报告。
4. 初期不设置硬门槛，只观察。
5. 稳定后对核心模块设置门槛。

验收标准：

- PR artifact 中能看到覆盖率报告。
- 协议解析、状态机等核心模块覆盖率逐步提升。

### 阶段 8：引入硬件在环测试

目标：

- 验证真实硬件行为。

执行：

1. 准备固定测试板。
2. 准备烧录脚本。
3. 准备串口测试脚本。
4. 本地或 self-hosted runner 执行。
5. 将硬件测试从 PR 必选项设计为夜间任务或发布前任务。

验收标准：

- 固件能自动烧录。
- 串口命令能自动收发验证。
- 关键外设功能能被脚本确认。

## 13. CI/CD 推荐阶段表

| 阶段 | 必须项 | 可选项 | 目标 |
| --- | --- | --- | --- |
| 初期 | Build、flawfinder | clang-format check | 保证能编译，减少明显风险 |
| 中期 | Build、format、cppcheck、unit test | coverage | 稳定开发节奏 |
| 后期 | Build、static analysis、unit test、coverage、release artifact | HIL test | 支持发布和长期维护 |
| 成熟期 | 全量 CI、tag release、HIL、质量门禁 | 自动 changelog | 工程化闭环 |

## 14. 设计决策记录

### 14.1 为什么不把所有源码都放进顶层 CMakeLists

顶层 CMakeLists 如果直接列出所有 `.c` 文件，会很快变成一个难维护的大清单。初学者看不出哪个文件属于哪层，也看不出依赖关系。

每层一个 CMakeLists 后，目录结构和构建结构一致，更适合长期维护。

### 14.2 为什么 Module 不直接调用 LL

Module 的价值是复用业务能力。如果 Module 直接调用 LL，它就被 STM32F103 和当前板级配置绑定，后续难以在 PC 测试，也难以移植到其他 STM32 或其他硬件。

把硬件访问限制在 BSP 后，Module 可以通过 fake BSP 在 PC 上测试。

### 14.3 为什么 Application 不直接调用 BSP

Application 表达业务流程，BSP 表达硬件能力。如果 Application 直接调用 BSP，业务流程会和硬件细节耦合，后续状态机、协议、测试都会变复杂。

更好的方式是 Application 调用 Module，由 Module 决定需要哪些硬件能力。

### 14.4 为什么 ThirdParty 独立出来

第三方组件有自己的版本、许可证和维护节奏。独立目录可以让项目清楚知道：

- 这个组件从哪里来。
- 当前版本是什么。
- 是否修改过源码。
- 哪些模块依赖它。

这对长期维护和开源合规都重要。

### 14.5 为什么覆盖率先从 PC 单元测试开始

嵌入式项目的硬件相关代码很难在普通 CI 上覆盖。如果一开始追求全工程覆盖率，会花很多时间 mock 寄存器和外设，收益不高。

更合理的顺序是：

1. 先测纯软件逻辑。
2. 再 fake BSP 测 Module。
3. 最后用硬件在环测真实外设。

## 15. 推荐检查清单

每次新增模块时检查：

- 是否放在正确层级？
- 是否新增了对应 `.h` 和 `.c`？
- 是否更新了对应层的 `CMakeLists.txt`？
- Module 是否避免直接包含 `stm32f1xx_ll_xxx.h`？
- Application 是否避免包含 `bsp_xxx.h`？
- 公共头文件是否只暴露必要接口？
- 是否有参数检查？
- 是否有错误返回值？
- 是否能写 PC 单元测试？
- 是否需要更新文档？

每次提交前检查：

- 本地 Debug 构建通过。
- 本地 Release 构建通过。
- 格式化通过。
- 单元测试通过，如果已有测试。
- 没有把临时调试代码提交到 `main`。
- 没有修改 CubeMX 生成区中不必要的内容。

## 16. 推荐下一步

建议按以下顺序执行，不要一次性大改：

1. 建立或修正 `BSP/CMakeLists.txt`。
2. 新增 `Application/CMakeLists.txt` 和最小 `app.c/app.h`。
3. 新增 `Module/CMakeLists.txt`。
4. 顶层 CMake 接入三个目录。
5. 修改 `main.c`，只调用 `app_Init()` 和 `app_MainLoop()`。
6. 编译验证。
7. 增加第一个 BSP 模块，例如 `bsp_usart` 或 `bsp_dwt`。
8. 增加第一个 Module 模块，例如 `mod_protocol`。
9. 增加第一个 PC 单元测试。
10. CI 增加 format check、cppcheck、unit test、coverage。

这条路线的重点是小步演进：每一步都能编译，每一步都能回退，每一步都能让项目更清楚。
