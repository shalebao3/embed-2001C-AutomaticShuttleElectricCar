#ifndef APP_SHUTTLECAR_STATEMACHINE_H
#define APP_SHUTTLECAR_STATEMACHINE_H

#include <stdint.h>
#include "stm32f10x.h"

/**
 * @brief 自动往返小车业务状态。
 * @note 反向状态先保留业务语义；H 桥方向接口确定后再接入真实反转执行。
 */
typedef enum
{
    APP_SHUTTLE_STATE_FORWARD_HIGH = 0,
    APP_SHUTTLE_STATE_FORWARD_LOW,
    APP_SHUTTLE_STATE_STOP_WAIT,
    APP_SHUTTLE_STATE_REVERSE_HIGH,
    APP_SHUTTLE_STATE_REVERSE_LOW
} App_ShuttleCarState;

/**
 * @brief 状态机给总编排层的运动方向命令。
 */
typedef enum
{
    APP_SHUTTLE_DIRECTION_STOP = 0,
    APP_SHUTTLE_DIRECTION_FORWARD,
    APP_SHUTTLE_DIRECTION_REVERSE
} App_ShuttleCarDirection;

/**
 * @brief 状态机输出的运动目标。
 * @note 目标速度当前单位为 counts / 10ms，使用正值表示速度大小；
 *       实际前进/后退方向由 direction 单独表达。
 */
typedef struct
{
    App_ShuttleCarDirection direction;
    int16_t left_target_speed;
    int16_t right_target_speed;
} App_ShuttleCarCommand;

/**
 * @brief 初始化自动往返状态机。
 */
void App_ShuttleCarStateMachine_Init(void);

/**
 * @brief 获取当前业务状态对应的运动命令。
 * @param command 输出运动命令。
 * @return SUCCESS：命令有效；ERROR：空指针或状态异常。
 * @note 当前只建立状态与输出映射；黑线事件和状态转移条件后续接入。
 */
ErrorStatus App_ShuttleCarStateMachine_GetCommand(
    App_ShuttleCarCommand *command);

#endif /* APP_SHUTTLECAR_STATEMACHINE_H */
