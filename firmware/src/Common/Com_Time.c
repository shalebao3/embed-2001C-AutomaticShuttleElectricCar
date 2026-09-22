#include "Com_Time.h"

static volatile uint32_t s_ms_ticks = 0U;

ErrorStatus Com_Time_Init(void)
{
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
    {
        return ERROR;
    }

    return SUCCESS;
}

uint32_t Com_Time_GetMs(void)
{
    return s_ms_ticks;
}

void Com_Time_DelayMs(uint32_t delay_ms)
{
    const uint32_t start_ms = Com_Time_GetMs();

    while ((uint32_t)(Com_Time_GetMs() - start_ms) < delay_ms)
    {
    }
}

void Com_Time_Tick(void)
{
    s_ms_ticks++;
}
