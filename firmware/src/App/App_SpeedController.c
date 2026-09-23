#include "App_SpeedController.h"
#include "bsp_Motor.h"

/*
 * 基础占空比。
 *
 * 当前使用 500，即 50% PWM 作为速度控制器的基础输出。
 * 左右轮 PI 和同步 P 都是在这个基础值上继续做修正。
 */
#define APP_SPEED_BASE_DUTY       500U

/*
 * 左右轮同步 P 控制比例系数。
 *
 * 同步环解决的是：
 * “左右轮实际速度差”是否符合“左右轮目标速度差”。
 */
#define APP_SPEED_SYNC_KP         1

/*
 * 左右轮独立速度 PI 参数。
 *
 * 当前都是软件验证初值，
 * 真实参数需要等电机、编码器和底盘接好后再实车调试。
 */
#define APP_SPEED_KP              1
#define APP_SPEED_KI              1

/*
 * PI 积分累计限幅。
 *
 * 防止电机堵转或长时间达不到目标速度时，
 * integral 一直累加形成积分饱和。
 */
#define APP_SPEED_INTEGRAL_LIMIT  500

/*
 * 单个 PI 控制器需要保存的参数和内部状态。
 *
 * kp             → 比例系数
 * ki             → 积分系数
 * integral       → 历史误差累计，也就是 PI 的“记忆”
 * integral_limit → 积分正负限幅
 */
typedef struct
{
    int32_t kp;
    int32_t ki;
    int32_t integral;
    int32_t integral_limit;
} App_PIController;

/*
 * 左轮速度 PI 控制器。
 *
 * 左右轮虽然使用同一套 PI 算法，
 * 但 integral 必须分别保存，因此需要两个独立对象。
 */
static App_PIController s_left_speed_pi =
{
    APP_SPEED_KP,
    APP_SPEED_KI,
    0,
    APP_SPEED_INTEGRAL_LIMIT
};

/* 右轮速度 PI 控制器。 */
static App_PIController s_right_speed_pi =
{
    APP_SPEED_KP,
    APP_SPEED_KI,
    0,
    APP_SPEED_INTEGRAL_LIMIT
};

/*
 * 更新一次 PI 控制器。
 *
 * @param controller 当前需要更新的 PI 控制器结构体指针
 * @param error 当前一次速度误差，error = target_speed - actual_speed
 * @return 当前一次 PI 输出修正量
 *
 * 离散 PI：
 *
 * integral[k] = integral[k - 1] + error[k]
 *
 * output =
 *     Kp * error
 *     +
 *     Ki * integral
 *
 * 当前控制周期固定为 10ms，因此这里先把采样周期 Ts
 * 视为已经吸收到离散 Ki 参数中。
 */
static int32_t App_SpeedController_PIUpdate(
    App_PIController *controller,
    int32_t error)
{
    /* 累计当前一次速度误差。 */
    controller->integral += error;

    /*
     * 积分正向限幅。
     *
     * 如果长时间 error > 0，integral 会不断增加，
     * 因此必须限制最大值。
     */
    if (controller->integral > controller->integral_limit)
    {
        controller->integral = controller->integral_limit;
    }
    /*
     * 积分反向限幅。
     *
     * error < 0 时积分也可能不断向负方向累计，
     * 所以负方向同样需要限幅。
     */
    else if (controller->integral < -controller->integral_limit)
    {
        controller->integral = -controller->integral_limit;
    }

    /* PI 输出 = P 项 + I 项。 */
    return
        controller->kp * error +
        controller->ki * controller->integral;
}

/*
 * 将最终电机占空比限制到 BSP 约定的合法范围。
 *
 * @param duty PI + 同步修正后得到的原始占空比
 * @return 0 ~ BSP_MOTOR_DUTY_MAX 范围内的占空比
 *
 * 这里使用 int32_t 接收，是因为控制计算过程中
 * duty 可能暂时出现负数，不能提前使用无符号类型。
 */
static uint16_t App_SpeedController_ClampDuty(int32_t duty)
{
    if (duty < 0)
    {
        return 0U;
    }

    if (duty > BSP_MOTOR_DUTY_MAX)
    {
        return BSP_MOTOR_DUTY_MAX;
    }

    return (uint16_t)duty;
}

/*
 * 初始化速度控制器。
 *
 * 当前 PI 参数使用静态初始化，
 * 因此这里只需要把运行历史状态清零。
 */
void App_SpeedController_Init(void)
{
    App_SpeedController_Reset();
}

/*
 * 重置左右轮速度 PI 控制器。
 *
 * 停车、换向、控制模式发生变化时调用，
 * 避免旧 integral 带入新的运行状态。
 */
