
#include <zephyr.h>
#include <app_event_manager.h>

#define MODULE motor
#include "../events/module_state_event.h"
#include "../events/mesh_module_event.h"
#include "../events/motor_module_event.h"

#include "../../drivers/motors/motor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MOTOR_MODULE_LOG_LEVEL);

// Forward declarations
enum motor_module_state
{
    STANDBY,       // Movement not configured, can not move.
    READY_TO_MOVE, // Movement configured, waiting for clear to move.
    MOVING,        // In motion. No new new configurations will be accepted.
};

static void set_module_state(enum motor_module_state new_state);

/* Message queue */

struct motor_msg_data
{
    union
    {
        struct mesh_module_event mesh;
    } event;
};
K_MSGQ_DEFINE(motor_module_msg_q, sizeof(struct motor_msg_data), 10, 4);

/* Global module data */
static const int32_t motor_power = 10000000;

static const struct device *motor_a = DEVICE_DT_GET(DT_NODELABEL(motor_a));
static const struct device *motor_b = DEVICE_DT_GET(DT_NODELABEL(motor_b));

struct robot_movement_config next_movement = {0};
static int set_next_angle(int32_t angle)
{
    next_movement.angle = angle;
    return 0;
}
static int set_next_time(uint32_t time)
{
    next_movement.time = time;
    return 0;
}

/* Motor actuation */

static void stop_motor_work_fn(struct k_work_user *work)
{
    drive_continous(motor_a, 0);
    drive_continous(motor_b, 0);
    LOG_DBG("Stopped motors");
    set_module_state(STANDBY);
}
K_WORK_DELAYABLE_DEFINE(stop_motor_work, stop_motor_work_fn);

static int turn_degrees(int32_t angle)
{
    // TODO: Implement somewhat accurate turning
    return 0;
}

static int drive_forward(uint32_t time)
{
    drive_continous(motor_a, motor_power);
    drive_continous(motor_b, motor_power);
    LOG_DBG("Started motors");
    k_work_schedule(&stop_motor_work, K_MSEC(time));
    return 0;
}

/* State handling*/

static enum motor_module_state module_state = STANDBY; // Current state of the module

static void set_module_state(enum motor_module_state new_state)
{
    module_state = new_state;
}

static int on_state_standby(struct motor_msg_data *msg)
{
    if (is_mesh_module_event((struct event_header *)(&msg->event.mesh)))
    {
        switch (msg->event.mesh.type)
        {
        case MESH_EVT_MOVEMENT_RECEIVED:
        {
            set_next_time(msg->event.mesh.data.movement.time);
            set_next_angle(msg->event.mesh.data.movement.angle);
            LOG_DBG("New movement received: Time:%d  Angle:%d", next_movement.time, next_movement.angle);
            set_module_state(READY_TO_MOVE);
            return 0;
        }
        default:
        {
            return 0;
        }
        }
    }
    return 0;
}

static int on_state_ready_to_move(struct motor_msg_data *msg)
{
    if (is_mesh_module_event(&msg->event.mesh))
    {
        switch (msg->event.mesh.type)
        {
        case MESH_EVT_MOVEMENT_RECEIVED:
        {
            set_next_time(msg->event.mesh.data.movement.time);
            set_next_angle(msg->event.mesh.data.movement.angle);
            LOG_DBG("New movement received: Time:%d  Angle:%d", next_movement.time, next_movement.angle);
            return 0;
        }
        case MESH_EVT_CLEAR_TO_MOVE_RECEIVED:
        {
            LOG_DBG("Starting movement");
            turn_degrees(next_movement.angle);
            drive_forward(next_movement.time);
            set_module_state(MOVING);
            return 0;
        }
        default:
        {
            return 0;
        }
        }
    }
    return 0;
}

static int on_state_moving(struct motor_msg_data *msg)
{
    return 0;
}

/* Setup */

static int init_motors()
{
    LOG_DBG("Initializing motor drivers");
    int err;
    err = !device_is_ready(motor_a);
    if (err)
    {
        LOG_ERR("Motor a not ready: Error %d", err);
        return err;
    }

    err = !device_is_ready(motor_b);
    if (err)
    {
        LOG_ERR("Motor b not ready: Error %d", err);
        return err;
    }
    return 0;
}

/* Module thread */
static void module_thread_fn(void)
{
    LOG_DBG("motor module thread started");
    struct motor_msg_data msg;

    int err;

    err = init_motors();
    if (err)
    {
        return;
    }

    while (true)
    {
        k_msgq_get(&motor_module_msg_q, &msg, K_FOREVER);

        switch (module_state)
        {
        case STANDBY:
        {
            on_state_standby(&msg);
            break;
        }
        case READY_TO_MOVE:
        {
            on_state_ready_to_move(&msg);
            break;
        }
        case MOVING:
        {
            on_state_moving(&msg);
            break;
        }
        default:
        {
            LOG_ERR("Unknown motor module state %d", module_state);
        }
        }
    }
}

K_THREAD_DEFINE(
    motor_module_thread,
    CONFIG_MOTOR_THREAD_STACK_SIZE,
    module_thread_fn,
    NULL,
    NULL,
    NULL,
    K_LOWEST_APPLICATION_THREAD_PRIO,
    0,
    0);

/* Event handling */
static bool app_event_handler(const struct app_event_header *header)
{
    struct motor_msg_data msg = {0};
    bool enqueue = false;

    if (is_mesh_module_event(header))
    {
        msg.event.mesh = *cast_mesh_module_event(header);
        enqueue = true;
    }

    if (enqueue)
    {
        int err = k_msgq_put(&motor_module_msg_q, &msg, K_FOREVER);
        if (err)
        {
            LOG_ERR("Message could not be enqueued");
        }
    }

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, mesh_module_event);