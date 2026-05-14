/**
 * @file    bsp_timer.h
 * @brief   毫秒级 tick / 超时判断接口（裸机调度器友好）。
 *          内部使用 SysTick 1ms 中断累加 tick；上层只感知 32-bit 毫秒计数。
 */

#ifndef BSP_TIME_H
#define BSP_TIME_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  初始化 tick 计数器（清零）；不重新配置 SysTick（CubeMX 已配置 1ms）。
 */
bsp_status_t bsp_timer_Init(void);

/**
 * @brief  获取自启动以来的毫秒数（32-bit，约 49.7 天回绕）
 */
uint32_t bsp_timer_GetTickMs(void);

/**
 * @brief  超时判断（自动处理 32-bit 回绕）
 * @param  start_ms    起点（一般来自 bsp_timer_GetTickMs()）
 * @param  timeout_ms  超时阈值
 * @retval true        已超时
 */
bool bsp_timer_IsTimeout(uint32_t start_ms, uint32_t timeout_ms);

/**
 * @brief  tick 推进（必须每 1ms 调用一次）
 * @note   建议在 SysTick_Handler 的 USER CODE 区调用本函数。
 */
void bsp_timer_TickInc(void);

extern void bsp_timer_mDelay(uint32_t _Delay);

#ifdef __cplusplus
}
#endif

#endif /* BSP_TIME_H */
