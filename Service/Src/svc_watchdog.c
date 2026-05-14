/**
 * @file    svc_watchdog.c
 * @brief   看门狗策略：任务存活上报 + 条件喂狗。
 *
 * 设计要点：
 *   1. IWDG 名义超时设为 SVC_WATCHDOG_IWDG_TIMEOUT_MS（默认 2000ms）。
 *   2. 每个任务有"存活窗口"（SVC_WATCHDOG_TASK_WINDOW_MS，默认 500ms）。
 *      若 (now - last_alive_ms) > 窗口，则视为该任务卡死。
 *   3. svc_watchdog_Poll 每次最多以 SVC_WATCHDOG_FEED_PERIOD_MS（默认 100ms）
 *      节流喂狗一次，避免每轮主循环都写 IWDG 寄存器。
 *   4. 默认所有任务"未启用"——这是为了支持渐进迁移：
 *        - 项目初期，Service 还没做完，主循环活着 = 喂狗；
 *        - 任意 Service 真正接入后，调用 EnableTask 把它纳入保护范围。
 *
 * 安全边界：
 *   - 严格不直接 #include "bsp_*.h" / LL/HAL 头；
 *   - 时间统统走 mod_systime；
 *   - 喂狗统统走 mod_watchdog。
 */

#include "svc_watchdog.h"

#include "mod_systime.h"
#include "mod_watchdog.h"

/* ---- 可调参数（必要时迁移到 svc_param 持久化） -------------------------- */
#define SVC_WATCHDOG_IWDG_TIMEOUT_MS  2000U
#define SVC_WATCHDOG_TASK_WINDOW_MS    500U
#define SVC_WATCHDOG_FEED_PERIOD_MS    100U

/* ---- 内部状态 --------------------------------------------------------- */
typedef struct {
    bool         enabled;
    unsigned int last_alive_ms;
} svc_watchdog_task_state_t;

static svc_watchdog_task_state_t g_tasks[SVC_WATCHDOG_TASK_COUNT];
static unsigned int              g_last_feed_ms = 0U;
static bool                      g_initialized  = false;

/* ---- 公开接口 --------------------------------------------------------- */

svc_status_t svc_watchdog_Init(void)
{
    /* 启动底层 IWDG（启用后不可关闭，仅可被复位清除） */
    if (mod_watchdog_Init(SVC_WATCHDOG_IWDG_TIMEOUT_MS) != MOD_OK) {
        return SVC_ERR_HW;
    }

    const unsigned int now = mod_systime_GetMs();
    for (unsigned int i = 0U; i < (unsigned int)SVC_WATCHDOG_TASK_COUNT; ++i) {
        g_tasks[i].enabled       = false;
        g_tasks[i].last_alive_ms = now;
    }
    g_last_feed_ms = now;
    g_initialized  = true;

    /* 立即喂一次，给后续主循环初始化留出时间 */
    (void)mod_watchdog_Feed();
    return SVC_OK;
}

svc_status_t svc_watchdog_EnableTask(svc_watchdog_task_t task)
{
    if (!g_initialized)                  { return SVC_ERR_NOT_INIT; }
    if (task >= SVC_WATCHDOG_TASK_COUNT) { return SVC_ERR_PARAM;    }

    g_tasks[task].enabled       = true;
    g_tasks[task].last_alive_ms = mod_systime_GetMs();
    return SVC_OK;
}

void svc_watchdog_ReportAlive(svc_watchdog_task_t task)
{
    if (!g_initialized)                  { return; }
    if (task >= SVC_WATCHDOG_TASK_COUNT) { return; }

    g_tasks[task].last_alive_ms = mod_systime_GetMs();
}

/* 内部：检查所有"已启用"任务是否都在窗口内 */
static bool all_enabled_tasks_alive(unsigned int now)
{
    for (unsigned int i = 0U; i < (unsigned int)SVC_WATCHDOG_TASK_COUNT; ++i) {
        if (!g_tasks[i].enabled) {
            continue;
        }
        if ((unsigned int)(now - g_tasks[i].last_alive_ms) > SVC_WATCHDOG_TASK_WINDOW_MS) {
            return false;
        }
    }
    return true;
}

void svc_watchdog_Poll(void)
{
    if (!g_initialized) { return; }

    const unsigned int now = mod_systime_GetMs();

    /* 节流：避免每轮主循环都写 IWDG 寄存器 */
    if ((unsigned int)(now - g_last_feed_ms) < SVC_WATCHDOG_FEED_PERIOD_MS) {
        return;
    }

    if (all_enabled_tasks_alive(now)) {
        (void)mod_watchdog_Feed();
        g_last_feed_ms = now;
    }
    /* 否则：本轮拒绝喂狗，IWDG 计数器继续衰减；
       若持续不喂超过 IWDG 名义超时，将触发硬件复位。 */
}

bool svc_watchdog_WasResetByWatchdog(void)
{
    return mod_watchdog_WasResetByWatchdog();
}
