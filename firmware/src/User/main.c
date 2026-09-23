#include "main.h"
#include "Com_Time.h"
#include "bsp_Motor.h"

/**
 * @brief 通用工程入口。
 * @note 新题目通常在这里依次加入 App_xxx_Init()，并在 while(1) 中调用 App_xxx_Task()。
 */
int main(void)
{
    SystemCoreClockUpdate();

    if (Com_Time_Init() != SUCCESS)
    {
        Error_Handler();
    }

    Bsp_Motor_Init();

    Bsp_Motor_SetLeftDuty(500);  /* 50% */
    Bsp_Motor_SetRightDuty(750); /* 75% */

    while (1)
    {
        /*
         * 模板默认不绑定任何题目业务。
         * 示例：
         * App_xxx_Task();
         */
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
