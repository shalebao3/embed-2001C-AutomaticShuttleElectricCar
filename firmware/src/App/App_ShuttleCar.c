#include "App_ShuttleCar.h"
#include "bsp_ControlTimer.h"
#include "bsp_Encoder.h"
#include "bsp_Motor.h"

static uint16_t s_base_duty = 500U;

static int16_t s_left_speed = 0;
static int16_t s_right_speed = 0;

static int16_t s_speed_error = 0;

static int32_t s_correction = 0;

static uint32_t s_last_control_tick = 0U;

#define APP_SPEED_SYNC_KP 1

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

    current_tick = Bsp_ControlTimer_GetTick();

    if (current_tick == s_last_control_tick)
    {
        return;
    }

    s_last_control_tick = current_tick;

    /*
     * 到这里就代表：
     * 新的一个 10ms 控制周期到了。
     */
    s_left_speed = Bsp_Encoder_GetLeftSpeed();
    s_right_speed = Bsp_Encoder_GetRightSpeed();

    /* 计算左右轮速度差 */
    s_speed_error =
        s_left_speed -
        s_right_speed;

    /* P 控制修正量 */
    s_correction =
        APP_SPEED_SYNC_KP *
        s_speed_error;


}
