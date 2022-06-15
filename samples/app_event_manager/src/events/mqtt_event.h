/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MQTT_EVENT_H_
#define _MQTT_EVENT_H_

/**
 * @brief MQTT Event
 * @defgroup mqtt_event MQTT Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mqtt_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(mqtt_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MQTT_EVENT_H_ */
