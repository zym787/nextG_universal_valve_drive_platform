/**
 * @file    svc_system.c
 * @brief   Service 层"系统能力"聚合 + 心跳调度。
 *
 * 调用关系（严格遵守单向依赖）：
 *   app_MainLoop
 *     -> svc_system_Init / svc_system_Poll      (本文件)
 *        -> svc_watchdog_Init / Poll            (Service)
 *           -> mod_watchdog_*                    (Module)
 *              -> bsp_iwdg_*                     (BSP)
 *                 -> LL_IWDG_*                   (CubeMX/LL)
 *        -> mod_indicator_StartBlink / Poll      (Module 心跳)
 *           -> bsp_gpio_Toggle / bsp_timer_*      (BSP)
 *              -> LL_GPIO_TogglePin / SysTick    (CubeMX/LL)
 *
 * 注意：本文件可访问 mod_xxx.h，但禁止 #include "bsp_xxx.h" / LL/HAL 头。
 *       心跳节拍逻辑被下沉到 Module 层（mod_indicator_Poll），
 *       Service 不再直接读取 BSP tick。
 */

#include "svc_system.h"

#include "mod_indicator.h"
#include "svc_watchdog.h"

#define HEARTBEAT_PERIOD_MS  500U   /* WORK 灯半周期 = 500ms => 1Hz 闪烁 */

svc_status_t svc_system_Init(void)
{
    /* 1) 看门狗策略优先启动，其后的 Init 均在 IWDG 保护下运行 */
    if (svc_watchdog_Init() != SVC_OK) {
        return SVC_ERR_HW;
    }

    /* 2) 后续可在这里依次串联：mod_eeprom_Init -> svc_param_Load -> ... */
    if (mod_indicator_Init() != MOD_OK) {
        return SVC_ERR_HW;
    }

    if (mod_indicator_StartBlink(MOD_INDICATOR_WORK, HEARTBEAT_PERIOD_MS) != MOD_OK) {
        return SVC_ERR_HW;
    }
    return SVC_OK;
}

void svc_system_Poll(void)
{
    /* 让 Module 层根据自身节拍翻转闪烁灯，Service 只做调度 */
    mod_indicator_Poll();

    /* 看门狗调度：默认无启用任务 → 节流阅狗；
       当 svc_protocol/svc_motion/svc_safety 落地后，调用 svc_watchdog_EnableTask
       + 在各自的 Poll 内 ReportAlive，即可进入“条件阅狗”状态。 */
    svc_watchdog_Poll();

    /* 后续在这里追加：svc_protocol_Poll / svc_motion_Poll / svc_safety_Poll 等
       每个 Poll 需在自己函数体内调用 svc_watchdog_ReportAlive(<对应枚举>) */
}
