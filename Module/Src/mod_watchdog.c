/**
 * @file    mod_watchdog.c
 * @brief   IWDG 器件级抽象：薄封装 bsp_iwdg；
 *          - 把 bsp_iwdg_status_t 收敛为 mod_status_t；
 *          - 不引入策略（策略放在 svc_watchdog）。
 */

#include "mod_watchdog.h"

#include "bsp_iwdg.h"

/* bsp_iwdg 使用独立的状态枚举（bsp_iwdg_status_t），
   在这里集中转译，避免向上透传底层错误码。 */
static inline mod_status_t to_mod(bsp_iwdg_status_t s)
{
    switch (s) {
    case BSP_IWDG_OK:              return MOD_OK;
    case BSP_IWDG_ERR_INVALID_ARG: return MOD_ERR_PARAM;
    case BSP_IWDG_ERR_TIMEOUT:     return MOD_ERR_TIMEOUT;
    case BSP_IWDG_ERR_NOT_READY:   return MOD_ERR_NOT_INIT;
    default:                       return MOD_ERR_HW;
    }
}

mod_status_t mod_watchdog_Init(uint32_t timeout_ms)
{
    return to_mod(bsp_iwdg_Init(timeout_ms));
}

mod_status_t mod_watchdog_Feed(void)
{
    /* bsp_iwdg_Refresh 无返回值（IWDG 重载寄存器写入不会失败）；
       这里统一返回 MOD_OK，保留接口形态以便未来替换为带状态的实现。 */
    bsp_iwdg_Refresh();
    return MOD_OK;
}

bool mod_watchdog_WasResetByWatchdog(void)
{
    return bsp_iwdg_WasResetByWatchdog();
}

uint32_t mod_watchdog_GetTimeoutMs(void)
{
    return bsp_iwdg_GetTimeoutMs();
}
