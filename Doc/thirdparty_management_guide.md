# ThirdParty 管理指南

返回：[工程文档总入口](about.md)

本文档说明 `ThirdParty` 如何管理。`ThirdParty` 是外部组件区，不属于项目自有分层；其源码尽量保持上游原样，减少后续升级成本。

## 1. 定位

一句话：

```text
ThirdParty 放外部源码，项目自有适配代码放 Service、Module 或 BSP。
```

分阶段引入：

| 阶段 | 组件 | 说明 |
| --- | --- | --- |
| 第一批 | ringbuffer、最小 event、最小 Modbus RTU slave | 先满足通信和协议闭环 |
| 第二批 | 轻量 log | Release 必须可裁剪，字符串要短 |
| 第三批 | 轻量 OSAL scheduler | 只做 tick、timeout、短任务调度 |

结论：第三方源码保持上游命名，并集中放在 `ThirdParty`。项目自有适配代码放回 `Service`、`Module` 或 `BSP`。

## 2. 管理方式

初期推荐 vendor：

```text
ThirdParty/
  ringbuffer/
  event/
  modbus/
  log/          后续加入
  osal/         后续加入
  CMakeLists.txt
```

后续是否使用 git submodule：

- 小型、稳定、文件少的库：继续 vendor。
- Modbus、日志等可能长期升级的库：稳定后可以考虑 git submodule。
- 团队 git 经验不足时，先不用 submodule，避免增加操作门槛。

每个组件建议保留：

```text
README.md
LICENSE
VERSION.txt 或 SOURCE.txt
```

`SOURCE.txt` 建议记录：

```text
Name:
Upstream:
Version/Commit:
Imported Date:
Local Modifications:
```

## 3. 命名规则

ThirdParty 内：

- 文件名保持上游原名。
- 函数名保持上游原名。
- 类型名和宏保持上游原名。
- 不强制使用 `app_`、`svc_`、`mod_`、`bsp_`。

项目自有适配层：

- 协议适配放 `Service`，使用 `svc_`。
- 器件适配放 `Module`，使用 `mod_`。
- MCU 外设适配放 `BSP`，使用 `bsp_`。

## 4. CMake

示例：

```cmake
add_library(ringbuffer STATIC
    ringbuffer/ringbuffer.c
)

target_include_directories(ringbuffer PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/ringbuffer
)

add_library(modbus STATIC
    modbus/mb.c
    modbus/mbcrc.c
)

target_include_directories(modbus PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/modbus
)
```

上层依赖：

```cmake
target_link_libraries(service PUBLIC
    ringbuffer
    modbus
    event
    log
)
```

规则：

- 每个第三方组件一个 CMake target。
- 不在顶层 `CMakeLists.txt` 直接列第三方源码。
- 不把 ThirdParty 目录整体暴露为 include path。

## 5. 推荐组件策略

### 5.1 ringbuffer

用途：

- USART RX buffer。
- 协议输入缓存。
- 日志输出缓存。

要求：

- 不动态分配。
- API 简单。
- 可 PC 单元测试。

### 5.2 event/message

用途：

- 协议命令事件。
- 到位事件。
- 故障事件。
- 周期 tick 事件。

初期可以使用固定长度队列，不引入复杂事件总线。

### 5.3 modbus

建议：

- 初期使用 Modbus RTU slave。
- 用于工厂写参、状态读取、动作测试。
- 先 vendor 固定版本，稳定后再评估 submodule。
- 选择库时优先考虑代码体积、无动态内存、可裁剪功能，不追求协议栈“大而全”。

### 5.4 osal

OSAL 放到第三批。初期 OSAL 只做轻量抽象：

```c
uint32_t osal_GetTickMs(void);
bool osal_IsTimeout(uint32_t start_ms, uint32_t timeout_ms);
void osal_SchedulerPoll(void);
```

不要一开始做完整 RTOS 封装。
不要把队列、线程、互斥锁、动态内存池一次做全；等 FreeRTOS/RT-Thread 真正引入时再扩展。

### 5.5 log

要求：

- Release 可裁剪。
- 不大量占用 RAM。
- 不在中断中格式化长字符串。
- 支持输出到串口或 ringbuffer。
- 禁止默认引入完整 `printf` 浮点格式化。
- 日志字符串尽量短，错误码优先。

## 6. 测试和 CI

CI 建议：

- 编译 ThirdParty。
- 对可测试组件运行 PC 单元测试。
- 静态分析默认可排除第三方源码，减少噪声。
- 如果修改了 ThirdParty 源码，必须记录修改原因。

覆盖率建议：

- ringbuffer、event 可以统计覆盖率。
- modbus 等大型第三方库初期不强制覆盖率。

## 7. 执行计划

项目级详细 TODO 统一维护在 [about.md](about.md)。ThirdParty 本层开发只保留以下检查重点：

- 第三方源码保持原始文件名、函数名、类型名和宏定义。
- 每个组件记录来源、版本或 commit、许可证。
- 第一批只引入 ringbuffer、最小 event、最小 Modbus RTU slave。
- 小型稳定库优先 vendor；Modbus 稳定后再评估 git submodule。
- 静态分析默认可排除大型第三方源码，减少噪声。
- 修改第三方源码时必须记录 patch 原因。
