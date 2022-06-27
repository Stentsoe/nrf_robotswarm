#pragma once

#include <zephyr/bluetooth/mesh.h>
#include "../../mesh_model_defines/robot_movement_srv.h"

typedef void (*movement_received_handler_t)(struct robot_movement_set_msg *);
typedef void (*start_movement_handler_t)();

const struct bt_mesh_comp *model_handler_init(
    movement_received_handler_t movement_received_handler,
    start_movement_handler_t start_movement_handler);