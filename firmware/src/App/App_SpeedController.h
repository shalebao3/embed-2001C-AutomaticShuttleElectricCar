#ifndef APP_SPEEDCONTROLLER_H
#define APP_SPEEDCONTROLLER_H

#include <stdint.h>
#include "stm32f10x.h"

/**
 * @brief 初始化双轮速度控制器状态。
 */
void App_SpeedController_Init(void);

/**
 * @brief 清空左右轮 PI 的积分历史。
 * @note 停车、换向、控制模式切换时调用，避免旧积分继续影响新状态。
 */
void App_SpeedController_Reset(void);

/**
 * @brief 根据目标速度和编码器反馈计算左右轮 PWM 占空比。
 * @param left_target_speed 左轮目标速度，单位 counts / 10ms。
 * @param right_target_speed 右轮目标速度，单位 counts / 10ms。
 * @param left_speed 左轮实际速度，单位 counts / 10ms。
 * @param right_speed 右轮实际速度，单位 counts / 10ms。
 * @param left_duty 输出左轮占空比，范围 0~BSP_MOTOR_DUTY_MAX。
 * @param right_duty 输出右轮占空比，范围 0~BSP_MOTOR_DUTY_MAX。
 * @return SUCCESS：计算完成；ERROR：输出指针无效。
 */
ErrorStatus App_SpeedController_Update(
    int16_t left_target_speed,
    int16_t right_target_speed,
    int16_t left_speed,
    int16_t right_speed,
    uint16_t *left_duty,
    uint16_t *right_duty);

#endif /* APP_SPEEDCONTROLLER_H */
