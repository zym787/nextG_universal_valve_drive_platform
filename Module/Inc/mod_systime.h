/**
 * @file    mod_systime.h
 * @brief   系统毫秒时基对 Service 层的暴露接口。
 *          - Service 不能直接 #include "bsp_timer.h"；
 *          - 由 Module 层桥接 BSP 1ms tick，向上提供"无 LL/HAL 类型"的最小 API。
 *          - 适用场景：协议帧间隔判定、运动状态机、看门狗任务窗口、超时等。
 */

#ifndef MOD_SYSTIME_H
#define MOD_SYSTIME_H

#include <stdbool.h>

#include "mod_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化时基模块（清零内部 tick；不重新配置 SysTick）
 */
mod_status_t mod_systime_Init(void);

/**
 * @brief 获取自启动以来的毫秒数（32-bit，约 49.7 天回绕）
 */
unsigned int mod_systime_GetMs(void);

/**
 * @brief 超时判断（自动处理 32-bit 回绕）
 * @param start_ms   起点时间戳
 * @param timeout_ms 超时阈值
 * @retval true      已超时
 */
bool mod_systime_IsTimeout(unsigned int start_ms, unsigned int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* MOD_SYSTIME_H */
