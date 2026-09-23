#include "stm32f10x_it.h"
#include "Com_Time.h"
#include "bsp_Encoder.h"
#include "bsp_ControlTimer.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    Com_Time_Tick();
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        
        /* 每 10ms 更新一次左右轮速度 */
        Bsp_Encoder_UpdateSpeed();

        /* 记录一个新的 10ms 控制周期 */
        Bsp_ControlTimer_Tick();
    }
}
