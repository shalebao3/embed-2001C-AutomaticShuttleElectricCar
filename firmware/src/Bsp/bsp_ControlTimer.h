#ifndef __BSP_CONTROL_TIMER_H
#define __BSP_CONTROL_TIMER_H

#include "stm32f10x.h"

void Bsp_ControlTimer_Init(void);

/* TIM4 每产生一次 10ms 控制周期时调用 */
void Bsp_ControlTimer_Tick(void);

/* 获取当前控制周期计数 */
uint32_t Bsp_ControlTimer_GetTick(void);

#endif
