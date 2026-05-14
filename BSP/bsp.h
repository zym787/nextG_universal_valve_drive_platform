/**
 * @file     bsp.h
 * @author   Drinkto
 * @brief    
 * @version  V1.0
 * @date     2026-05-14
 */

#ifndef __BSP_H__
#define __BSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BSP

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#if defined(BSP)

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/


/* Private macros ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/


/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
extern void bsp_Init(void);


#endif /* BSP */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
