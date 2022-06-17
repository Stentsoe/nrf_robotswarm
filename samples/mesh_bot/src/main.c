/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>
#include <zephyr/logging/log.h>

#define MODULE main
#include "module_state_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#include "../drivers/motors/motor.h"

void main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	const struct device *motor_a_dev = device_get_binding(DT_LABEL(DT_NODELABEL(motora)));
	const struct device *motor_b_dev = device_get_binding(DT_LABEL(DT_NODELABEL(motorb)));
	if (motor_a_dev == NULL || motor_b_dev == NULL) {
		LOG_ERR("Could not get motor");
	} else {
		drive_continous(motor_a_dev, -20000000);
		drive_continous(motor_b_dev, 20000000);
	}

}
