/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <net/aws_iot.h>
#include <nrf_modem_at.h>
#include <string.h>
#include <qos.h>

#define MODULE cloud_module

#include "modules_common.h"
#include "modem_module_event.h"
#include "cloud_module_event.h"

#include <zephyr/logging/log.h>
#define CLOUD_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, CLOUD_MODULE_LOG_LEVEL);

#define TOPIC_UPDATE_DELTA "$aws/things/robot_wars_gateway/shadow/update/delta"
#define TOPIC_GET_ACCEPTED "$aws/things/robot_wars_gateway/shadow/get/accepted"
#define TOPIC_UPDATE_ACCEPTED "$aws/things/robot_wars_gateway/shadow/update/accepted"
#define TOPIC_UPDATE_REJECTED "$aws/things/robot_wars_gateway/shadow/update/rejected"

QOS_MESSAGE_TYPES_REGISTER(CLOUD_SHADOW_UPDATE);
QOS_MESSAGE_TYPES_REGISTER(CLOUD_SHADOW_CLEAR);

struct cloud_msg_data {
	union {
		struct cloud_module_event cloud;
		struct modem_module_event modem;
	} module;
};

/* Cloud module super states. */
static enum state_type {
	STATE_INIT,
	STATE_LTE_DISCONNECTED,
	STATE_LTE_CONNECTED,
} state;

/* Cloud module sub states. */
static enum sub_state_type {
	SUB_STATE_CLOUD_DISCONNECTED,
	SUB_STATE_CLOUD_CONNECTED,
} sub_state;

static struct k_work_delayable connect_check_work;

struct cloud_backoff_delay_lookup {
	int delay;
};

/* Lookup table for backoff reconnection to cloud. Binary scaling. */
static struct cloud_backoff_delay_lookup backoff_delay[] = {
	{ 32 }, { 64 }, { 128 }, { 256 }, { 512 },
	{ 2048 }, { 4096 }, { 8192 }, { 16384 }, { 32768 },
	{ 65536 }, { 131072 }, { 262144 }, { 524288 }, { 1048576 }
};

/* Variable that keeps track of how many times a reconnection to cloud
 * has been tried without success.
 */
static int connect_retries;

/* Cloud module message queue. */
#define CLOUD_QUEUE_ENTRY_COUNT		20
#define CLOUD_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_cloud, sizeof(struct cloud_msg_data),
	      CLOUD_QUEUE_ENTRY_COUNT, CLOUD_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "cloud",
	.msg_q = &msgq_cloud,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void connect_check_work_fn(struct k_work *work);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_LTE_DISCONNECTED:
		return "STATE_LTE_DISCONNECTED";
	case STATE_LTE_CONNECTED:
		return "STATE_LTE_CONNECTED";
	default:
		return "Unknown";
	}
}

static char *sub_state2str(enum sub_state_type new_state)
{
	switch (new_state) {
	case SUB_STATE_CLOUD_DISCONNECTED:
		return "SUB_STATE_CLOUD_DISCONNECTED";
	case SUB_STATE_CLOUD_CONNECTED:
		return "SUB_STATE_CLOUD_CONNECTED";
	default:
		return "Unknown";
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

static void sub_state_set(enum sub_state_type new_state)
{
	if (new_state == sub_state) {
		LOG_DBG("Sub state: %s", sub_state2str(sub_state));
		return;
	}

	LOG_DBG("Sub state transition %s --> %s",
		sub_state2str(sub_state),
		sub_state2str(new_state));

	sub_state = new_state;
}

static inline int is_topic(const struct aws_iot_evt *evt, char* topic) 
{
	return (0 == strcmp(evt->data.msg.topic.str, topic));
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct cloud_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);
		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);
		msg.module.modem = *evt;
		enqueue_msg = true;
	}


	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
		}
	}

	return false;
}

