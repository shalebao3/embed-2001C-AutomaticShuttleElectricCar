#ifndef APP_SHUTTLE_CAR_H
#define APP_SHUTTLE_CAR_H

#include "stm32f10x.h"

/**
 * @brief 初始化自动往返小车应用及其内部模块。
 * @return SUCCESS：初始化完成。
 * @note 调用前必须先完成 Com_Time_Init()。
 */
ErrorStatus App_ShuttleCar_Init(void);

/**
 * @brief 自动往返小车非阻塞总编排任务。
 * @note 在主循环中持续调用；具体状态机和速度控制算法由独立 App 模块负责。
 */
void App_ShuttleCar_Task(void);

#endif /* APP_SHUTTLE_CAR_H */
