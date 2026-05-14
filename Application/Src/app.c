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

/* 后续可以在这里 #include "svc_xxx.h"
   严禁 #include "mod_xxx.h" / "bsp_xxx.h" / "stm32f1xx_ll_xxx.h" */

/* ========================================================================
 * 公开接口实现
 * ====================================================================== */

/**
 * @brief Application 主循环入口
 * @note  本函数永不返回；main.c 中的 while(1) 仅作链接器兜底。
 *        当前为最小骨架，后续按 Phase 5 计划补全：
 *          - app_Init   : 聚合 svc_system_Init
 *          - app_mode   : Normal / Factory / Aging 模式切换
 *          - app_guardian : 应用级故障保持 + 喂狗策略
 */
void app_MainLoop(void)
{
    /* TODO: app_Init();          (聚合各 service 初始化) */

    for (;;)
    {
        /* TODO: svc_system_Poll();   每轮主循环只调一次 service 层 poll */
        /* TODO: app_mode_Tick();     按当前模式分发任务 */
        /* TODO: app_guardian_Feed(); 满足条件后喂狗 */
    }
}
