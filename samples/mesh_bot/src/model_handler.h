#pragma once

#include <zephyr/bluetooth/mesh.h>

struct robot_movement_config
{
    uint32_t time;
    int32_t angle;
};

typedef void (*movement_received_handler_t)(struct robot_movement_config *);
typedef void (*start_movement_handler_t)();

const struct bt_mesh_comp *model_handler_init(
    movement_received_handler_t movement_received_handler,
    start_movement_handler_t start_movement_handler);