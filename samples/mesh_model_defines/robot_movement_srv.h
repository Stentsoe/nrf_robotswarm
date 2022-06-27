#pragma once
#include <zephyr/bluetooth/mesh/access.h>

#define ROBOT_MOVEMENT_SRV_MODEL_ID 0x0001

#define OP_VND_ROBOT_MOVEMENT_GET  BT_MESH_MODEL_OP_3(0x00, CONFIG_BT_COMPANY_ID)
#define OP_VND_ROBOT_MOVEMENT_SET  BT_MESH_MODEL_OP_3(0x01, CONFIG_BT_COMPANY_ID)
#define OP_VND_ROBOT_CLEAR_TO_MOVE BT_MESH_MODEL_OP_3(0x02, CONFIG_BT_COMPANY_ID)

struct robot_movement_set_msg {
    uint32_t time;
    int32_t angle;
};