#pragma once

#include <app_event_manager.h>

typedef enum {
    MOTOR_EVT_MOVEMENT_START,
    MOTOR_EVT_MOVEMENT_DONE,
} motor_module_event_type;

struct motor_module_event {
    struct app_event_header header;
    motor_module_event_type type;
};

APP_EVENT_TYPE_DECLARE(motor_module_event);