/**
 * @file    svc_watchdog.h
 * @brief   Service 层"看门狗策略"服务。
 *
 * 架构定位：
 *   - 不直接操作 IWDG 寄存器（由 mod_watchdog -> bsp_iwdg 完成）；
 *   - 不暴露任何 LL/HAL 类型；
 *   - 实现"任务存活上报 + 条件喂狗"：仅当所有已启用的关键任务都在
 *     窗口内上报过存活时才喂狗，否则让 IWDG 触发系统复位。
 *
 * 调用关系：
 *   各 svc_xxx_Poll  ->  svc_watchdog_ReportAlive(task)
 *   app_MainLoop -> svc_system_Poll -> svc_watchdog_Poll -> mod_watchdog_Feed
 */

#ifndef SVC_WATCHDOG_H
#define SVC_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#include "svc_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 受看门狗保护的关键任务枚举。
 * @note  新增任务后只需扩枚举 + 在对应 svc_xxx_Poll 中调用 ReportAlive。
 */
typedef enum {
    SVC_WATCHDOG_TASK_COMM   = 0,   /* 通信/协议任务 */
    SVC_WATCHDOG_TASK_MOTION,       /* 运动状态机    */
    SVC_WATCHDOG_TASK_SAFETY,       /* 安全/急停     */
    SVC_WATCHDOG_TASK_COUNT
} svc_watchdog_task_t;

/**
 * @brief 初始化看门狗策略并启动 IWDG。
 * @note  启动 IWDG 后不可关闭；svc_watchdog_Poll 必须在主循环周期调用。
 *        默认所有任务"未启用"，调用 svc_watchdog_EnableTask 后才纳入存活检查。
 */
svc_status_t svc_watchdog_Init(void);

/**
 * @brief 启用一个任务的存活检查（用于该 Service 真正落地后挂入保护范围）。
 * @param task  要启用的任务。
 */
svc_status_t svc_watchdog_EnableTask(svc_watchdog_task_t task);

/**
 * @brief 任务在自己的 Poll 中调用，证明本周期仍然存活。
 * @note  非阻塞、可中断外调用（中断里上报无意义，仅在线程上下文调用）。
 */
void svc_watchdog_ReportAlive(svc_watchdog_task_t task);

/**
 * @brief 看门狗调度入口；周期性调用即可（建议 ≤100ms 一次）。
 *        - 当所有"已启用任务"都在窗口内上报存活，调用 mod_watchdog_Feed；
 *        - 任一任务超窗：本轮拒绝喂狗，由 IWDG 触发硬件复位。
 */
void svc_watchdog_Poll(void);

/**
 * @brief 查询是否上次启动由 IWDG 复位（应在 Init 前调用一次）
 */
bool svc_watchdog_WasResetByWatchdog(void);

#ifdef __cplusplus
}
#endif

#endif /* SVC_WATCHDOG_H */
