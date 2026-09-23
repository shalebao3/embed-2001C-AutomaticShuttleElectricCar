#include "App_ShuttleCar.h"
#include "App_ShuttleCarStateMachine.h"
#include "App_SpeedController.h"
#include "bsp_ControlTimer.h"
#include "bsp_Encoder.h"
#include "bsp_Motor.h"

/*
 * 上一次已经处理过的 10ms 控制周期计数。
 *
 * main 中的 App_ShuttleCar_Task() 会被 while(1) 高频调用，
 * 通过比较当前 control_tick 和该变量，保证真正的控制逻辑
 * 每个 TIM4 10ms 周期只执行一次。
 */
static uint32_t s_last_control_tick = 0U;

/*
 * 自动往返小车应用初始化。
 *
 * 初始化顺序：
 * 1. 初始化电机 PWM，并先将左右占空比归零；
 * 2. 初始化左右编码器；
 * 3. 初始化速度控制器和状态机；
 * 4. 最后启动 TIM4 10ms 控制时基。
 *
 * TIM4 放在最后启动，是为了保证第一次中断到来时，
 * 电机、编码器和 App 内部状态都已经准备完成。
 */
ErrorStatus App_ShuttleCar_Init(void)
{
    /* 初始化 TIM1 双路 PWM。 */
    Bsp_Motor_Init();

    /* 初始化阶段先保持左右电机输出为 0，避免上电后直接转动。 */
    Bsp_Motor_SetLeftDuty(0U);
    Bsp_Motor_SetRightDuty(0U);

    /* 初始化 TIM2 / TIM3 编码器模式。 */
    Bsp_Encoder_Init();

    /* 清空左右轮 PI 积分状态。 */
    App_SpeedController_Init();

    /* 状态机默认进入高速前进状态。 */
    App_ShuttleCarStateMachine_Init();

    /* 最后启动 TIM4，每 10ms 产生一次新的控制周期。 */
    Bsp_ControlTimer_Init();

    return SUCCESS;
}

/*
 * 自动往返小车总编排任务。
 *
 * 这个函数本身不实现 PI 算法，也不实现具体状态转移细节。
 * 它只负责把几个 App / BSP 模块组合起来：
 *
 * TIM4 控制周期
 *      ↓
 * 状态机给出运动命令
 *      ↓
 * 读取左右轮实际速度
 *      ↓
 * SpeedController 计算左右 PWM
 *      ↓
 * Bsp_Motor 输出 PWM
 */
void App_ShuttleCar_Task(void)
{
    uint32_t current_tick;

    int16_t left_speed;
    int16_t right_speed;

    uint16_t left_duty;
    uint16_t right_duty;

    App_ShuttleCarCommand command;

    /* 读取 TIM4 已经产生到第几个 10ms 控制周期。 */
    current_tick = Bsp_ControlTimer_GetTick();

    /*
     * 如果 control_tick 没有变化，说明还处于同一个 10ms 周期。
     * 直接 return，不重复执行速度控制。
     */
    if (current_tick == s_last_control_tick)
    {
        return;
    }

    /* 记录本次已经处理过的控制周期。 */
    s_last_control_tick = current_tick;

    /*
     * 获取状态机当前给出的运动命令。
     *
     * 状态机只回答：
     * 1. 当前应该前进 / 后退 / 停止；
     * 2. 左右轮目标速度是多少。
     *
     * 状态机不直接操作 TIM、GPIO、PWM 或编码器。
     */
    if (App_ShuttleCarStateMachine_GetCommand(&command) != SUCCESS)
    {
        /* 状态异常时按安全策略停车，并清空 PI 历史状态。 */
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    /*
     * STOP 状态不再继续执行速度闭环。
     * 直接停车并清空 PI 积分，避免下次重新启动时带入旧积分。
     */
    if (command.direction == APP_SHUTTLE_DIRECTION_STOP)
    {
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    /*
     * 当前 H 桥型号和方向 GPIO 还没有确定。
     * 因此 REVERSE 状态目前只保留业务语义，不伪造真实反转能力。
     *
     * 等后续 Bsp_Motor 增加方向控制接口后，
     * 这里再接入真正的反向执行。
     */
    if (command.direction == APP_SHUTTLE_DIRECTION_REVERSE)
    {
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    /*
     * 读取 TIM4 中断最近一次更新好的左右轮速度。
     *
     * 当前速度单位：
     * counts / 10ms
     */
    left_speed = Bsp_Encoder_GetLeftSpeed();
    right_speed = Bsp_Encoder_GetRightSpeed();

    /*
     * 根据：
     * 1. 状态机给出的左右目标速度；
     * 2. 编码器测得的左右实际速度；
     *
     * 计算下一控制周期需要输出的左右 PWM 占空比。
     */
    if (App_SpeedController_Update(
            command.left_target_speed,
            command.right_target_speed,
            left_speed,
            right_speed,
            &left_duty,
            &right_duty) != SUCCESS)
    {
        /* 控制计算异常时同样按安全策略停车。 */
        App_SpeedController_Reset();
        Bsp_Motor_SetLeftDuty(0U);
        Bsp_Motor_SetRightDuty(0U);
        return;
    }

    /* 将速度控制器最终算出的占空比交给 TIM1 PWM 输出。 */
    Bsp_Motor_SetLeftDuty(left_duty);
    Bsp_Motor_SetRightDuty(right_duty);
}
