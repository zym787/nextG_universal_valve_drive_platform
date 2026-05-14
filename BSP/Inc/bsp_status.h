/**
 * @file    bsp_status.h
 * @brief   BSP 层统一返回值。
 *          所有 bsp_* 对外函数应返回 bsp_status_t，禁止暴露 LL/HAL 错误码。
 */

#ifndef BSP_STATUS_H
#define BSP_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_OK              =  0,
    BSP_ERR_PARAM       = -1,   /* 参数非法（空指针、id 越界、非法频率等）         */
    BSP_ERR_NOT_INIT    = -2,   /* 外设尚未初始化                                  */
    BSP_ERR_BUSY        = -3,   /* 外设忙                                          */
    BSP_ERR_TIMEOUT     = -4,   /* 等待标志位超时                                  */
    BSP_ERR_UNSUPPORTED = -5,   /* 当前板卡未提供该外设/通道                       */
    BSP_ERR_IO          = -6,   /* I/O 失败（NACK、帧错误等）                      */
    BSP_ERR_OVERFLOW    = -7,   /* 缓冲溢出                                        */
    BSP_ERR_HW          = -8,   /* 无法细分的硬件异常                              */
} bsp_status_t;

#ifdef __cplusplus
}
#endif

#endif /* BSP_STATUS_H */
