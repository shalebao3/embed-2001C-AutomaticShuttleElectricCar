#include "App_ShuttleCar.h"
#include "bsp_ControlTimer.h"
#include "bsp_Encoder.h"
#include "bsp_Motor.h"

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
    /*
     * 当前阶段先保持非阻塞空任务。
     * 后续在这里编排：
     * 1. 黑线/位置事件；
     * 2. 自动往返状态机；
     * 3. 目标速度和电机控制。
     *
     * 10ms 编码器测速由 TIM4 中断负责更新，不在这里依赖主循环频率。
     */
}
