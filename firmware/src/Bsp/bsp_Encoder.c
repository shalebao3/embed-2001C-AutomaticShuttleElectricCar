#include "bsp_Encoder.h"

#define ENCODER_COUNTER_MID 0x8000U

static uint16_t l_current = 0;
static uint16_t l_last = 0;
static volatile int16_t l_speed = 0;

static uint16_t r_current = 0;
static uint16_t r_last = 0;
static volatile int16_t r_speed = 0;

void Bsp_Encoder_Init(void)
{
    // 初始化编码器
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    // GPIOA 挂在 APB2，TIM2 挂在 APB1，因此分别开启时钟; TIM3 开启
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 GPIO 引脚
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置定时器
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* left: CH1 + CH2 组成left正交编码器接口 */
    TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM2, ENCODER_COUNTER_MID);

    /* right: CH1 + CH2 组成right正交编码器接口 */
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM3, ENCODER_COUNTER_MID);

    l_last = ENCODER_COUNTER_MID;
    r_last = ENCODER_COUNTER_MID;

    /* 启动定时器 */
    TIM_Cmd(TIM2, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

void Bsp_Encoder_UpdateSpeed(void)
{
    // 更新左编码器速度
    l_current = TIM_GetCounter(TIM2);

    l_speed = (int16_t)(l_current - l_last);

    l_last = l_current;

    // 更新右编码器速度
    r_current = TIM_GetCounter(TIM3);

    r_speed = (int16_t)(r_current - r_last);

    r_last = r_current;
}

int16_t Bsp_Encoder_GetLeftSpeed(void)
{
    return l_speed;
}

int16_t Bsp_Encoder_GetRightSpeed(void)
{
    return r_speed;
}

