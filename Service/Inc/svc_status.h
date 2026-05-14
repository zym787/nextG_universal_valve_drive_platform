/**
 * @file    svc_status.h
 * @brief   Service 层统一返回值。
 */

#ifndef SVC_STATUS_H
#define SVC_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SVC_OK              =  0,
    SVC_ERR_PARAM       = -1,
    SVC_ERR_NOT_INIT    = -2,
    SVC_ERR_BUSY        = -3,
    SVC_ERR_TIMEOUT     = -4,
    SVC_ERR_HW          = -5,    /* 底层硬件异常（来自 mod/bsp） */
    SVC_ERR_CONFIG      = -6,    /* 参数加载/校验失败          */
    SVC_ERR_INTERNAL    = -7,
} svc_status_t;

#ifdef __cplusplus
}
#endif

#endif /* SVC_STATUS_H */
