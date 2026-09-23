#include "App_SpeedController.h"
#include "bsp_Motor.h"

#define APP_SPEED_BASE_DUTY       500U
#define APP_SPEED_SYNC_KP         1
#define APP_SPEED_KP              1
#define APP_SPEED_KI              1
#define APP_SPEED_INTEGRAL_LIMIT  500

typedef struct
{
    int32_t kp;
    int32_t ki;
    int32_t integral;
    int32_t integral_limit;
} App_PIController;

static App_PIController s_left_speed_pi =
{
    APP_SPEED_KP,
    APP_SPEED_KI,
    0,
    APP_SPEED_INTEGRAL_LIMIT
};

static App_PIController s_right_speed_pi =
{
    APP_SPEED_KP,
    APP_SPEED_KI,
    0,
    APP_SPEED_INTEGRAL_LIMIT
};

static int32_t App_SpeedController_PIUpdate(
    App_PIController *controller,
    int32_t error)
{
    controller->integral += error;

    if (controller->integral > controller->integral_limit)
    {
        controller->integral = controller->integral_limit;
    }
    else if (controller->integral < -controller->integral_limit)
    {
        controller->integral = -controller->integral_limit;
    }

    return
        controller->kp * error +
        controller->ki * controller->integral;
}

static uint16_t App_SpeedController_ClampDuty(int32_t duty)
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

void App_SpeedController_Init(void)
{
    App_SpeedController_Reset();
}

void App_SpeedController_Reset(void)
{
    s_left_speed_pi.integral = 0;
    s_right_speed_pi.integral = 0;
}

ErrorStatus App_SpeedController_Update(
    int16_t left_target_speed,
    int16_t right_target_speed,
    int16_t left_speed,
    int16_t right_speed,
    uint16_t *left_duty,
    uint16_t *right_duty)
{
    int32_t left_error;
    int32_t right_error;
    int32_t target_speed_difference;
    int32_t actual_speed_difference;
    int32_t sync_error;
    int32_t sync_correction;
    int32_t left_speed_correction;
    int32_t right_speed_correction;
    int32_t left_output;
    int32_t right_output;

    if ((left_duty == 0) || (right_duty == 0))
    {
        return ERROR;
    }

    if ((left_target_speed == 0) && (right_target_speed == 0))
    {
        App_SpeedController_Reset();
        *left_duty = 0U;
        *right_duty = 0U;
        return SUCCESS;
    }

    left_error =
        (int32_t)left_target_speed -
        (int32_t)left_speed;

    right_error =
        (int32_t)right_target_speed -
        (int32_t)right_speed;

    /*
     * 同步误差不是简单要求左右轮永远同速，而是比较：
     * “实际左右速度差”与“目标左右速度差”是否一致。
     * 当前直行时左右目标相同，因此等价于 left_speed - right_speed。
     */
    target_speed_difference =
        (int32_t)left_target_speed -
        (int32_t)right_target_speed;

    actual_speed_difference =
        (int32_t)left_speed -
        (int32_t)right_speed;

    sync_error =
        actual_speed_difference -
        target_speed_difference;

    sync_correction =
        APP_SPEED_SYNC_KP *
        sync_error;

    left_speed_correction =
        App_SpeedController_PIUpdate(
            &s_left_speed_pi,
            left_error);

    right_speed_correction =
        App_SpeedController_PIUpdate(
            &s_right_speed_pi,
            right_error);

    left_output =
        (int32_t)APP_SPEED_BASE_DUTY +
        left_speed_correction -
        sync_correction;

    right_output =
        (int32_t)APP_SPEED_BASE_DUTY +
        right_speed_correction +
        sync_correction;

    *left_duty =
        App_SpeedController_ClampDuty(left_output);

    *right_duty =
        App_SpeedController_ClampDuty(right_output);

    return SUCCESS;
}
