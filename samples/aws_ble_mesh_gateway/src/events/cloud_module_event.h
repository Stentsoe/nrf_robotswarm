/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CLOUD_MODULE_EVENT_H_
#define _CLOUD_MODULE_EVENT_H_

/**
 * @brief CLOUD Event
 * @defgroup cloud_module_event CLOUD Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <qos.h>

#ifdef __cplusplus
extern "C" {
#endif


enum cloud_module_event_type {
	CLOUD_EVT_CONNECTED,
	CLOUD_EVT_CONNECTING,
	CLOUD_EVT_DISCONNECTED,
	CLOUD_EVT_CONNECTION_TIMEOUT,
	CLOUD_EVT_SEND_QOS,
	CLOUD_EVT_SEND_QOS_CLEAR,
	CLOUD_EVT_UPDATE_DELTA,
	CLOUD_EVT_ERROR,
};

#define CONFIG_NUM_ROBOTS 8


struct publish_data {
	char * ptr;
	int len;
};


struct cloud_module_event {
	struct app_event_header header;
	enum cloud_module_event_type type;
	union {
		/** Variable that contains the message that should be sent to cloud. */
		struct qos_data qos_msg;
		struct publish_data pub_msg;
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(cloud_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CLOUD_MODULE_EVENT_H_ */
