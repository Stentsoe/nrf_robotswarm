/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "cloud_module_event.h"


static void profile_cloud_module_event(struct log_event_buf *buf,
			      const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(cloud_module_event,
		  ENCODE(),
		  ENCODE(),
		  profile_cloud_module_event);

APP_EVENT_TYPE_DEFINE(cloud_module_event,
		  NULL,
		  &cloud_module_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
