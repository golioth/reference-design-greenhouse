/*
 * Copyright (c) 2022-2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_state, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <golioth/lightdb_state.h>
#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include "json_helper.h"

#include "app_state.h"
#include "app_sensors.h"

#define DEVICE_STATE_FMT "{\"light\":%d,\"vent\":%d}"

static struct golioth_client *client;

static void async_handler(struct golioth_client *client,
				       const struct golioth_response *response,
				       const char *path,
				       void *arg)
{
	if (response->status != GOLIOTH_OK) {
		LOG_WRN("Failed to set state: %d", response->status);
		return;
	}

	LOG_DBG("State successfully set");
}

int app_state_reset_desired(void)
{
	LOG_INF("Resetting \"%s\" LightDB State endpoint to defaults.", APP_STATE_DESIRED_ENDP);

	char sbuf[sizeof(DEVICE_STATE_FMT) + 4]; /* space for two "-1" values */

	snprintk(sbuf, sizeof(sbuf), DEVICE_STATE_FMT, -1, -1);

	int err;
	err = golioth_lightdb_set_async(client,
					APP_STATE_DESIRED_ENDP,
					GOLIOTH_CONTENT_TYPE_JSON,
					sbuf,
					strlen(sbuf),
					async_handler,
					NULL);
	if (err) {
		LOG_ERR("Unable to write to LightDB State: %d", err);
	}

	return err;
}

int app_state_update_actual(void)
{
	struct relay_state r_state;
	get_relay_state(&r_state);

	char sbuf[sizeof(DEVICE_STATE_FMT) + 10]; /* space for uint16 values */

	snprintk(sbuf, sizeof(sbuf), DEVICE_STATE_FMT, r_state.light, r_state.vent);

	int err;

	err = golioth_lightdb_set_async(client,
					APP_STATE_ACTUAL_ENDP,
					GOLIOTH_CONTENT_TYPE_JSON,
					sbuf,
					strlen(sbuf),
					async_handler,
					NULL);

	if (err) {
		LOG_ERR("Unable to write to LightDB State: %d", err);
	}

	return err;
}

static void app_state_desired_handler(struct golioth_client *client,
				      const struct golioth_response *response,
				      const char *path,
				      const uint8_t *payload,
				      size_t payload_size,
				      void *arg)
{
	int err = 0;
	int ret = 0;

	if (response->status != GOLIOTH_OK) {
		LOG_ERR("Failed to receive '%s' endpoint: %d",
			APP_STATE_DESIRED_ENDP,
			response->status);
		return;
	}

	LOG_HEXDUMP_DBG(payload, payload_size, APP_STATE_DESIRED_ENDP);

	struct greenhouse_state parsed_state;

	ret = json_obj_parse((char *)payload, payload_size, greenhouse_state_descr,
			     ARRAY_SIZE(greenhouse_state_descr), &parsed_state);

	if (ret < 0) {
		LOG_ERR("Error parsing desired values: %d", ret);
		app_state_reset_desired();
		return;
	}

	uint8_t desired_processed_count = 0;
	uint8_t state_change_count = 0;

	if (ret & 1<<0) {
		/* Process  relay_state light */
		if ((parsed_state.light >= 0) && (parsed_state.light < 2)) {
			LOG_DBG("Validated desired Light setting: %d", parsed_state.light);

			err = manual_light_on_off(parsed_state.light);
			if (err == -EBUSY) {
				LOG_ERR("Manual input ignored while Light set to auto");
			} else if (err) {
				LOG_ERR("Error changing state of Light: %d", err);
			}

			++desired_processed_count;
			++state_change_count;

		} else if (parsed_state.light == -1) {
			LOG_DBG("No change requested for Light");
		} else {
			LOG_ERR("Invalid desired Light value: %d", parsed_state.light);
			++desired_processed_count;
		}
	}

	if (ret & 1<<1) {
		/* Process  relay_state vent */
		if ((parsed_state.vent >= 0) && (parsed_state.vent < 2)) {
			LOG_DBG("Validated desired Vent setting: %d", parsed_state.vent);

			err = manual_vent_on_off(parsed_state.vent);
			if (err == -EBUSY) {
				LOG_ERR("Manual input ignored while Vent set to auto");
			} else if (err) {
				LOG_ERR("Error changing state of Vent: %d", err);
			}

			++desired_processed_count;
			++state_change_count;

		} else if (parsed_state.vent == -1) {
			LOG_DBG("No change requested for Vent");
		} else {
			LOG_ERR("Invalid desired Vent value: %d", parsed_state.vent);
			++desired_processed_count;
		}
	}

	if (state_change_count) {
		/* The state was changed, so update the state on the Golioth servers */
		err = app_state_update_actual();
	}
	if (desired_processed_count) {
		/* We processed some desired changes to return these to -1 on the server
		 * to indicate the desired values were received.
		 */
		err = app_state_reset_desired();
	}

	if (err) {
		LOG_ERR("Failed to update cloud state: %d", err);
	}
}

int app_state_observe(struct golioth_client *state_client)
{
	int err;

	client = state_client;

	err = golioth_lightdb_observe_async(client, APP_STATE_DESIRED_ENDP,
						app_state_desired_handler, NULL);
	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
		return err;
	}

	/* This will only run once. It updates the actual state of the device
	 * with the Golioth servers. Future updates will be sent whenever
	 * changes occur.
	 */
	err = app_state_update_actual();

	return err;
}
