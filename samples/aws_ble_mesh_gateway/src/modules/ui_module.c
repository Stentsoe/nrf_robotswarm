/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/device.h>

#define MODULE ui_module

#include "modules_common.h"
#include "app_module_event.h"
#include "ui_module_event.h"

#include <zephyr/logging/log.h>
#define UI_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, UI_MODULE_LOG_LEVEL);

/* Ui module super states. */
static enum state_type {
	STATE_INIT,
	STATE_RUNNING,
} state;

struct ui_msg_data {
	union {
		struct app_module_event app;
	} module;
};

static struct module_data self = {
	.name = "ui",
	.msg_q = NULL,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void message_handler(struct ui_msg_data *msg);

static inline void state_set(enum state_type new_state)
{
	state = new_state;
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_app_module_event(aeh)) {
		struct app_module_event *event = cast_app_module_event(aeh);
		struct ui_msg_data ui_msg = {
			.module.app = *event
		};

		message_handler(&ui_msg);
	}

	return false;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	

	if (has_changed & button_states) 
	{
		struct ui_module_event *event = new_ui_module_event();
		event->type = UI_EVT_BUTTON;
		event->data.button.action = BUTTON_PRESS;

		if (has_changed & DK_BTN1_MSK) 
		{
			event->data.button.num = DK_BTN1;
		} 
		else if (has_changed & DK_BTN2_MSK) 
		{
			event->data.button.num = DK_BTN2;
		}
		APP_EVENT_SUBMIT(event);
	}

	

}

/* Static module functions. */
static int setup(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
		return err;
	}

	return 0;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err = module_start(&self);

		if (err) {
			LOG_ERR("Failed starting module, error: %d", err);
			SEND_ERROR(ui, UI_EVT_ERROR, err);
		}

		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct ui_msg_data *msg)
{
	
}

/* Message handler for all states. */
static void on_all_states(struct ui_msg_data *msg)
{
}

static void message_handler(struct ui_msg_data *msg)
{
	switch (state) {
	case STATE_INIT:
		on_state_init(msg);
		break;
	case STATE_RUNNING:
		on_state_running(msg);
		break;
	default:
		break;
	}

	on_all_states(msg);
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);

SYS_INIT(setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

