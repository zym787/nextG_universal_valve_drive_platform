/**
 * @file    svc_system.h
 * @brief   Service 层"系统能力"聚合入口。
 *          - svc_system_Init  : 串联 BSP/Module 初始化（按 bsp_development_guide 11 节）
 *          - svc_system_Poll  : 主循环每轮调用一次，分发心跳/Service 周期任务
 */

#ifndef SVC_SYSTEM_H
#define SVC_SYSTEM_H

#include "svc_status.h"

#ifdef __cplusplus
extern "C" {
#endif

svc_status_t svc_system_Init(void);
void         svc_system_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* SVC_SYSTEM_H */