void App_SpeedController_Reset(void)
{
    s_left_speed_pi.integral = 0;
    s_right_speed_pi.integral = 0;
}

/*
 * 根据左右轮目标速度和实际速度，计算左右轮最终 PWM 占空比。
 *
 * @param left_target_speed  左轮目标速度，单位 counts / 10ms
 * @param right_target_speed 右轮目标速度，单位 counts / 10ms
 * @param left_speed         左轮实际速度，单位 counts / 10ms
 * @param right_speed        右轮实际速度，单位 counts / 10ms
 * @param left_duty          输出左轮占空比
 * @param right_duty         输出右轮占空比
 * @return SUCCESS：计算成功
 *         ERROR：输出指针为空
 *
 * 控制结构：
 *
 * 左轮：
 * BaseDuty + PI_Left - SyncCorrection
 *
 * 右轮：
 * BaseDuty + PI_Right + SyncCorrection
 *
 * 其中：
 * PI_Left / PI_Right
 *     → 每个轮子各自追自己的目标速度
 *
 * SyncCorrection
 *     → 修正左右轮相对速度关系
 */
ErrorStatus App_SpeedController_Update(
    int16_t left_target_speed,
    int16_t right_target_speed,
    int16_t left_speed,
    int16_t right_speed,
    uint16_t *left_duty,
    uint16_t *right_duty)
{
    int32_t left_error;
    int32_t right_error;

    int32_t target_speed_difference;
    int32_t actual_speed_difference;

    int32_t sync_error;
    int32_t sync_correction;

    int32_t left_speed_correction;
    int32_t right_speed_correction;

    int32_t left_output;
    int32_t right_output;

    /* 输出地址无效时不能继续写入结果。 */
    if ((left_duty == 0) || (right_duty == 0))
    {
        return ERROR;
    }

    /*
     * 两侧目标速度都为 0，说明上层要求停车。
     *
     * 停车时：
     * 1. 清空 PI 历史积分；
     * 2. 左右 PWM 直接输出 0；
     * 3. 不继续执行后面的 PI 计算。
     */
    if ((left_target_speed == 0) && (right_target_speed == 0))
    {
        App_SpeedController_Reset();
        *left_duty = 0U;
        *right_duty = 0U;
        return SUCCESS;
    }

    /*
     * 左右轮各自的绝对速度误差。
     *
     * error > 0 → 实际速度低于目标，需要增加控制量
     * error < 0 → 实际速度高于目标，需要减少控制量
     */
    left_error =
        (int32_t)left_target_speed -
        (int32_t)left_speed;

    right_error =
        (int32_t)right_target_speed -
        (int32_t)right_speed;

    /*
     * 计算“目标左右速度差”和“实际左右速度差”。
     *
     * 不能简单永远要求 left_speed == right_speed，
     * 因为以后转弯时左右目标速度本来就可能不同。
     */
    target_speed_difference =
        (int32_t)left_target_speed -
        (int32_t)right_target_speed;

    actual_speed_difference =
        (int32_t)left_speed -
        (int32_t)right_speed;

    /*
     * 同步误差：
     *
     * sync_error =
     *     actual_speed_difference
     *     -
     *     target_speed_difference
     *
     * 当前直行时左右目标速度相同，
     * target_speed_difference = 0，
     * 所以等价于 left_speed - right_speed。
     */
    sync_error =
        actual_speed_difference -
        target_speed_difference;

    /* 同步环当前先使用简单 P 控制。 */
    sync_correction =
        APP_SPEED_SYNC_KP *
        sync_error;

    /* 左右轮分别使用自己的 PI 控制器追目标速度。 */
    left_speed_correction =
        App_SpeedController_PIUpdate(
            &s_left_speed_pi,
            left_error);

    right_speed_correction =
        App_SpeedController_PIUpdate(
            &s_right_speed_pi,
            right_error);

    /*
     * 合成最终控制量。
     *
     * 如果左轮相对过快：
     * sync_error > 0
     *
     * 则：
     * 左轮减去 sync_correction
     * 右轮加上 sync_correction
     *
     * 从而形成负反馈同步纠偏。
     */
    left_output =
        (int32_t)APP_SPEED_BASE_DUTY +
        left_speed_correction -
        sync_correction;

    right_output =
        (int32_t)APP_SPEED_BASE_DUTY +
        right_speed_correction +
        sync_correction;

    /* 最终输出前统一限制到 0 ~ BSP_MOTOR_DUTY_MAX。 */
    *left_duty =
        App_SpeedController_ClampDuty(left_output);

    *right_duty =
        App_SpeedController_ClampDuty(right_output);

    return SUCCESS;
}
