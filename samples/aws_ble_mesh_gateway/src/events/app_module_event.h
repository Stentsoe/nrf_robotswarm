/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODULE_STATE_EVENT_H_
#define _APP_MODULE_EVENT_H_

/**
 * @brief Module Event
 * @defgroup app_module_event Module State Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain/common.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Module states. */
enum app_module_event_type {
	/** Signal that the application has done necessary setup, and
	 *  now started.
	 */
	APP_EVT_START,
};

/** Module event. */
struct app_module_event {
	struct app_event_header header;
	enum app_module_event_type type;
};

APP_EVENT_TYPE_DECLARE(app_module_event);


/**
 * @}
 */

#endif /* _MODULE_STATE_EVENT_H_ */
