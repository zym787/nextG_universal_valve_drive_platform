/**
 * @file    mod_watchdog.h
 * @brief   独立看门狗"器件级"抽象。
 *          - 只暴露 Init / Feed / WasResetByWatchdog / GetTimeoutMs；
 *          - Service 通过本接口操作 IWDG，不直接调用 bsp_iwdg。
 *
 * 调用关系：
 *   svc_watchdog_*  ->  mod_watchdog_*  ->  bsp_iwdg_*  ->  LL_IWDG_*
 */

#ifndef MOD_WATCHDOG_H
#define MOD_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#include "mod_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 IWDG 并启动；启用后不可关闭，仅可被复位清除。
 * @param timeout_ms 期望溢出时间（毫秒），有效范围约 1 ~ 26214。
 * @note  喂狗周期必须严格小于 timeout_ms（建议留 30%~50% 裕量）。
 */
mod_status_t mod_watchdog_Init(uint32_t timeout_ms);

/**
 * @brief 喂狗（重载计数器）。
 * @note  必须由主循环 / 调度器周期性调用，禁止放在中断里。
 *        本函数对 IWDG 操作幂等，可安全重复调用。
 */
mod_status_t mod_watchdog_Feed(void);

/**
 * @brief 查询本次启动是否由 IWDG 复位触发。
 * @note  应在 mod_watchdog_Init 之前调用；调用后将清除底层复位标志。
 */
bool mod_watchdog_WasResetByWatchdog(void);

/**
 * @brief 获取实际生效的名义超时（毫秒），来自 BSP 反算。
 */
uint32_t mod_watchdog_GetTimeoutMs(void);

#ifdef __cplusplus
}
#endif

#endif /* MOD_WATCHDOG_H */
