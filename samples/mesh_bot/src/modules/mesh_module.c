
#include <zephyr.h>
#include <app_event_manager.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/dk_prov.h>

#define MODULE mesh
#include "../events/module_state_event.h"
#include "../events/motor_module_event.h"
#include "../events/mesh_module_event.h"

#include "../model_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_MODULE_LOG_LEVEL);

/* State handling*/

// Top level module states
enum mesh_module_state
{
    UNPROVISIONED,
    PROVISIONED,
    READY_TO_MOVE,
    MOVING,
};

enum mesh_module_state module_state = UNPROVISIONED; // Current state of the module

static void set_module_state(enum mesh_module_state new_state)
{
    module_state = new_state;
}

/* Mesh handlers */

static void movement_received_handler(struct robot_movement_config *movement) {
    LOG_DBG("Movement received: Time:%d  Angle:%d", movement->time, movement->angle);
}

/* Setup */

static int setup_mesh()
{
    int err;
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Failed to initialize bluetooth: Error %d", err);
        return err;
    }
    LOG_DBG("Bluetooth initialized");
    err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init(movement_received_handler));
    if (err) {
        LOG_ERR("Failed to initialize mesh: Error %d", err);
        return err;
    }

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        err = settings_load();
        if (err) {
            LOG_ERR("Failed to load settings: Error %d", err);
        }
    }

    err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
    if (err == -EALREADY) {
        LOG_DBG("Device already provisioned");
    }
    LOG_DBG("Mesh initialized");
    return 0;
}

/* Message queue */

/** Message item that holds any received events */
struct mesh_msg_data
{
    union
    {
        struct motor_module_event motor;
    } event;
};

/** Module message queue */
K_MSGQ_DEFINE(mesh_module_msg_q, sizeof(struct mesh_msg_data), 10, 4);

/* Module thread */
static void module_thread_fn(void)
{
    LOG_DBG("Mesh module thread started");
    struct mesh_msg_data msg;

    int err;

    err = setup_mesh();

    if (err) {
        LOG_ERR("Failed to set up mesh. Error %d", err);
        return;
    }

    while (true) {
        k_msgq_get(&mesh_module_msg_q, &msg, K_FOREVER);

        switch(module_state) {
            case UNPROVISIONED: {
                break;
            }
            case PROVISIONED: {
                break;
            }
            case READY_TO_MOVE: {
                break;
            }
            case MOVING: {
                break;
            }
            default: {
                LOG_ERR("Unknown mesh module state %d", module_state);
            }
        }
    }
}

K_THREAD_DEFINE(
    mesh_module_thread,
    CONFIG_MESH_THREAD_STACK_SIZE,
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
    struct mesh_msg_data msg = {0};
    bool enqueue = false;

    if (is_motor_module_event(header))
    {
        LOG_DBG("Motor module event received");
        struct motor_module_event *evt = cast_motor_module_event(header);
        msg.event.motor = *evt;
        enqueue = true;
    }

    if (enqueue)
    {
        int err = k_msgq_put(&mesh_module_msg_q, &msg, K_FOREVER);
        if (err)
        {
            LOG_ERR("Message could not be enqueued");
        }
    }

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, motor_module_event);