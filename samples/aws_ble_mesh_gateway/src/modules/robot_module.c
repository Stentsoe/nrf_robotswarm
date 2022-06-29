/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>

#define MODULE robot_module

#include "modules_common.h"
#include "app_module_event.h"
#include "robot_module_event.h"
#include "cloud_module_event.h"

#include <zephyr/logging/log.h>
#define ROBOT_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, ROBOT_MODULE_LOG_LEVEL);

/* Robot module super states. */
static enum state_type {
	STATE_CLOUD_DISCONNECTED,
	STATE_CLOUD_CONNECTED,
} state;

/* Robot module super states. */
static enum state_sub_type {
	STATE_CONFIGURING,
	STATE_EXECUTING,
} sub_state;

struct robot_msg_data {
	union {
		struct app_module_event app;
		struct robot_module_event robot;
		struct cloud_module_event cloud;
	} module;
};

/* Robot module message queue. */
#define ROBOT_QUEUE_ENTRY_COUNT		10
#define ROBOT_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_robot, sizeof(struct robot_msg_data),
	      ROBOT_QUEUE_ENTRY_COUNT, ROBOT_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "robot",
	.msg_q = &msgq_robot,
	.supports_shutdown = true,
};

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_CONFIGURING:
		return "STATE_CONFIGURING";
	case STATE_EXECUTING:
		return "STATE_EXECUTING";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}


/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct robot_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_app_module_event(aeh)) {
		struct app_module_event *evt = cast_app_module_event(aeh);
		
		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_robot_module_event(aeh)) {
		struct robot_module_event *evt = cast_robot_module_event(aeh);
		msg.module.robot = *evt;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);
		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(robot, ROBOT_EVT_ERROR, err);
		}
	}

	return false;
}

/* Message handler for STATE_CONFIGURING. */
static void on_state_cloud_disconnected(struct robot_msg_data *msg)
{

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
			report_clear_robot_list();
			report_robot_list();

			state_set(STATE_CLOUD_CONNECTED);
	}
}

/* Message handler for STATE_EXECUTING. */
static void on_state_cloud_connected(struct robot_msg_data *msg)
{

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DISCONNECTED)) {
		report_robot_list();
		state_set(STATE_CLOUD_DISCONNECTED);
	}
}

/* Message handler for all states. */
static void on_all_states(struct robot_msg_data *msg)
{ 

}

static void module_thread_fn(void)
{
	int err;
	struct robot_msg_data msg;

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(robot, ROBOT_EVT_ERROR, err);
	}

	state_set(STATE_CLOUD_DISCONNECTED);

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_CLOUD_DISCONNECTED:
			on_state_cloud_disconnected(&msg);
			break;
		case STATE_CLOUD_CONNECTED:
			on_state_cloud_connected(&msg);
			break;
		default:
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(robot_module_thread, CONFIG_ROBOT_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);
APP_EVENT_SUBSCRIBE(MODULE, cloud_module_event);