static void cloud_event_handler(const struct aws_iot_evt *evt) 
{
switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING: {
		LOG_DBG("AWS_IOT_EVT_CONNECTING %d", evt->data.err);
		SEND_EVENT(cloud, CLOUD_EVT_CONNECTING);
		} break;
	case AWS_IOT_EVT_CONNECTED: {
		LOG_DBG("AWS_IOT_EVT_CONNECTED, err %d", evt->data.err);
		SEND_EVENT(cloud, CLOUD_EVT_CONNECTED);
		} break;
	case AWS_IOT_EVT_READY:
		LOG_DBG("AWS_IOT_EVT_READY");
		break;
	case AWS_IOT_EVT_DISCONNECTED: {
		LOG_DBG("AWS_IOT_EVT_DISCONNECTED %d", evt->data.err);
		SEND_EVENT(cloud, CLOUD_EVT_DISCONNECTED);
		} break;
	case AWS_IOT_EVT_DATA_RECEIVED: {
		LOG_DBG("AWS_IOT_EVT_DATA_RECEIVED: %s", evt->data.msg.topic.str);
		
		if (is_topic(evt, TOPIC_UPDATE_DELTA)) {
			LOG_DBG("received delta update of length %d", evt->data.msg.len);

			struct cloud_module_event *event = new_cloud_module_event();
			event->type = CLOUD_EVT_UPDATE_DELTA;
			event->data.pub_msg.ptr = evt->data.msg.ptr;
			event->data.pub_msg.len = evt->data.msg.len;
			APP_EVENT_SUBMIT(event);
		}
		
	    } break;
	case AWS_IOT_EVT_PUBACK: {
		LOG_DBG("AWS_IOT_EVT_PUBACK");
		int err;
		err = qos_message_remove(evt->data.message_id);
		if (err == -ENODATA) {
			LOG_DBG("Message Acknowledgment not in pending QoS list, ID: %d",
				evt->data.message_id);
		} else if (err) {
			LOG_ERR("qos_message_remove, error: %d", err);
			SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
		}
		} break;
	case AWS_IOT_EVT_PINGRESP:
		LOG_DBG("AWS_IOT_EVT_PINGRESP");
		break;
	case AWS_IOT_EVT_FOTA_START:
		LOG_DBG("AWS_IOT_EVT_FOTA_START");
		break;
	case AWS_IOT_EVT_FOTA_ERASE_PENDING:
		LOG_DBG("AWS_IOT_EVT_FOTA_ERASE_PENDING");
		break;
	case AWS_IOT_EVT_FOTA_ERASE_DONE:
		LOG_DBG("AWS_IOT_EVT_FOTA_ERASE_DONE");
		break;
	case AWS_IOT_EVT_FOTA_DONE:
		LOG_DBG("AWS_IOT_EVT_FOTA_DONE");
		break;
	case AWS_IOT_EVT_FOTA_DL_PROGRESS:
		/* Dont spam FOTA progress events. */
		break;
	case AWS_IOT_EVT_FOTA_ERROR:
		LOG_DBG("AWS_IOT_EVT_FOTA_ERROR");
		break;
	case AWS_IOT_EVT_ERROR: {
		LOG_DBG("AWS_IOT_EVT_ERROR");
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, evt->data.err);
		} break;
	default:
		LOG_ERR("Unknown AWS IoT event type: %d", evt->type);
		break;
	}
}

static void qos_event_handler(const struct qos_evt *evt)
{
	switch (evt->type) {
	case QOS_EVT_MESSAGE_NEW: {
		// LOG_DBG("QOS_EVT_MESSAGE_NEW");
		if (evt->message.type == CLOUD_SHADOW_UPDATE) {
			struct cloud_module_event *event = new_cloud_module_event();

			event->type = CLOUD_EVT_SEND_QOS;
			event->data.qos_msg = evt->message;

			APP_EVENT_SUBMIT(event);
		}

		if (evt->message.type == CLOUD_SHADOW_CLEAR) {
			struct cloud_module_event *event = new_cloud_module_event();

			event->type = CLOUD_EVT_SEND_QOS_CLEAR;
			event->data.qos_msg = evt->message;

			APP_EVENT_SUBMIT(event);
		}	
	}
		break;
	case QOS_EVT_MESSAGE_TIMER_EXPIRED: {
		LOG_DBG("QOS_EVT_MESSAGE_TIMER_EXPIRED");

		struct cloud_module_event *event = new_cloud_module_event();

		event->type = CLOUD_EVT_SEND_QOS;
		event->data.qos_msg = evt->message;

		APP_EVENT_SUBMIT(event);
	}
		break;
	case QOS_EVT_MESSAGE_REMOVED_FROM_LIST:
		// LOG_DBG("QOS_EVT_MESSAGE_REMOVED_FROM_LIST");

		if (evt->message.heap_allocated) {
			LOG_DBG("Freeing pointer: %p", evt->message.data.buf);
			k_free(evt->message.data.buf);
		}
		break;
	default:
		LOG_DBG("Unknown QoS handler event");
		break;
	}
}

/* Static module functions. */
static int setup(void)
{
	int err;

	err =  aws_iot_init(NULL, cloud_event_handler);
	if (err) {
		LOG_ERR("Failed initializing aws, error: %d", err);
	}

	err = qos_init(qos_event_handler);
	if (err) {
		LOG_ERR("qos_init, error: %d", err);
		return err;
	}

	return err;
}

static void connect_cloud(void)
{
	int err;
	int backoff_sec = backoff_delay[connect_retries].delay;

	LOG_DBG("Connecting to cloud");

	if (connect_retries > CONFIG_CLOUD_CONNECT_RETRIES) {
		LOG_WRN("Too many failed cloud connection attempts");
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, -ENETUNREACH);
		return;
	}
	/* The cloud will return error if cloud_wrap_connect() is called while
	 * the socket is polled on in the internal cloud thread or the
	 * cloud backend is the wrong state. We cannot treat this as an error as
	 * it is rather common that cloud_connect can be called under these
	 * conditions.
	 */
	err = aws_iot_connect(NULL);
	if (err) {
		LOG_ERR("cloud_connect failed, error: %d", err);
	}

	connect_retries++;

	LOG_WRN("Cloud connection establishment in progress");
	LOG_WRN("New connection attempt in %d seconds if not successful",
		backoff_sec);

	/* Start timer to check connection status after backoff */
	k_work_reschedule(&connect_check_work, K_SECONDS(backoff_sec));
}

