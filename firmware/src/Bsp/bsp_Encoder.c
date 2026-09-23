#include "bsp_Encoder.h"

#define ENCODER_COUNTER_MID 0x8000U

static volatile uint16_t l_current = 0;
static volatile uint16_t l_last = 0;
static volatile int16_t l_delta = 0;
static volatile int16_t l_direction;
static volatile int16_t l_speed = 0;


void Bsp_Encoder_Init(void)
{
    // 初始化编码器
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    // GPIOA 挂在 APB2，TIM2 挂在 APB1，因此分别开启时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    

    // 配置 GPIO 引脚
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置定时器
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* CH1 + CH2 组成正交编码器接口 */
    TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM2, ENCODER_COUNTER_MID);
    l_last = ENCODER_COUNTER_MID;
    TIM_Cmd(TIM2, ENABLE);
}


int16_t Bsp_Encoder_GetLeftCount(void)
{
    /*
     * TIM2 从 0x8000 开始计数：
     * 正方向使 CNT 增大，反方向使 CNT 减小。
     * 返回相对初始化位置的有符号计数，便于直接观察正负方向。
     */


    return l_delta;
}

int16_t Bsp_Encoder_GetRightCount(void)
{
    // 返回右编码器的计数值
    return 0;
}

uint16_t GetLeftDelta(void)
{
    l_delta = (int16_t)(TIM_GetCounter(TIM2) - l_last);
    l_last = TIM_GetCounter(TIM2);
    return (uint16_t)l_delta;
}
