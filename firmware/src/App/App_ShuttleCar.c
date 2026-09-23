#include "App_ShuttleCar.h"
#include "App_ShuttleCarStateMachine.h"
#include "App_SpeedController.h"
#include "bsp_ControlTimer.h"
#include "bsp_Encoder.h"
#include "bsp_Motor.h"

static uint32_t s_last_control_tick = 0U;

ErrorStatus App_ShuttleCar_Init(void)
{
    /*
     * 先建立执行器和编码器，再初始化 App 内部控制模块，
     * 最后启动 10ms 控制时基。
     */
    Bsp_Motor_Init();
    Bsp_Motor_SetLeftDuty(0U);
    Bsp_Motor_SetRightDuty(0U);

    Bsp_Encoder_Init();

    App_SpeedController_Init();
    App_ShuttleCarStateMachine_Init();

    Bsp_ControlTimer_Init();

    return SUCCESS;
}

void App_ShuttleCar_Task(void)
{
    uint32_t current_tick;
    int16_t left_speed;
    int16_t right_speed;
    uint16_t left_duty;
    uint16_t right_duty;
    App_ShuttleCarCommand command;

    current_tick = Bsp_ControlTimer_GetTick();

    /*
     * TIM4 每 10ms 产生一个新的控制周期。
     * 没有新周期时立即返回，使 main 可以继续执行其他非阻塞任务。
     */
    if (current_tick == s_last_control_tick)
    {
        return;
    }

    s_last_control_tick = current_tick;

    /*
     * 状态机只回答“当前应该怎么运动”，
     * 不直接操作 TIM、GPIO 或编码器。
     */
    if (App_ShuttleCarStateMachine_GetCommand(&command) != SUCCESS)
    {
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    if (command.direction == APP_SHUTTLE_DIRECTION_STOP)
    {
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    /*
     * H 桥方向接口尚未确定。
     * 在真实反转能力接入 BSP 之前，反向命令保持安全停车，
     * 不伪造反转逻辑。
     */
    if (command.direction == APP_SHUTTLE_DIRECTION_REVERSE)
    {
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    left_speed = Bsp_Encoder_GetLeftSpeed();
    right_speed = Bsp_Encoder_GetRightSpeed();

    if (App_SpeedController_Update(
            command.left_target_speed,
            command.right_target_speed,
            left_speed,
            right_speed,
            &left_duty,
            &right_duty) != SUCCESS)
    {
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    Bsp_Motor_SetLeftDuty(left_duty);
    Bsp_Motor_SetRightDuty(right_duty);
}
