/**
 * @file    mod_systime.c
 * @brief   把 BSP 的毫秒 tick 桥接给 Service 层。
 *          Module 层允许 #include "bsp_timer.h"；Service 不行。
 */

#include "mod_systime.h"

#include "bsp_timer.h"

mod_status_t mod_systime_Init(void)
{
    return (bsp_timer_Init() == BSP_OK) ? MOD_OK : MOD_ERR_HW;
}

unsigned int mod_systime_GetMs(void)
{
    return (unsigned int)bsp_timer_GetTickMs();
}

bool mod_systime_IsTimeout(unsigned int start_ms, unsigned int timeout_ms)
{
    return bsp_timer_IsTimeout((uint32_t)start_ms, (uint32_t)timeout_ms);
}
