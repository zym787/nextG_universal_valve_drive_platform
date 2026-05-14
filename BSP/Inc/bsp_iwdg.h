/**
 * @file     bsp_iwdg.h
 * @author   Drinkto
 * @brief    
 * @version  V1.0
 * @date     2026-05-14
 */

#ifndef __BSP_IWDG_H__
#define __BSP_IWDG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_IWDG

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#if defined(BSP_IWDG)

/* Private types -------------------------------------------------------------*/
/**
 * @brief    IWDG status
 */
typedef enum _ {
    BSP_IWDG_OK     = 0,
    BSP_IWDG_ERR_INVALID_ARG = -1,
    BSP_IWDG_ERR_TIMEOUT     = -2,
    BSP_IWDG_ERR_NOT_READY   = -3,
} bsp_iwdg_status_t;
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/**
  * @}
  */


/* Exported macro ------------------------------------------------------------*/

/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/**
 * @brief 初始化并启动独立看门狗（启用后不可关闭，仅可被复位清除）
 * @param _timeout_ms 期望溢出时间（毫秒），有效范围约 1 ~ 26214
 *                   （基于 LSI≈40kHz、最大分频 256、RL 12bit）
 * @return BSP_IWDG_OK 成功；其他为错误码
 *
 * @note  喂狗周期必须严格小于 timeout_ms，建议留 30%~50% 裕量
 * @note  LSI 误差较大（约 ±10%），timeout_ms 是名义值，不是精确值
 */
bsp_iwdg_status_t bsp_iwdg_Init(uint32_t _timeout_ms);

/**
 * @brief 反初始化并关闭独立看门狗
 */
bsp_iwdg_status_t bsp_iwdg_DeInit(uint32_t _timeout_ms);

/**
 * @brief 喂狗（重载计数器）
 * @note  必须由主循环 / 调度器周期性调用，禁止放在中断里
 */
void bsp_iwdg_Refresh(void);

/**
 * @brief 查询本次启动是否由 IWDG 复位触发
 * @retval true  本次为 IWDG 看门狗复位
 * @retval false 其他原因（上电、外部 NRST、软复位等）
 * @note  应在 bsp_iwdg_Init 之前调用；调用后将清除 RCC CSR 复位标志
 */
bool bsp_iwdg_WasResetByWatchdog(void);

/**
 * @brief 获取实际配置的名义超时（毫秒）
 * @return 内部根据分频/RL 反算得到的名义溢出时间
 */
uint32_t bsp_iwdg_GetTimeoutMs(void);

/**
  * @}
  */


#endif /* BSP_IWDG */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_IWDG_H__ */
