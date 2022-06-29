/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <modem/lte_lc.h>

#define MODULE modem_module

#include "modules_common.h"
#include "app_module_event.h"
#include "modem_module_event.h"
#include "ui_module_event.h"

#include <zephyr/logging/log.h>
#define MODEM_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, MODEM_MODULE_LOG_LEVEL);

/* Modem module super states. */
static enum state_type {
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED,
} state;

struct modem_msg_data {
	union {
		struct app_module_event app;
		struct modem_module_event modem;
		struct ui_module_event ui;
	} module;
};

/* Modem module message queue. */
#define MODEM_QUEUE_ENTRY_COUNT		10
#define MODEM_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_modem, sizeof(struct modem_msg_data),
	      MODEM_QUEUE_ENTRY_COUNT, MODEM_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "modem",
	.msg_q = &msgq_modem,
	.supports_shutdown = true,
};

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_DISCONNECTED:
		return "STATE_DISCONNECTED";
	case STATE_CONNECTING:
		return "STATE_CONNECTING";
	case STATE_CONNECTED:
		return "STATE_CONNECTED";
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
	struct modem_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_app_module_event(aeh)) {
		struct app_module_event *evt = cast_app_module_event(aeh);
		
		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);
		LOG_INF("event %d", evt->type);
		msg.module.modem = *evt;
		enqueue_msg = true;
	}

	if (is_ui_module_event(aeh)) {
		struct ui_module_event *evt = cast_ui_module_event(aeh);

		msg.module.ui = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	return false;
}

static void lte_evt_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS: {
		LOG_INF("nw_reg_status: %d", evt->nw_reg_status);
		if (evt->nw_reg_status == LTE_LC_NW_REG_NOT_REGISTERED) {
			SEND_EVENT(modem, MODEM_EVT_LTE_DISCONNECTED);
			break;
		}

		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
				break;
		}

        LOG_INF("Connected to: %s network\n",
             evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "home" : "roaming");
		SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTED);
		break;
	}
	default:
		break;
	}
}

/* Static module functions. */
static int setup(void)
{
	int err;

	err = lte_lc_init();
	if (err) {
		LOG_ERR("lte_lc_init, error: %d", err);
		return err;
	}

	return err;
}

static int lte_connect(void)
{
	int err;

	err = lte_lc_connect_async(lte_evt_handler);
	if (err) {
		LOG_ERR("lte_lc_connect_async, error: %d", err);

		return err;
	}

	SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTING);

	return 0;
}

static int lte_disconnect(void)
{
	int err;

	err = lte_lc_offline();
	if (err) {
		LOG_ERR("failed to go offline, error: %d", err);

		return err;
	}

	return 0;
}

/* Message handler for STATE_DISCONNECTED. */
static void on_state_disconnected(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, ui, UI_EVT_BUTTON)) {
		int err;
		
		LOG_INF("button event");
		if ((msg->module.ui.data.button.action == BUTTON_PRESS) &&
		(msg->module.ui.data.button.num == BTN2)) {
			
			err = lte_connect();
			if (err) {
				LOG_ERR("Failed connecting to LTE, error: %d", err);
				SEND_ERROR(modem, MODEM_EVT_ERROR, err);
				return;
			}
		}
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTING)) {
		state_set(STATE_CONNECTING);
	}
}

/* Message handler for STATE_CONNECTING. */
static void on_state_connecting(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) {
		state_set(STATE_CONNECTED);
	}
}

/* Message handler for STATE_CONNECTED. */
static void on_state_connected(struct modem_msg_data *msg)
{
	// if (IS_EVENT(msg, ui, UI_EVT_BUTTON)) {
	// 	int err;
		
	// 	if ((msg->module.ui.data.button.action == BUTTON_PRESS) &&
	// 	(msg->module.ui.data.button.num == BTN1)) {
			
	// 		err = lte_disconnect();
	// 		if (err) {
	// 			LOG_ERR("Failed disconnecting from LTE, error: %d", err);
	// 			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
	// 			return;
	// 		}
	// 	}
	// }

	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_DISCONNECTED)) {
		state_set(STATE_DISCONNECTED);
	}
}

/* Message handler for all states. */
static void on_all_states(struct modem_msg_data *msg)
{
		if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err;

		err = lte_connect();
		if (err) {
			LOG_ERR("Failed connecting to LTE, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}
	}
}

static void module_thread_fn(void)
{
	int err;
	struct modem_msg_data msg;

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(modem, MODEM_EVT_ERROR, err);
	}

	state_set(STATE_DISCONNECTED);
	SEND_EVENT(modem, MODEM_EVT_INITIALIZED);

	err = setup();
	if (err) {
		LOG_ERR("Failed setting up the modem, error: %d", err);
		SEND_ERROR(modem, MODEM_EVT_ERROR, err);
	}

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_DISCONNECTED:
			on_state_disconnected(&msg);
			break;
		case STATE_CONNECTING:
			on_state_connecting(&msg);
			break;
		case STATE_CONNECTED:
			on_state_connected(&msg);
			break;
		default:
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(modem_module_thread, CONFIG_MODEM_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE(MODULE, ui_module_event);


