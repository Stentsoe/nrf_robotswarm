#pragma once
#include "../mesh_model_defines/robot_movement_cli.h" 
#include "../mesh_model_defines/robot_movement_srv.h"

typedef void (* robot_movement_ctm_status_handler_t)(struct robot_movement_done_status_msg *msg);
typedef void (* robot_movement_ctm_status_handler_t)(struct robot_movement_done_status_msg *msg);

const struct bt_mesh_comp *model_handler_init(void);