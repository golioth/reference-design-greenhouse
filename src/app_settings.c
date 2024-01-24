/*
 * Copyright (c) 2022-2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_DBG);

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <net/golioth/settings.h>
#include "app_settings.h"
#include "main.h"

static struct golioth_client *client;

static struct sensor_value _temp_thresh_sensorval = {0, 0};
static int32_t _loop_delay_s = 60;
static bool _light_auto = false;
static bool _temp_auto = false;
static int64_t _light_thresh = 0;
static float _temp_thresh = 0;

int32_t get_loop_delay_s(void)
{
	return _loop_delay_s;
}

void get_light_settings(struct light_settings *ls)
{
	ls->ctrl_auto = _light_auto;
	ls->thresh = _light_thresh;
}

void get_temp_settings(struct temp_settings *ts)
{
	ts->ctrl_auto = _temp_auto;
	ts->tem.val1 = _temp_thresh_sensorval.val1;
	ts->tem.val2 = _temp_thresh_sensorval.val2;
}

enum golioth_settings_status on_setting(const char *key, const struct golioth_settings_value *value)
{

	LOG_DBG("Received setting: key = %s, type = %d", key, value->type);
	if (strcmp(key, "LOOP_DELAY_S") == 0) {
		/* This setting is expected to be numeric, return an error if it's not */
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_INT64) {
			LOG_DBG("Received LOOP_DELAY_S is not an integer type.");
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Limit to 12 hour max delay: [1, 43200] */
		if (value->i64 < 1 || value->i64 > 43200) {
			LOG_DBG("Received LOOP_DELAY_S setting is outside allowed range.");
			return GOLIOTH_SETTINGS_VALUE_OUTSIDE_RANGE;
		}

		/* Only update if value has changed */
		if (_loop_delay_s == (int32_t)value->i64) {
			LOG_DBG("Received LOOP_DELAY_S already matches local value.");
		} else {
			_loop_delay_s = (int32_t)value->i64;
			LOG_INF("Set loop delay to %d seconds", _loop_delay_s);
			wake_system_thread();
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	if (strcmp(key, "LIGHT_AUTO") == 0) {
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_BOOL) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Only update if value has changed */
		if (_light_auto == (int32_t)value->b) {
			LOG_DBG("Received LIGHT_AUTO already matches local value.");
		} else {
			_light_auto = (bool)value->b;

			LOG_INF("Set _light_auto to: %s", (bool)value->b == true ? "true" : "false");
			wake_system_thread();
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	if (strcmp(key, "TEMP_AUTO") == 0) {
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_BOOL) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Only update if value has changed */
		if (_temp_auto == (int32_t)value->b) {
			LOG_DBG("Received TEMP_AUTO already matches local value.");
		} else {
			_temp_auto = (bool)value->b;

			LOG_INF("Set _temp_auto to: %s", (bool)value->b == true ? "true" : "false");
			wake_system_thread();
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	if (strcmp(key, "LIGHT_THRESH") == 0) {
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_INT64) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Only update if value has changed */
		if (_light_thresh == (int64_t)value->i64) {
			LOG_DBG("Received LIGHT_THRESH already matches local value.");
		} else {
			_light_thresh = (int64_t)value->i64;

			LOG_INF("Set _light_thresh to: %lld", _light_thresh);
			wake_system_thread();
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	if (strcmp(key, "TEMP_THRESH") == 0) {
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_FLOAT) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* Only update if value has changed */
		if (_temp_thresh == (float)value->f) {
			LOG_DBG("Received TEMP_THRESH already matches local value.");
		} else {
			_temp_thresh = (float)value->f;

			int32_t temp_conversion = (int32_t)(_temp_thresh * 100);
			_temp_thresh_sensorval.val1 = temp_conversion / 100;
			_temp_thresh_sensorval.val2 = (temp_conversion % 100) * 10000;

			LOG_INF("Set _temp_thresh_sensorval to: %d.%d",
				_temp_thresh_sensorval.val1, _temp_thresh_sensorval.val2);
			wake_system_thread();
		}
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	/* If the setting is not recognized, we should return an error */
	return GOLIOTH_SETTINGS_KEY_NOT_RECOGNIZED;
}

int app_settings_init(struct golioth_client *state_client)
{
	client = state_client;
	int err = app_settings_register(client);

	return err;
}

int app_settings_observe(void)
{
	int err = golioth_settings_observe(client);
	if (err) {
		LOG_ERR("Failed to observe settings: %d", err);
	}

	return err;
}

int app_settings_register(struct golioth_client *settings_client)
{
	int err = golioth_settings_register_callback(settings_client, on_setting);

	if (err) {
		LOG_ERR("Failed to register settings callback: %d", err);
	}

	return err;
}

