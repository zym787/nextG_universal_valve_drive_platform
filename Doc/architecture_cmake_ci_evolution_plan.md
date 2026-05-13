# CMake、CI/CD 与测试方案

返回：[工程文档总入口](about.md)

本文档说明如何组织 CMake、单元测试、覆盖率、CI/CD 和自动发布。项目级详细 TODO 统一维护在 [about.md](about.md)。

## 1. 构建目标

本项目同时需要两套构建体系：

```text
CMake:    STM32 固件交叉编译，生成 elf/hex/bin/map
Ceedling: PC 单元测试，生成测试结果和覆盖率
```

不要把所有测试硬塞进 CMake，也不要让 Ceedling 负责 STM32 固件交叉编译。二者职责分开，初学者更容易维护。

## 2. CMake target 依赖

推荐 target 图：

```text
application -> service -> module -> bsp -> stm32cubemx
service/module/bsp -> selected_board_config
service/module/bsp -> thirdparty targets as needed
```

顶层 `CMakeLists.txt` 建议按依赖顺序加入：

```cmake
add_subdirectory(BoardConfig)
add_subdirectory(ThirdParty)
add_subdirectory(BSP)
add_subdirectory(Module)
add_subdirectory(Service)
add_subdirectory(Application)
```

最终固件 target 链接：

```cmake
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        application
        service
        module
        bsp
        selected_board_config
        stm32cubemx
)
```

## 3. 各目录 CMake 职责

### 3.1 BoardConfig

```cmake
set(BOARD_HW "hw_rev_a" CACHE STRING "Board hardware revision")

add_library(selected_board_config OBJECT
    ${BOARD_HW}/board_config.c
)

target_include_directories(selected_board_config PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/${BOARD_HW}
)
```

`BOARD_HW` 由 CMake preset 或 CI matrix 指定。

### 3.2 BSP

```cmake
add_library(bsp OBJECT)

target_sources(bsp PRIVATE
    Src/bsp_gpio.c
    Src/bsp_usart.c
    Src/bsp_timer.c
    Src/bsp_pwm.c
    Src/bsp_iwdg.c
    Src/bsp_dwt.c
    Src/bsp_soft_i2c.c
    bsp.c
)

target_include_directories(bsp PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Inc
)

target_link_libraries(bsp PUBLIC
    stm32cubemx
    selected_board_config
)
```

BSP 可以依赖 CubeMX/LL/CMSIS，其他层不直接依赖。

### 3.3 Module

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

Module 不直接链接 `stm32cubemx`。

### 3.4 Service

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

Service 不直接链接 `bsp` 和 `stm32cubemx`。

### 3.5 Application

```cmake
add_library(application OBJECT)

target_sources(application PRIVATE
    Src/app.c
    Src/app_mode.c
    Src/app_guardian.c
    Src/app_version.c
)

target_include_directories(application PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Inc
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(application PUBLIC
    service
)
```

Application 只链接 Service。

## 4. CMake Presets

建议至少提供：

```text
Debug
Release
Debug-hw-rev-a
Release-hw-rev-a
```

示例：

```json
{
  "name": "Release-hw-rev-a",
  "inherits": "Release",
  "cacheVariables": {
    "BOARD_HW": "hw_rev_a"
  }
}
```

本地构建命令：

```bash
cmake --preset Release-hw-rev-a
cmake --build --preset Release-hw-rev-a
```

## 5. 版本头文件

版本来源固定为 Git tag：

```text
vMAJOR.MINOR.PATCH
```

CMake 生成：

```text
build/generated/app_version.h
```

字段至少包含：

```text
firmware_version
git_commit
build_time_utc
build_type
board_hw
protocol_version
config_schema_version
```

详细规则见 [软件版本与发布管理指南](software_version_release_guide.md)。

## 6. Ceedling 单元测试

推荐目录：

```text
test/
  test_service/
  test_module/
  test_application/
  support/
project.yml
```

测试策略：

- Service 测试 mock Module。
- Module 测试 mock BSP。
- Application 测试 mock Service。
- BSP 不强制 PC 覆盖率，后续用 HIL 和板级验证。

推荐命令：

```bash
ceedling test:all
ceedling gcov:all
```

初期覆盖率只观察；稳定后先对 Service 设置门槛，再逐步扩大。

## 7. GitHub Actions

推荐 job：

```text
format:       clang-format dry-run
build:        CMake Debug/Release
static-check: flawfinder/cppcheck/CodeQL
test:         Ceedling unit test
coverage:     gcov/gcovr report artifact
release:      tag 触发固件发布
```

PR 必须通过：

- CMake build。
- 单元测试。
- 基础静态分析。

Release 必须通过：

- Release 构建。
- 单元测试。
- 覆盖率生成。
- 固件产物重命名。
- `firmware-info` 和 `sha256sums` 生成。

## 8. 发布产物

tag `v0.1.0`、板卡 `hw_rev_a` 的产物命名：

```text
valve-fw-hw_rev_a-v0.1.0.elf
valve-fw-hw_rev_a-v0.1.0.hex
valve-fw-hw_rev_a-v0.1.0.bin
valve-fw-hw_rev_a-v0.1.0.map
firmware-info-hw_rev_a-v0.1.0.txt
sha256sums.txt
coverage-summary.txt
```

发布命令示例：

```bash
git tag v0.1.0
git push origin v0.1.0
```

## 9. 自动化工具

推荐工具：

| 任务 | 工具 |
| --- | --- |
| 本地构建 | CMake Tools、Ninja、ARM GNU Toolchain |
| 单元测试 | Ceedling、Unity、CMock |
| 覆盖率 | gcov、gcovr |
| 格式 | clang-format |
| 静态分析 | flawfinder、cppcheck、CodeQL |
| 烧录 | STM32CubeProgrammer CLI |
| 生产工具 | Python、pyserial、pymodbus、JSON schema |

## 10. 检查项

- `application` 不直接链接 `module`、`bsp`、`stm32cubemx`。
- `service` 不直接链接 `bsp`、`stm32cubemx`。
- `module` 不直接链接 `stm32cubemx`。
- `BOARD_HW` 能在本地和 CI 中明确指定。
- Debug/Release 都能构建。
- tag 发布能从 Git tag 推导版本，不依赖手改宏。
