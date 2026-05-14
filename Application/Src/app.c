/**
 * @file     app.c
 * @author   Drinkto
 * @brief    Application 层入口。
 *           main.c 在 USER CODE BEGIN 2 区域调用 app_MainLoop()，
 *           由本文件接管整个用户主循环。
 * @version  V1.0
 * @date     2026-05-14
 */

#include "app.h"

#ifdef APP_ENABLE_BRINGUP
#include "bsp.h"
#include "bsp_gpio.h"
#include "bsp_iwdg.h"
#include "bsp_timer.h"

#endif

// #include "svc_system.h"

/* 严禁 #include "mod_xxx.h" / "bsp_xxx.h" / "stm32f1xx_ll_xxx.h" */

static uint32_t ticks;

/* ========================================================================
 * 公开接口实现
 * ====================================================================== */

/**
 * @brief Application 主循环入口
 * @note  本函数永不返回；main.c 中的 while(1) 仅作链接器兜底。
 *        当前为最小骨架（LED 心跳示例），后续按 Phase 5
 * 计划补全：两位进样切换阀（两两相通）
 *          - app_Init     : 聚合 svc_system_Init 等
 *          - app_mode     : Normal / Factory / Aging 模式切换
 *          - app_guardian : 应用级故障保持 + 喂狗策略
 */
void app_MainLoop(void)
{
        /* 系统级初始化：含 WORK LED 心跳启动（500ms 周期，1Hz） */
#ifndef APP_ENABLE_BRINGUP
        // (void)svc_system_Init();
#else
        bsp_Init();
#endif

        /* 主循环 永远卡在这个循环 */
#ifndef APP_ENABLE_BRINGUP
        for (;;) {
                // svc_system_Poll(); /* 心跳翻转 + 后续 service 周期任务 */
                /* TODO: app_mode_Tick();      按当前模式分发任务 */
                /* TODO: app_guardian_Feed();  满足条件后喂狗 */
        }
#else
        for (;;) {
                ticks++;
                bsp_gpio_Toggle(0);
                bsp_timer_mDelay(1000);
                // bsp_iwdg_Refresh();
        }
#endif
}
