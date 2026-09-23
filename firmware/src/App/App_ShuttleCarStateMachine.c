#include "App_ShuttleCarStateMachine.h"

#define APP_SHUTTLE_HIGH_SPEED 50
#define APP_SHUTTLE_LOW_SPEED  30

static App_ShuttleCarState s_state = APP_SHUTTLE_STATE_FORWARD_HIGH;

void App_ShuttleCarStateMachine_Init(void)
{
    s_state = APP_SHUTTLE_STATE_FORWARD_HIGH;
}

ErrorStatus App_ShuttleCarStateMachine_GetCommand(
    App_ShuttleCarCommand *command)
{
    if (command == 0)
    {
        return ERROR;
    }

    switch (s_state)
    {
        case APP_SHUTTLE_STATE_FORWARD_HIGH:
            command->direction = APP_SHUTTLE_DIRECTION_FORWARD;
            command->left_target_speed = APP_SHUTTLE_HIGH_SPEED;
            command->right_target_speed = APP_SHUTTLE_HIGH_SPEED;
            break;

        case APP_SHUTTLE_STATE_FORWARD_LOW:
            command->direction = APP_SHUTTLE_DIRECTION_FORWARD;
            command->left_target_speed = APP_SHUTTLE_LOW_SPEED;
            command->right_target_speed = APP_SHUTTLE_LOW_SPEED;
            break;

        case APP_SHUTTLE_STATE_STOP_WAIT:
            command->direction = APP_SHUTTLE_DIRECTION_STOP;
            command->left_target_speed = 0;
            command->right_target_speed = 0;
            break;

        case APP_SHUTTLE_STATE_REVERSE_HIGH:
            command->direction = APP_SHUTTLE_DIRECTION_REVERSE;
            command->left_target_speed = APP_SHUTTLE_HIGH_SPEED;
            command->right_target_speed = APP_SHUTTLE_HIGH_SPEED;
            break;

        case APP_SHUTTLE_STATE_REVERSE_LOW:
            command->direction = APP_SHUTTLE_DIRECTION_REVERSE;
            command->left_target_speed = APP_SHUTTLE_LOW_SPEED;
            command->right_target_speed = APP_SHUTTLE_LOW_SPEED;
            break;

        default:
            command->direction = APP_SHUTTLE_DIRECTION_STOP;
            command->left_target_speed = 0;
            command->right_target_speed = 0;
            return ERROR;
    }

    return SUCCESS;
}
