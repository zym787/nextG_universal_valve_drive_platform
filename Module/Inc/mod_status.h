/**
 * @file    mod_status.h
 * @brief   Module 层统一返回值。
 *          不向上透传 bsp_status_t；由 mod_xxx 内部转译为 mod_status_t。
 */

#ifndef MOD_STATUS_H
#define MOD_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOD_OK              =  0,
    MOD_ERR_PARAM       = -1,
    MOD_ERR_NOT_INIT    = -2,
    MOD_ERR_BUSY        = -3,
    MOD_ERR_TIMEOUT     = -4,
    MOD_ERR_UNSUPPORTED = -5,
    MOD_ERR_IO          = -6,
    MOD_ERR_HW          = -7,
} mod_status_t;

#ifdef __cplusplus
}
#endif

#endif /* MOD_STATUS_H */
