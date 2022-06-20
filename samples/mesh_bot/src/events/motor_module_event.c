
#include "motor_module_event.h"

static char *type_to_str(motor_module_event_type type)
{
    switch (type)
    {
    case MOTOR_EVT_MOVEMENT_START:
        return "MOTOR_EVT_MOVEMENT_START";
    case MOTOR_EVT_MOVEMENT_DONE:
        return "MOTOR_EVT_MOVEMENT_DONE";
    default:
        return "UNKNOWN";
    }
}

static void log_motor_event(const struct app_event_header *header)
{
    struct motor_module_event *evt = cast_motor_module_event(header);
    char *type_str = type_to_str(evt->type);

    APP_EVENT_MANAGER_LOG(header, "Type: %s", type_str);
}

APP_EVENT_TYPE_DEFINE(
    motor_module_event,
    log_motor_event,
    NULL,
    APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LOG_MOTOR_MODULE_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));