#ifndef APP_SHUTTLE_CAR_H
#define APP_SHUTTLE_CAR_H

#include "stm32f10x.h"

/**
 * @brief 初始化自动往返小车应用所需的板级资源。
 * @return SUCCESS：初始化完成。
 * @note 调用前必须先完成 Com_Time_Init()。
 */
ErrorStatus App_ShuttleCar_Init(void);

/**
 * @brief 自动往返小车非阻塞业务任务。
 * @note 在主循环中持续调用；后续状态机、黑线检测和速度控制均在此编排。
 */
void App_ShuttleCar_Task(void);

#endif /* APP_SHUTTLE_CAR_H */
