#include "bsp_Motor.h"

#define MOTOR_PWM_PERIOD 3599U

void Bsp_Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 1. 开启 GPIOA 和 TIM1 时钟 */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1,
        ENABLE);

    /* 2. PA8 / PA9 配置为复用推挽输出 */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; // ((uint16_t)0x0100)，((uint16_t)0x0200)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 3. TIM1 基本计数参数 */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    /* 时基配置：只负责设置最小时基单元 */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_Period = MOTOR_PWM_PERIOD;  // ARR
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // 时钟分频为 1

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* 4. PWM 输出公共配置 */
    TIM_OCStructInit(&TIM_OCInitStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    /* CH1 -> PA8 -> 左电机 */
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    /* CH2 -> PA9 -> 右电机 */
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);

    /* 5. CCR 预装载 */
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM1, ENABLE);

    /* 6. 启动 TIM1 */
    TIM_Cmd(TIM1, ENABLE);

    /*
     * TIM1 是高级定时器。
     * 除了 TIM_Cmd，还必须打开主输出 MOE，
     * 否则 PA8/PA9 不会真正输出 PWM。
     */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

void Bsp_Motor_SetLeftDuty(uint16_t duty)
{
    uint32_t compare;

    if (duty > BSP_MOTOR_DUTY_MAX)
    {
        duty = BSP_MOTOR_DUTY_MAX;
    }

    compare =
        ((uint32_t)(MOTOR_PWM_PERIOD + 1U) * duty) / BSP_MOTOR_DUTY_MAX;

    TIM_SetCompare1(TIM1, compare);
}

void Bsp_Motor_SetRightDuty(uint16_t duty)
{
    uint32_t compare;

    if (duty > BSP_MOTOR_DUTY_MAX)
    {
        duty = BSP_MOTOR_DUTY_MAX;
    }

    compare =
        ((uint32_t)(MOTOR_PWM_PERIOD + 1U) * duty) / BSP_MOTOR_DUTY_MAX;

    TIM_SetCompare2(TIM1, compare);
}

