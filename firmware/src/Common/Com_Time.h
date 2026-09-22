#ifndef COM_TIME_H
#define COM_TIME_H

#include <stdint.h>
#include "stm32f10x.h"

/**
 * @brief 以当前 SystemCoreClock 配置 1ms SysTick 时间基准。
 */
ErrorStatus Com_Time_Init(void);

/**
 * @brief 获取累计毫秒数。判断时间间隔应使用无符号差值，以允许自然回绕。
 */
uint32_t Com_Time_GetMs(void);

/**
 * @brief 阻塞毫秒延时。
 * @note 不可在中断或关闭中断的临界区调用，也不能代替精确微秒延时。
 */
void Com_Time_DelayMs(uint32_t delay_ms);

/**
 * @brief 每次 SysTick 中断调用一次，仅供中断入口转发。
 */
void Com_Time_Tick(void);

#endif /* COM_TIME_H */
