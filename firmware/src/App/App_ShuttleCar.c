#include "App_ShuttleCar.h"
#include "bsp_ControlTimer.h"
#include "bsp_Encoder.h"
#include "bsp_Motor.h"

/* 基础占空比 */
static uint16_t s_base_duty = 500U;

/* 速度修正量 */
static int32_t s_correction = 0;

/* 当前左右轮的速度差 */
static int16_t s_speed_error = 0;

/* 上一次控制周期时间戳 */
static uint32_t s_last_control_tick = 0U;

/* 当前左右轮的目標速度 */
static int16_t s_left_target_speed = 50;
static int16_t s_right_target_speed = 50;

/* 当前左右轮的实际速度 */
static int16_t s_left_speed = 0;
static int16_t s_right_speed = 0;

/* 当前左右轮的速度误差 */
static int16_t s_left_speed_error = 0;
static int16_t s_right_speed_error = 0;

#define APP_SPEED_SYNC_KP 1

/* 左轮 PI 控制器积分累计 */
static int32_t s_left_speed_integral = 0;
/*
 * 左轮速度 PI 控制器输出修正量
/*
 * 计算公式：s_left_speed_correction = APP_LEFT_SPEED_KP * s_left_speed_error + APP_LEFT_SPEED_KI * s_left_speed_integral
 */
static int32_t s_left_speed_correction = 0;

/* 左轮速度 PI 参数，真实值后续结合实车调试 */
#define APP_LEFT_SPEED_KP 1
#define APP_LEFT_SPEED_KI 1

/* 左轮速度 PI 控制器积分限幅，防止积分项累加过大 */
#define APP_LEFT_SPEED_INTEGRAL_LIMIT 500

/*
 * 限幅函数：将占空比限制在 0 到 BSP_MOTOR_DUTY_MAX 之间，防止调速超出约定占空比：0 - BSP_MOTOR_DUTY_MAX
 * 
 * @param duty 待限制的占空比
 * @return 限制后的占空比
*/
static uint16_t App_ShuttleCar_ClampDuty(int32_t duty)
{
    if (duty < 0)
    {
        return 0U;
    }

    if (duty > BSP_MOTOR_DUTY_MAX)
    {
        return BSP_MOTOR_DUTY_MAX;
    }

    return (uint16_t)duty;
}

ErrorStatus App_ShuttleCar_Init(void)
{
    /*
     * 先建立执行器和编码器，再最后启动 10ms 控制时基。
     * 这样 TIM4 开始产生中断时，编码器状态已经准备完成。
     */
    Bsp_Motor_Init();
    Bsp_Motor_SetLeftDuty(0U);
    Bsp_Motor_SetRightDuty(0U);

    Bsp_Encoder_Init();

    Bsp_ControlTimer_Init();

    return SUCCESS;
}

void App_ShuttleCar_Task(void)
{
    uint32_t current_tick;

    int32_t left_duty;

    int32_t right_duty;

    current_tick = Bsp_ControlTimer_GetTick();

    /* TIM4 每 10ms 更新一次控制周期，PSC = 71，ARR = 9999 */
    /* 中断执行在 User/stm32f10x_it.c/TIM4_IRQHandler */
    if (current_tick == s_last_control_tick)
    {
        return;
    }

    s_last_control_tick = current_tick;

    /*
     * 到这里就代表：
     * 新的一个 10ms 控制周期到了。
     */
    /* 获取当前左右轮的实际速度 */
    s_left_speed = Bsp_Encoder_GetLeftSpeed();
    s_right_speed = Bsp_Encoder_GetRightSpeed();

    /* 左右轮与目标速度的误差 */
    s_left_speed_error =
        s_left_target_speed -
        s_left_speed;

    s_right_speed_error =
        s_right_target_speed -
        s_right_speed;

    /* 左右轮速度差，比如左轮速度 20，右轮速度 10，则差值为 10 */
    s_speed_error =
        s_left_speed -
        s_right_speed;

    /* P 控制修正量 */
    s_correction =
        APP_SPEED_SYNC_KP *
        s_speed_error;

    /* 累计左轮速度误差，作为 PI 控制器积分状态 */
    s_left_speed_integral +=
        s_left_speed_error;

    /* 积分限幅，防止积分项累加过大 */
    if (s_left_speed_integral > APP_LEFT_SPEED_INTEGRAL_LIMIT)
    {
        s_left_speed_integral = APP_LEFT_SPEED_INTEGRAL_LIMIT;
    }
    else if (s_left_speed_integral < -APP_LEFT_SPEED_INTEGRAL_LIMIT)
    {
        s_left_speed_integral = -APP_LEFT_SPEED_INTEGRAL_LIMIT;
    }

    /* 计算左轮速度 PI 控制器输出修正量 */
    s_left_speed_correction =
        APP_LEFT_SPEED_KP * s_left_speed_error +
        APP_LEFT_SPEED_KI * s_left_speed_integral;

    /* 设置左右轮电机占空比 */

    /* 限制占空比在合理范围内 */
    left_duty =
        (int32_t)s_base_duty +
        s_left_speed_correction -
        s_correction;

    /* 限制左轮占空比在合理范围内 */ 
    left_duty = App_ShuttleCar_ClampDuty(left_duty);

    /* 限制右轮占空比在合理范围内 */
    right_duty = App_ShuttleCar_ClampDuty(right_duty);

    Bsp_Motor_SetLeftDuty((uint16_t)left_duty);
    Bsp_Motor_SetRightDuty((uint16_t)right_duty);
}
