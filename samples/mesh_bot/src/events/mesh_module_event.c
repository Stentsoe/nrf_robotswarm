
#include "mesh_module_event.h"

static char *type_to_str(mesh_module_event_type type)
{
    switch (type)
    {
    case MESH_EVT_MOVEMENT_RECEIVED:
        return "MOVEMENT_RECEIVED";
    default:
        return "UNKNOWN";
    }
}

static void log_mesh_event(const struct app_event_header *header)
{
    struct mesh_module_event *evt = cast_mesh_module_event(header);
    char *type_str = type_to_str(evt->type);

    APP_EVENT_MANAGER_LOG(header, "Type: %s", type_str);
}

APP_EVENT_TYPE_DEFINE(
    mesh_module_event,
    log_mesh_event,
    NULL,
    APP_EVENT_FLAGS_CREATE());