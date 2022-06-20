#pragma once

#include <app_event_manager.h>

typedef enum {
    MESH_EVT_PROVISIONED,
    MESH_EVT_DISCONNECTED,
    MESH_EVT_MOVEMENT_RECEIVED,
    MESH_EVT_CLEAR_TO_MOVE_RECEIVED,
} mesh_module_event_type;

struct mesh_module_event {
    struct app_event_header header;
    mesh_module_event_type type;
};

APP_EVENT_TYPE_DECLARE(mesh_module_event);