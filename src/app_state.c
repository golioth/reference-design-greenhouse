/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_state, LOG_LEVEL_DBG);

#include <net/golioth/system_client.h>
#include <zephyr/data/json.h>
#include "json_helper.h"

#include "app_state.h"
#include "app_work.h"

#define DEVICE_STATE_FMT "{\"light\":%d,\"vent\":%d}"

static struct golioth_client *client;

static K_SEM_DEFINE(reset_desired, 0, 1);
static K_SEM_DEFINE(update_actual, 0, 1);

static int async_handler(struct golioth_req_rsp *rsp)
{
	if (rsp->err) {
		LOG_WRN("Failed to set state: %d", rsp->err);
		return rsp->err;
	}

	LOG_DBG("State successfully set");

	return 0;
}

void app_state_init(struct golioth_client* state_client) {
	client = state_client;
	app_state_update_actual();
	k_sem_give(&reset_desired);
	k_sem_give(&update_actual);
}

static void reset_desired_work_handler(struct k_work *work) {
	char sbuf[strlen(DEVICE_STATE_FMT)+8]; /* small bit of extra space */
	snprintk(sbuf, sizeof(sbuf), DEVICE_STATE_FMT, -1, -1);

	int err;
	err = golioth_lightdb_set_cb(client, APP_STATE_DESIRED_ENDP,
			GOLIOTH_CONTENT_FORMAT_APP_JSON, sbuf, strlen(sbuf),
			async_handler, NULL);
	if (err) {
		LOG_ERR("Unable to write to LightDB State: %d", err);
	}
	k_sem_give(&reset_desired);
}

K_WORK_DEFINE(reset_desired_work, reset_desired_work_handler);

static void update_actual_state_work_handler(struct k_work *work) {
	struct relay_state r_state;
	get_relay_state(&r_state);

	char sbuf[strlen(DEVICE_STATE_FMT)+8]; /* small bit of extra space */
	snprintk(sbuf, sizeof(sbuf), DEVICE_STATE_FMT, r_state.light, r_state.vent);

	int err;
	err = golioth_lightdb_set_cb(client, APP_STATE_ACTUAL_ENDP,
			GOLIOTH_CONTENT_FORMAT_APP_JSON, sbuf, strlen(sbuf),
			async_handler, NULL);
	if (err) {
		LOG_ERR("Unable to write to LightDB State: %d", err);
	}
	k_sem_give(&update_actual);
}

K_WORK_DEFINE(update_actual_state_work, update_actual_state_work_handler);

void app_state_update_actual(void) {
	if (k_sem_take(&update_actual, K_NO_WAIT) == 0) {
		k_work_submit(&update_actual_state_work);
	}
}

int app_state_desired_handler(struct golioth_req_rsp *rsp) {
	if (rsp->err) {
		LOG_ERR("Failed to receive '%s' endpoint: %d", APP_STATE_DESIRED_ENDP, rsp->err);
		return rsp->err;
	}

	LOG_HEXDUMP_DBG(rsp->data, rsp->len, APP_STATE_DESIRED_ENDP);

	struct greenhouse_state parsed_state;

	int ret = json_obj_parse((char *)rsp->data, rsp->len,
			greenhouse_state_descr, ARRAY_SIZE(greenhouse_state_descr),
			&parsed_state);

	if (ret < 0) {
		LOG_ERR("Error parsing desired values: %d", -1);
		k_work_submit(&reset_desired_work);
		return 0;
	}

	uint8_t desired_processed_count = 0;
	uint8_t state_change_count = 0;
	int err;
	if (ret & 1<<0) {
		if ((parsed_state.light >= 0) && (parsed_state.light < 2)) {
			LOG_DBG("Validated desired Light setting: %d", parsed_state.light);
			err = manual_light_on_off(parsed_state.light);
			if (err) {
				if (err == -EBUSY) {
					LOG_ERR("Manual input ignored while Light set to auto");
				}
				else {
					LOG_ERR("Error changing state of Light: %d", err);
				}
			}
			++desired_processed_count;
			++state_change_count;
		}
		else if (parsed_state.light == -1) {
			LOG_DBG("No change requested for Light");
		}
		else {
			LOG_ERR("Invalid desired Light value: %d", parsed_state.light);
			++desired_processed_count;
		}
	}
	if (ret & 1<<1) {
		if ((parsed_state.vent >= 0) && (parsed_state.vent < 2)) {
			LOG_DBG("Validated desired Vent setting: %d", parsed_state.vent);
			err = manual_vent_on_off(parsed_state.vent);
			if (err) {
				if (err == -EBUSY) {
					LOG_ERR("Manual input ignored while Vent set to auto");
				}
				else {
					LOG_ERR("Error changing state of Vent: %d", err);
				}
			}
			++desired_processed_count;
			++state_change_count;
		}
		else if (parsed_state.vent == -1) {
			LOG_DBG("No change requested for Vent");
		}
		else {
			LOG_ERR("Invalid desired Vent value: %d", parsed_state.vent);
			++desired_processed_count;
		}
	}

	if (desired_processed_count) {
		if (k_sem_take(&reset_desired, K_NO_WAIT) == 0) {
			k_work_submit(&reset_desired_work);
		}
	}

	return 0;
}

