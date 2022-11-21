/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_DBG);

#include <net/golioth/settings.h>
#include "main.h"

static int32_t _loop_delay_s = 60;

static uint8_t _relay_0_state = 0;
static uint8_t _relay_1_state = 0;
static bool _relay_0_auto = false;
static bool _relay_1_auto = false;

int32_t get_loop_delay_s(void) {
	return _loop_delay_s;
}

enum golioth_settings_status on_setting(
		const char *key,
		const struct golioth_settings_value *value) {

	LOG_DBG("Received setting: key = %s, type = %d", key, value->type);
	if (strcmp(key, "LOOP_DELAY_S") == 0) {
		/* This setting is expected to be numeric, return an error if it's not */
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_INT64) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Limit to 12 hour max delay: [1, 43200] */
		if (value->i64 < 1 || value->i64 > 43200) {
			return GOLIOTH_SETTINGS_VALUE_OUTSIDE_RANGE;
		}

		/* Only update if value has changed */
		if (_loop_delay_s == (int32_t)value->i64) {
			LOG_DBG("Received LOOP_DELAY_S already matches local value.");
		}
		else {
			_loop_delay_s = (int32_t)value->i64;
			LOG_INF("Set loop delay to %d seconds", _loop_delay_s);

			wake_system_thread();
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	if (strcmp(key, "RELAY0_AUTO") == 0) {
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_BOOL) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Only update if value has changed */
		if (_relay_0_auto == (int32_t)value->b) {
			LOG_DBG("Received RELAY0_AUTO already matches local value.");
		}
		else {
			_relay_0_auto = (bool)value->b;

			LOG_INF("Set _relay_0_auto to: %s", (bool)value->b == true ? "true" : "false");
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	if (strcmp(key, "RELAY1_AUTO") == 0) {
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_BOOL) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Only update if value has changed */
		if (_relay_1_auto == (int32_t)value->b) {
			LOG_DBG("Received RELAY1_AUTO already matches local value.");
		}
		else {
			_relay_1_auto = (bool)value->b;

			LOG_INF("Set _relay_1_auto to: %s", (bool)value->b == true ? "true" : "false");
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	/* If the setting is not recognized, we should return an error */
	return GOLIOTH_SETTINGS_KEY_NOT_RECOGNIZED;
}

int app_register_settings(struct golioth_client *settings_client) {
	int err = golioth_settings_register_callback(settings_client, on_setting);

	if (err) {
		LOG_ERR("Failed to register settings callback: %d", err);
	}

	return err;
}
