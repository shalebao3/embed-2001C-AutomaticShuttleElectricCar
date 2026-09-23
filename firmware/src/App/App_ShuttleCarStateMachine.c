#include "App_ShuttleCarStateMachine.h"

/*
 * 当前阶段使用的两档目标速度。
 *
 * 单位均为：
 * counts / 10ms
 *
 * 真实数值后续需要根据电机、编码器和实车运行效果重新标定。
 */
#define APP_SHUTTLE_HIGH_SPEED 50
#define APP_SHUTTLE_LOW_SPEED  30

/*
 * 当前业务状态。
 *
 * 状态机初始化后默认从高速前进开始。
 * 后续黑线检测接入后，再根据位置事件修改该状态。
 */
static App_ShuttleCarState s_state = APP_SHUTTLE_STATE_FORWARD_HIGH;

/*
 * 初始化自动往返状态机。
 *
 * 每次重新初始化业务流程时，都从高速前进状态开始。
 */
void App_ShuttleCarStateMachine_Init(void)
{
    s_state = APP_SHUTTLE_STATE_FORWARD_HIGH;
}

/*
 * 根据当前业务状态生成运动命令。
 *
 * @param command 输出给 App_ShuttleCar 总编排层的运动命令
 * @return SUCCESS：当前状态有效，command 已填写
 *         ERROR：command 为空或当前状态异常
 *
 * 当前阶段只完成“状态 → 运动命令”的映射：
 *
 * FORWARD_HIGH → 前进 + 高速
 * FORWARD_LOW  → 前进 + 低速
 * STOP_WAIT    → 停止
 * REVERSE_HIGH → 后退 + 高速
 * REVERSE_LOW  → 后退 + 低速
 *
 * 黑线事件、终点事件、等待 10 秒等“状态转移条件”
 * 后续再继续加入，不在这里提前伪造。
 */
ErrorStatus App_ShuttleCarStateMachine_GetCommand(
    App_ShuttleCarCommand *command)
{
    /* 输出地址无效时不能继续写入命令。 */
    if (command == 0)
    {
        return ERROR;
    }

    switch (s_state)
    {
        /*
         * 高速前进：
         * 左右轮都给高速目标值。
         */
        case APP_SHUTTLE_STATE_FORWARD_HIGH:
            command->direction = APP_SHUTTLE_DIRECTION_FORWARD;
            command->left_target_speed = APP_SHUTTLE_HIGH_SPEED;
            command->right_target_speed = APP_SHUTTLE_HIGH_SPEED;
            break;

        /*
         * 低速前进：
         * 用于以后进入 D~E 限速区后的低速运行。
         */
        case APP_SHUTTLE_STATE_FORWARD_LOW:
            command->direction = APP_SHUTTLE_DIRECTION_FORWARD;
            command->left_target_speed = APP_SHUTTLE_LOW_SPEED;
            command->right_target_speed = APP_SHUTTLE_LOW_SPEED;
            break;

        /*
         * 停车等待：
         * 目标速度归零。
         *
         * 10 秒等待计时逻辑后续在状态转移部分实现，
         * 当前这里只负责输出“停车”这个业务命令。
         */
        case APP_SHUTTLE_STATE_STOP_WAIT:
            command->direction = APP_SHUTTLE_DIRECTION_STOP;
            command->left_target_speed = 0;
            command->right_target_speed = 0;
            break;

        /*
         * 高速返回：
         * 保留反向高速状态。
         *
         * 当前 H 桥方向接口尚未实现，
         * App_ShuttleCar 收到 REVERSE 后会先保持安全停车。
         */
        case APP_SHUTTLE_STATE_REVERSE_HIGH:
            command->direction = APP_SHUTTLE_DIRECTION_REVERSE;
            command->left_target_speed = APP_SHUTTLE_HIGH_SPEED;
            command->right_target_speed = APP_SHUTTLE_HIGH_SPEED;
            break;

        /*
         * 低速返回：
         * 用于返程再次经过限速区。
         */
        case APP_SHUTTLE_STATE_REVERSE_LOW:
            command->direction = APP_SHUTTLE_DIRECTION_REVERSE;
            command->left_target_speed = APP_SHUTTLE_LOW_SPEED;
            command->right_target_speed = APP_SHUTTLE_LOW_SPEED;
            break;

        /*
         * 出现非法状态时，先输出停车命令，
         * 再返回 ERROR 交给上层执行安全停车。
         */
        default:
            command->direction = APP_SHUTTLE_DIRECTION_STOP;
            command->left_target_speed = 0;
            command->right_target_speed = 0;
            return ERROR;
    }

    return SUCCESS;
}
