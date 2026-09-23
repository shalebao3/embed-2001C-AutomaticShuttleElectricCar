#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#include "stm32f10x.h"

void Bsp_Encoder_Init(void);

int16_t Bsp_Encoder_GetLeftCount(void);

int16_t Bsp_Encoder_GetRightCount(void);

#endif
