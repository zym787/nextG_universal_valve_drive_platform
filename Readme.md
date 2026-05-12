# nextG universal valve drive platform
下世代通用阀门驱动平台

[![Build STM32 CMake Project](https://github.com/zym787/nextG_universal_valve_drive_platform/actions/workflows/build.yml/badge.svg)](https://github.com/zym787/nextG_universal_valve_drive_platform/actions/workflows/build.yml)
[![CodeQL](https://github.com/zym787/nextG_universal_valve_drive_platform/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/zym787/nextG_universal_valve_drive_platform/actions/workflows/github-code-scanning/codeql)

# Architecture Design
```mermaid
block-beta
    columns 3
    %% App
    Application:1
    block:app:2
        columns 2
        AgreementDomain
        Movementdomain
        ControlDomain
        GuardianDomain
    end
    %% Module
    Module:1
    block:module:2
        columns 2
        Protocol Motor Storage OSAL
    end
    %% BSP
    BSP:1
    block:bsp:2
        columns 2
        bsp_USART bsp_I2C bsp_DLCI bsp_PULSE
    end
    %% Cube HAL驱动层
    block:cube:3
        columns 2
        Core:1
        block:cubemx_per:1
            columns 3
            GPIO DMA CRC
            IWDG TIM USART
        end
        Driver:1
        block:cubemx:1
            CMSIS HAL/LL
        end
    end
```

## Application
### AgreementDomain
协议域
### Movementdomain
运动域
### ControlDomain
控制域
### GuardianDomain
守护域
## Module
### Protocol
协议解析
### Motor
运动控制
### Storage
存储
### OSAL
简单线程调度
## BSP
### bsp_USART
串口
### bsp_I2C
软IIC
### bsp_DLCI
数字逻辑控制接口
### bsp_PULSE
脉冲生成
## Cube
### CMSIS
### HAL/LL
