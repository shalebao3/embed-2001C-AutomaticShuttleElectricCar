#include "main.h"
#include "App_ShuttleCar.h"
#include "Com_Time.h"

/**
 * @brief 程序入口：建立系统时间基准，初始化并持续运行自动往返小车应用。
 */
int main(void)
{
    SystemCoreClockUpdate();

    if (Com_Time_Init() != SUCCESS)
    {
        Error_Handler();
    }

    if (App_ShuttleCar_Init() != SUCCESS)
    {
        Error_Handler();
    }

    while (1)
    {
        App_ShuttleCar_Task();
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
