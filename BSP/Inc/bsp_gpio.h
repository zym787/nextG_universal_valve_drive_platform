/**
 * @file    bsp_gpio.h
 * @brief   通用 MCU GPIO 抽象。
 *          - 对外只提供 id（uint16_t），不暴露 GPIO_TypeDef* / LL_GPIO_xxx 类型；
 *          - 由 BSP 内部维护 id 到 (port, pin) 的映射，保证 Module/Service/Application
 *            可以在不知 STM32 寄存器的前提下读写 GPIO。
 *
 * @note    本头文件遵循 bsp_development_guide.md 第 9 / 12 节：
 *            - public header 不包含 stm32f1xx_ll_*.h
 *            - 业务名（如 LED、DIR）不出现在 BSP 层
 */

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GPIO 通用 id：BSP 内部映射到 (port, pin)。
 * 不在 BSP 中赋予业务含义（不写 LED / DIR / EN 这种名字）。
 * 命名仅用于"槽位"区分。 */
typedef enum {
    BSP_GPIO_ID_USER0 = 0,    /* 当前映射到 PC14（CubeMX: WORK_LED1） */
    /* 后续按需追加：BSP_GPIO_ID_USER1, ... */
    BSP_GPIO_ID_COUNT
} bsp_gpio_id_t;

/**
 * @brief 初始化 GPIO 模块（当前 CubeMX 已配置 PC14 输出，本函数仅检查/兜底）
 */
bsp_status_t bsp_gpio_Init(void);

/**
 * @brief 设置 GPIO 电平
 * @param id    GPIO 槽位
 * @param level true=高电平，false=低电平
 */
bsp_status_t bsp_gpio_Write(bsp_gpio_id_t id, bool level);

/**
 * @brief 读取 GPIO 电平
 * @param id    GPIO 槽位
 * @param level 输出参数：当前电平
 */
bsp_status_t bsp_gpio_Read(bsp_gpio_id_t id, bool *level);

/**
 * @brief 翻转 GPIO 电平
 */
bsp_status_t bsp_gpio_Toggle(bsp_gpio_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* BSP_GPIO_H */
