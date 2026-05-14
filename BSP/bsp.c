/**
 * @file     bsp.c
 * @author   Drinkto
 * @brief    
 * @version  V1.0
 * @date     2026-05-14
 */


#include "bsp.h"
#include "bsp_iwdg.h"
#include "bsp_gpio.h"


void bsp_Init(void)
{
    // Add your initialization code here
    // bsp_iwdg_Init(3000);
    bsp_gpio_Init();
}

