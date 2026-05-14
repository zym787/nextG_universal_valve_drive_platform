/**
 * @file    mod_indicator.c
 * @brief   板上指示灯 Module 实现，桥接到 bsp_gpio。
 */

#include "mod_indicator.h"

#include "bsp_gpio.h"
#include "bsp_timer.h"

/* 闪烁状态机：每个指示灯独立维护一个周期与最近翻转时刻 */
typedef struct {
    bool         enabled;     /* 是否启用自动闪烁 */
    unsigned int period_ms;   /* 半周期（保持时长）  */
    unsigned int last_ms;     /* 上次翻转的 BSP tick */
} mod_indicator_blink_t;

static mod_indicator_blink_t g_blink[MOD_INDICATOR_COUNT] = {0};

/* indicator -> bsp gpio id 的映射（板级配置，未来可放到 BoardConfig） */
static const bsp_gpio_id_t k_indicator_to_gpio[MOD_INDICATOR_COUNT] = {
    [MOD_INDICATOR_WORK] = BSP_GPIO_ID_USER0,
};

/* 把 bsp_status_t 收敛为 mod_status_t，避免向上透传底层错误码 */
static inline mod_status_t to_mod(bsp_status_t s)
{
    switch (s) {
    case BSP_OK:              return MOD_OK;
    case BSP_ERR_PARAM:       return MOD_ERR_PARAM;
    case BSP_ERR_NOT_INIT:    return MOD_ERR_NOT_INIT;
    case BSP_ERR_BUSY:        return MOD_ERR_BUSY;
    case BSP_ERR_TIMEOUT:     return MOD_ERR_TIMEOUT;
    case BSP_ERR_UNSUPPORTED: return MOD_ERR_UNSUPPORTED;
    case BSP_ERR_IO:          return MOD_ERR_IO;
    default:                  return MOD_ERR_HW;
    }
}

mod_status_t mod_indicator_Init(void)
{
    return to_mod(bsp_gpio_Init());
}

mod_status_t mod_indicator_Set(mod_indicator_t which, bool on)
{
    if (which >= MOD_INDICATOR_COUNT) { return MOD_ERR_PARAM; }
    /* WORK_LED1 默认电路：高电平点亮 / 低电平熄灭。
       如果硬件相反，只需在这里取反；上层无感知。 */
    return to_mod(bsp_gpio_Write(k_indicator_to_gpio[which], on));
}

mod_status_t mod_indicator_Toggle(mod_indicator_t which)
{
    if (which >= MOD_INDICATOR_COUNT) { return MOD_ERR_PARAM; }
    return to_mod(bsp_gpio_Toggle(k_indicator_to_gpio[which]));
}

mod_status_t mod_indicator_StartBlink(mod_indicator_t which, unsigned int period_ms)
{
    if (which >= MOD_INDICATOR_COUNT) { return MOD_ERR_PARAM; }
    if (period_ms == 0U)              { return MOD_ERR_PARAM; }

    g_blink[which].enabled   = true;
    g_blink[which].period_ms = period_ms;
    g_blink[which].last_ms   = bsp_timer_GetTickMs();
    return MOD_OK;
}

void mod_indicator_Poll(void)
{
    const unsigned int now = bsp_timer_GetTickMs();

    for (unsigned int i = 0U; i < (unsigned int)MOD_INDICATOR_COUNT; ++i) {
        if (!g_blink[i].enabled) { continue; }

        /* 32-bit 无符号差值天然处理 tick 回绕 */
        if ((unsigned int)(now - g_blink[i].last_ms) >= g_blink[i].period_ms) {
            g_blink[i].last_ms = now;
            (void)bsp_gpio_Toggle(k_indicator_to_gpio[i]);
        }
    }
}
