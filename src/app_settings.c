/*
 * Copyright (c) 2022-2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <golioth/settings.h>
#include <zephyr/drivers/sensor.h>
#include "app_settings.h"
#include "main.h"

#define LOOP_DELAY_S_MAX 43200
#define LOOP_DELAY_S_MIN 0
#define LIGHT_THRESH_MAX 1000000
#define LIGHT_THRESH_MIN 0

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

static enum golioth_settings_status on_loop_delay_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (_loop_delay_s == new_value) {
		LOG_DBG("LOOP_DELAY_S already matches local value.");
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	_loop_delay_s = new_value;
	LOG_INF("Set loop delay to %i seconds", new_value);
	wake_system_thread();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_light_auto_setting(bool new_value, void *arg)
{
	/* Only update if value has changed */
	if (_light_auto == new_value) {
		LOG_DBG("LIGHT_AUTO already matches local value.");
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	_light_auto = new_value;
	LOG_INF("Set LIGHT_AUTO to %s", new_value ? "true" : "false");
	wake_system_thread();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_temp_auto_setting(bool new_value, void *arg)
{
	/* Only update if value has changed */
	if (_temp_auto == new_value) {
		LOG_DBG("TEMP_AUTO already matches local value.");
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	_temp_auto = new_value;
	LOG_INF("Set TEMP_AUTO to %s", new_value ? "true" : "false");
	wake_system_thread();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_light_thresh_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (_light_thresh == new_value) {
		LOG_DBG("LIGHT_THRESH already matches local value.");
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	_light_thresh = new_value;
	LOG_INF("Set light threshold to %i seconds", new_value);
	wake_system_thread();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_temp_thresh_setting(float new_value, void *arg)
{
	/* Only update if value has changed */
	if (_temp_thresh == new_value) {
		LOG_DBG("TEMP_THRESH already matches local value.");
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	_temp_thresh = new_value;

	int32_t temp_conversion = (int32_t)(_temp_thresh * 100);
	_temp_thresh_sensorval.val1 = temp_conversion / 100;
	_temp_thresh_sensorval.val2 = (temp_conversion % 100) * 10000;

	LOG_INF("Set _temp_thresh_sensorval to: %d.%d",
		_temp_thresh_sensorval.val1, _temp_thresh_sensorval.val2);
	wake_system_thread();
	return GOLIOTH_SETTINGS_SUCCESS;
}

void app_settings_register(struct golioth_client *client)
{
	int err;
	struct golioth_settings *settings = golioth_settings_init(client);

	err = golioth_settings_register_int_with_range(settings,
							   "LOOP_DELAY_S",
							   LOOP_DELAY_S_MIN,
							   LOOP_DELAY_S_MAX,
							   on_loop_delay_setting,
							   NULL);

	if (err) {
		LOG_ERR("Failed to register loop settings callback: %d", err);
	}

	err = golioth_settings_register_bool(settings,
						 "LIGHT_AUTO",
						 on_light_auto_setting,
						 NULL);

	if (err) {
		LOG_ERR("Failed to register light auto settings callback: %d", err);
	}

	err = golioth_settings_register_bool(settings,
						 "TEMP_AUTO",
						 on_temp_auto_setting,
						 NULL);

	if (err) {
		LOG_ERR("Failed to register vent auto settings callback: %d", err);
	}

	err = golioth_settings_register_int_with_range(settings,
						       "LIGHT_THRESH",
						       LIGHT_THRESH_MIN,
						       LIGHT_THRESH_MAX,
						       on_light_thresh_setting,
						       NULL);

	if (err) {
		LOG_ERR("Failed to register light threshold settings callback: %d", err);
	}

	err = golioth_settings_register_float(settings,
					      "TEMP_THRESH",
					      on_temp_thresh_setting,
					      NULL);

	if (err) {
		LOG_ERR("Failed to register temp threshold settings callback: %d", err);
	}
}

