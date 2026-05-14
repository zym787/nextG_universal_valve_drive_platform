/**
 * @file    bsp_gpio.c
 * @brief   GPIO 通用抽象实现（基于 STM32 LL）。
 *
 * 现状：
 *   - CubeMX 在 MX_GPIO_Init() 中已完成时钟使能与 PC14 输出模式配置；
 *   - 本文件仅维护"id -> (port, pin)"映射表，将抽象 API 桥接到 LL。
 *
 * 后续扩展：
 *   - 增加 USER1/USER2 槽位时只需扩 g_gpio_table，不必改头文件之外。
 *   - 引入 BoardConfig 后，table 可由 BoardConfig 提供以支持多板差异。
 */

#include "bsp_gpio.h"

#include "stm32f1xx_ll_gpio.h"

/* ------------------------------------------------------------------------- */
/* id -> (port, pin) 映射表                                                  */
/* ------------------------------------------------------------------------- */
typedef struct {
    GPIO_TypeDef *port;
    uint32_t      pin;        /* LL_GPIO_PIN_xx */
} bsp_gpio_map_t;

static const bsp_gpio_map_t g_gpio_table[BSP_GPIO_ID_COUNT] = {
    /* BSP_GPIO_ID_USER0 -> PC14（CubeMX: WORK_LED1） */
    [BSP_GPIO_ID_USER0] = { GPIOC, LL_GPIO_PIN_14 },
};

static bool g_initialized = false;

/* ------------------------------------------------------------------------- */
/* 公开接口                                                                  */
/* ------------------------------------------------------------------------- */

bsp_status_t bsp_gpio_Init(void)
{
    /* CubeMX MX_GPIO_Init() 已使能 GPIOC 时钟并配置 PC14 为推挽输出。
       这里仅置标志，留出后续做参数校验/兜底初始化的位置。 */
    g_initialized = true;
    return BSP_OK;
}

bsp_status_t bsp_gpio_Write(bsp_gpio_id_t id, bool level)
{
    if (!g_initialized)         { return BSP_ERR_NOT_INIT; }
    if (id >= BSP_GPIO_ID_COUNT){ return BSP_ERR_PARAM;    }

    const bsp_gpio_map_t *m = &g_gpio_table[id];
    if (level) {
        LL_GPIO_SetOutputPin(m->port, m->pin);
    } else {
        LL_GPIO_ResetOutputPin(m->port, m->pin);
    }
    return BSP_OK;
}

bsp_status_t bsp_gpio_Read(bsp_gpio_id_t id, bool *level)
{
    if (!g_initialized)          { return BSP_ERR_NOT_INIT; }
    if (id >= BSP_GPIO_ID_COUNT) { return BSP_ERR_PARAM;    }
    if (level == (void *)0)      { return BSP_ERR_PARAM;    }

    const bsp_gpio_map_t *m = &g_gpio_table[id];
    /* 输出模式下读"输出寄存器"更直观；如需读输入请自行替换为 LL_GPIO_IsInputPinSet */
    *level = (LL_GPIO_IsOutputPinSet(m->port, m->pin) != 0U);
    return BSP_OK;
}

bsp_status_t bsp_gpio_Toggle(bsp_gpio_id_t id)
{
    if (!g_initialized)          { return BSP_ERR_NOT_INIT; }
    if (id >= BSP_GPIO_ID_COUNT) { return BSP_ERR_PARAM;    }

    const bsp_gpio_map_t *m = &g_gpio_table[id];
    LL_GPIO_TogglePin(m->port, m->pin);
    return BSP_OK;
}
