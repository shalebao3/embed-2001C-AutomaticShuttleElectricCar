#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include "stm32f10x.h"

/* 对外统一使用 0~1000 表示 0~100.0% 占空比 */
#define BSP_MOTOR_DUTY_MAX 1000U

void Bsp_Motor_Init(void);

/* 速度*/
void Bsp_Motor_SetLeftDuty(uint16_t duty);
void Bsp_Motor_SetRightDuty(uint16_t duty);

#endif
