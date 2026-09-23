#include "bsp_ControlTimer.h"

void Bsp_ControlTimer_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* TIM4 挂在 APB1 */
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_TIM4,
        ENABLE);

    /*
     * TIM4 clock = 72 MHz
     *
     * PSC = 71
     * CNT clock = 72 MHz / 72 = 1 MHz
     *
     * ARR = 9999
     * Update period = 10000 / 1 MHz = 10 ms
     */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_Period = 9999;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    /* 清除可能存在的旧 Update 标志 */
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

    /* Update Event 产生中断 */
    TIM_ITConfig(
        TIM4,
        TIM_IT_Update,
        ENABLE);

    /* NVIC 允许 CPU 响应 TIM4 中断 */
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);

    /* 启动 TIM4 */
    TIM_Cmd(TIM4, ENABLE);
}