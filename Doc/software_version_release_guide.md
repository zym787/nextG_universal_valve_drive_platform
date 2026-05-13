# 软件版本与发布管理指南

返回：[工程文档总入口](about.md)

本文档固定固件版本来源、固件内版本信息、发布产物命名和 GitHub Actions 发布流程。

## 1. 结论

软件版本以 Git tag 为唯一发布版本来源：

```text
vMAJOR.MINOR.PATCH
```

示例：

```text
v0.1.0
v0.2.0
v1.0.0
```

固件中可以使用宏，但这些宏不建议手写维护，应由 CMake 在构建时从 Git tag、commit、硬件版本和构建配置生成。

推荐链路：

```text
Git tag
  -> CMake 生成 app_version.h
  -> 固件内置 app_version_info
  -> 协议命令可读取版本
  -> GitHub Actions 生成带版本号的产物
  -> GitHub Release 上传固件和 firmware-info.txt
```

## 2. 版本字段

固件版本信息建议包含：

| 字段 | 来源 | 示例 |
| --- | --- | --- |
| firmware version | Git tag | `0.1.0` |
| git commit | Git commit hash | `a1b2c3d` |
| build type | CMake preset | `Release` |
| build time | CI 构建时间 | `2026-05-14T10:00:00Z` |
| board hardware | CMake `BOARD_HW` | `hw_rev_a` |
| protocol version | 项目固定宏 | `1` |
| config schema version | 参数结构版本 | `1` |

开发构建没有 tag 时，版本显示为：

```text
0.0.0-dev+<short_commit>
```

## 3. CMake 生成版本头文件

建议新增模板文件：

```text
Application/Inc/app_version.h.in
```

模板示例：

```c
#ifndef APP_VERSION_H
#define APP_VERSION_H

#define APP_VERSION_STRING        "@APP_VERSION_STRING@"
#define APP_GIT_COMMIT           "@APP_GIT_COMMIT@"
#define APP_BUILD_TYPE           "@CMAKE_BUILD_TYPE@"
#define APP_BUILD_TIME_UTC       "@APP_BUILD_TIME_UTC@"
#define APP_BOARD_HW             "@BOARD_HW@"
#define APP_PROTOCOL_VERSION     1U
#define APP_CONFIG_SCHEMA_VERSION 1U

#endif
```

CMake 生成到构建目录：

```cmake
execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE APP_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git describe --tags --exact-match
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE APP_GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(APP_GIT_TAG)
    string(REGEX REPLACE "^v" "" APP_VERSION_STRING "${APP_GIT_TAG}")
else()
    set(APP_VERSION_STRING "0.0.0-dev+${APP_GIT_COMMIT}")
endif()

string(TIMESTAMP APP_BUILD_TIME_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)

configure_file(
    ${CMAKE_SOURCE_DIR}/Application/Inc/app_version.h.in
    ${CMAKE_BINARY_DIR}/generated/app_version.h
    @ONLY
)

target_include_directories(application PUBLIC
    ${CMAKE_BINARY_DIR}/generated
)
```

## 4. 固件内版本结构

建议在 `Application` 或 `Service` 中提供只读版本结构：

```c
typedef struct {
    const char *version;
    const char *commit;
    const char *build_type;
    const char *build_time;
    const char *board_hw;
    uint16_t protocol_version;
    uint16_t config_schema_version;
} app_version_info_t;

const app_version_info_t *app_version_GetInfo(void);
```

版本字符串放在 Flash 中。STM32F103C8T6 资源有限，字符串保持短小，不保存冗长 release notes。

协议层应提供读取版本命令，至少返回：

```text
firmware_version
git_commit
board_hw
protocol_version
config_schema_version
```

## 5. 发布产物命名

Release 产物必须体现版本和硬件版本：

```text
valve-fw-hw_rev_a-v0.1.0.elf
valve-fw-hw_rev_a-v0.1.0.hex
valve-fw-hw_rev_a-v0.1.0.bin
valve-fw-hw_rev_a-v0.1.0.map
firmware-info-hw_rev_a-v0.1.0.txt
sha256sums.txt
```

`firmware-info` 至少包含：

```text
project
version
git_commit
board_hw
build_type
build_time_utc
toolchain
flash_size
ram_size
sha256_bin
```

## 6. GitHub Actions 发布流程

推荐流程：

```text
push / pull_request
  -> build
  -> static analysis
  -> unit test
  -> coverage artifact

push tag vX.Y.Z
  -> build Release
  -> unit test
  -> coverage
  -> generate firmware-info
  -> rename artifacts with version and board
  -> create GitHub Release
```

一次提交并发布的操作方式：

```bash
git commit -m "release: v0.1.0"
git tag v0.1.0
git push origin main
git push origin v0.1.0
```

也可以先合并到 `main`，确认 CI 通过后再创建 tag。发布版本以 tag 为准，不要求每次手动修改源码中的版本宏。

## 7. 执行计划

项目级详细 TODO 统一维护在 [about.md](about.md)。版本发布本层开发只保留以下检查重点：

- 发布版本只来自 Git tag，不手写散落版本宏。
- CMake 自动生成 `app_version.h`。
- 固件协议必须能读取版本、commit、硬件版本、协议版本。
- Release 产物文件名必须包含 `board_hw` 和版本号。
- Release 前必须通过构建、静态分析、单元测试和覆盖率生成。
