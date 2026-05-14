/**
 * @file    mod_indicator.h
 * @brief   板上指示灯（LED）器件抽象。
 *          - 把 BSP 提供的 GPIO 能力，组合成"指示灯 on/off/toggle"概念；
 *          - 上层 Service/Application 不需要知道 LED 接在哪个 GPIO。
 */

#ifndef MOD_INDICATOR_H
#define MOD_INDICATOR_H

#include <stdbool.h>

#include "mod_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 板上指示灯通道枚举（多个时按物理位置追加） */
typedef enum {
    MOD_INDICATOR_WORK = 0,    /* 工作指示灯（PCB 丝印 WORK_LED1） */
    MOD_INDICATOR_COUNT
} mod_indicator_t;

mod_status_t mod_indicator_Init(void);
mod_status_t mod_indicator_Set(mod_indicator_t which, bool on);
mod_status_t mod_indicator_Toggle(mod_indicator_t which);

/**
 * @brief 启动指示灯自动闪烁（心跳）
 * @param which     哪个指示灯
 * @param period_ms 完整一个闪烁周期中的半周期（上或下持续时间）
 */
mod_status_t mod_indicator_StartBlink(mod_indicator_t which, unsigned int period_ms);

/**
 * @brief 主循环周期调用；内部使用 BSP tick 判定是否该翻转闪烁灯。
 */
void mod_indicator_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* MOD_INDICATOR_H */
