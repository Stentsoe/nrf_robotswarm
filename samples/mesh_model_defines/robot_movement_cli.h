#pragma once
#include <zephyr/bluetooth/mesh/access.h>

#define ROBOT_MOVEMENT_CLI_MODEL_ID 0x0002

#define OP_VND_ROBOT_MOVEMENT_SET_STATUS BT_MESH_MODEL_OP_3(0x03, CONFIG_BT_COMPANY_ID)
#define OP_VND_ROBOT_MOVEMENT_DONE_STATUS BT_MESH_MODEL_OP_3(0x04, CONFIG_BT_COMPANY_ID)

/**
 * @brief ACK/Status message for movement set operation
 */
struct robot_movement_set_status_msg
{
    uint8_t status;

};

/**
 * @brief ACK/Status message for clear to move operation
 */
struct robot_movement_done_status_msg
{
    int16_t motor_a_rot;
    int16_t motor_b_rot;
    struct imu_data
    {
        int16_t rotation;
        struct translation
        {
            int16_t x;
            int16_t y;
            int16_t z;
        } local_trans;
    } imu;
};