static void disconnect_cloud(void)
{
	int err;

	err = aws_iot_disconnect();
	if (err) {
		LOG_ERR("aws_iot_disconnect, error: %d", err);
		return;
	}

	connect_retries = 0;
	qos_timer_reset();

	k_work_cancel_delayable(&connect_check_work);
}

/* Convenience function used to add messages to the QoS library. */
static void add_qos_message(uint8_t *ptr, size_t len, uint8_t type,
			    uint32_t flags, bool heap_allocated)
{
	int err;
	struct qos_data message = {
		.heap_allocated = heap_allocated,
		.data.buf = ptr,
		.data.len = len,
		.id = qos_message_id_get_next(),
		.type = type,
		.flags = flags
	};

	err = qos_message_add(&message);
	if (err == -ENOMEM) {
		LOG_WRN("Cannot add message, internal pending list is full");
	} else if (err) {
		LOG_ERR("qos_message_add, error: %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
	}
}

/* If this work is executed, it means that the connection attempt was not
 * successful before the backoff timer expired. A timeout message is then
 * added to the message queue to signal the timeout.
 */
static void connect_check_work_fn(struct k_work *work)
{
	// If cancelling works fails
	if ((state == STATE_LTE_CONNECTED && sub_state == SUB_STATE_CLOUD_CONNECTED) ||
		(state == STATE_LTE_DISCONNECTED)) {
		return;
	}

	LOG_DBG("Cloud connection timeout occurred");

	SEND_EVENT(cloud, CLOUD_EVT_CONNECTION_TIMEOUT);
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct cloud_msg_data *msg)
{
	if ((IS_EVENT(msg, modem, MODEM_EVT_INITIALIZED))) {
		int err;

		state_set(STATE_LTE_DISCONNECTED);
		err = setup();
		if (err) {
			LOG_ERR("Failed setting up the cloud, error: %d", err);
			SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
		}
	}	
}

/* Message handler for STATE_LTE_DISCONNECTED. */
static void on_state_lte_disconnected(struct cloud_msg_data *msg)
{
	if ((IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED))) {
		state_set(STATE_LTE_CONNECTED);

		/* LTE is now connected, cloud connection can be attempted */
		connect_cloud();
	}
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_state_lte_connected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_DISCONNECTED)) {
		sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);
		state_set(STATE_LTE_DISCONNECTED);

		/* Explicitly disconnect cloud when you receive an LTE disconnected event.
		 * This is to clear up the cloud library state.
		 */
		disconnect_cloud();
	}
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_sub_state_cloud_disconnected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
		sub_state_set(SUB_STATE_CLOUD_CONNECTED);

		connect_retries = 0;
		k_work_cancel_delayable(&connect_check_work);

	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTION_TIMEOUT)) {
		connect_cloud();
	}
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_sub_state_cloud_connected(struct cloud_msg_data *msg)
{
	int err = 0;

	if (IS_EVENT(msg, cloud, CLOUD_EVT_SEND_QOS)) {


		struct qos_payload *qos_payload = &msg->module.cloud.data.qos_msg.data;

		struct aws_iot_data message = {
			.ptr = qos_payload->buf,
			.len = qos_payload->len,
			.message_id = msg->module.cloud.data.qos_msg.id,
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
			.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
		};
		LOG_INF("%s", qos_payload->buf);
		err = aws_iot_send(&message);
		if (err) {
			LOG_ERR("aws_iot_send, error: %d", err);
		}
	}
	
	if (IS_EVENT(msg, cloud, CLOUD_EVT_SEND_QOS_CLEAR)) {
		struct qos_payload *qos_payload = &msg->module.cloud.data.qos_msg.data;

		struct aws_iot_data message = {
			.ptr = qos_payload->buf,
			.len = qos_payload->len,
			.message_id = msg->module.cloud.data.qos_msg.id,
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
			.topic.type = AWS_IOT_SHADOW_TOPIC_DELETE,
		};
		LOG_INF("%s", qos_payload->buf);
		err = aws_iot_send(&message);
		if (err) {
			LOG_ERR("aws_iot_send, error: %d", err);
		}
	}
}

static void module_thread_fn(void)
{
	int err;
	struct cloud_msg_data msg;

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
	}

	state_set(STATE_INIT);
	sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);

	k_work_init_delayable(&connect_check_work, connect_check_work_fn);

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_INIT:
			on_state_init(&msg);
			break;
		case STATE_LTE_DISCONNECTED:
			on_state_lte_disconnected(&msg);
			break;
		case STATE_LTE_CONNECTED:
			switch (sub_state)
			{
			case SUB_STATE_CLOUD_DISCONNECTED:
				on_sub_state_cloud_disconnected(&msg);
				break;
			case SUB_STATE_CLOUD_CONNECTED:
				on_sub_state_cloud_connected(&msg);
				break;
			
			default:
				break;
			}
			
			on_state_lte_connected(&msg);
			break;
		default:
			break;
		}
	}
}

K_THREAD_DEFINE(cloud_module_thread, CONFIG_CLOUD_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);


