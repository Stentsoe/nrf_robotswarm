/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UI_MODULE_EVENT_H_
#define _UI_MODULE_EVENT_H_

/**
 * @brief UI Event
 * @defgroup ui_module_event UI Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include "dk_buttons_and_leds.h"

#ifdef __cplusplus
extern "C" {
#endif


enum ui_module_event_type {
	UI_EVT_BUTTON,
	UI_EVT_LED,
	UI_EVT_ERROR,
};

struct ui_button {
	enum buton_num {
		BTN1 = DK_BTN1,
		BTN2,
	} num;
	enum button_action {
		BUTTON_PRESS,
		BUTTON_RELEASE,
	} action;
};

struct ui_led {
	int num;
	enum led_action {
		LED_ON,
		LED_OFF,
	} action;
};

struct ui_module_event {
	struct app_event_header header;
	enum ui_module_event_type type;
	union {
		struct ui_button button;
		struct ui_led led;
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(ui_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _UI_MODULE_EVENT_H_ */
