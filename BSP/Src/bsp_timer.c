/**
 * @file    bsp_timer.c
 * @brief   毫秒 tick 实现。
 *
 * 工作原理：
 *   1. CubeMX 在 main.c 中调用 LL_Init1msTick(72000000)，已把 SysTick 配为 1ms
 * 中断。
 *   2. 由 SysTick_Handler 的 USER CODE 区每 1ms 调用一次 bsp_timer_TickInc()。
 *   3. 本文件维护一个 volatile uint32_t 计数器，对外通过 bsp_timer_GetTickMs()
 * 暴露。
 */

#include "bsp_timer.h"
#include "stm32f1xx_ll_utils.h"

static volatile uint32_t g_tick_ms = 0U;

bsp_status_t bsp_timer_Init(void)
{
        g_tick_ms = 0U;
        return BSP_OK;
}

uint32_t bsp_timer_GetTickMs(void)
{
        /* 32-bit 单字读对 ARMv7-M 是原子的，无需关中断 */
        return g_tick_ms;
}

bool bsp_timer_IsTimeout(uint32_t start_ms, uint32_t timeout_ms)
{
        /* 利用无符号减法天然处理 32-bit 回绕：
           (now - start) >= timeout 时即超时 */
        return ((uint32_t)(bsp_timer_GetTickMs() - start_ms) >= timeout_ms);
}

void bsp_timer_TickInc(void) { g_tick_ms++; }

void bsp_timer_mDelay(uint32_t _Delay) {
        LL_mDelay(_Delay);
}