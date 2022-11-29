/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_work, LOG_LEVEL_DBG);

#include <net/golioth/settings.h>
#include <net/golioth/system_client.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include "app_settings.h"
#include "app_state.h"

static struct golioth_client *client;

const struct device *light_sensor = DEVICE_DT_GET(DT_NODELABEL(apds9960));
const struct device *weather_sensor = DEVICE_DT_GET(DT_NODELABEL(bme280));
const struct gpio_dt_spec relay_light = GPIO_DT_SPEC_GET(DT_NODELABEL(relay_0), gpios);
const struct gpio_dt_spec relay_vent = GPIO_DT_SPEC_GET(DT_NODELABEL(relay_1), gpios);

#define SENSOR_JSON_FMT	"{\"light\":{\"int\":%d,\"r\":%d,\"g\":%d,\"b\":%d},\"weather\":{\"tem\":%d.%d,\"pre\":%d.%d,\"hum\":%d.%d}}"

#define RELAY_ON  1
#define RELAY_OFF 0

uint8_t _light_cur_state = 0;
uint8_t _vent_cur_state = 0;

static void set_light_on_off(uint8_t state) {
	if (state > 1) {
		LOG_ERR("Light state out of range [0..1]: %d", state);
		return;
	}
	_light_cur_state = state;
	gpio_pin_set_dt(&relay_light, _light_cur_state);
}

static void set_vent_on_off(uint8_t state) {
	if (state > 1) {
		LOG_ERR("Vent state out of range [0..1]: %d", state);
		return;
	}
	_vent_cur_state = state;
	gpio_pin_set_dt(&relay_vent, _vent_cur_state);
}

int manual_light_on_off(uint8_t state) {
	struct light_settings ls;
	get_light_settings(&ls);

	if (ls.ctrl_auto) {
		return -EBUSY;
	}

	set_light_on_off(state);
	return 0;
}

int manual_vent_on_off(uint8_t state) {
	struct temp_settings ts;
	get_temp_settings(&ts);

	if (ts.ctrl_auto) {
		return -EBUSY;
	}

	set_vent_on_off(state);
	return 0;
}

static int async_error_handler(struct golioth_req_rsp *rsp) {
	if (rsp->err) {
		LOG_ERR("Async task failed: %d", rsp->err);
		return rsp->err;
	}
	return 0;
}

static bool temp_threshold_triggered(struct sensor_value *tem_sensor, struct
		sensor_value *thresh) {

	if (tem_sensor->val1 > thresh->val1) {
		return true;
	}

	if ((tem_sensor->val1 == thresh->val1) && (tem_sensor->val2 > thresh->val2))
	{
		return true;
	}

	return false;
}

static void sensor_work_handler(struct k_work *work) {
	struct sensor_value intensity, red, green, blue, tem, pre, hum;
	int err;
	char json_buf[256];

	/* Read all sensors */
	err = sensor_sample_fetch(light_sensor);
	if (err) {
		LOG_ERR("Light sensor fetch failed: %d", err);
		return;
	}

	sensor_channel_get(light_sensor, SENSOR_CHAN_LIGHT, &intensity);
	sensor_channel_get(light_sensor, SENSOR_CHAN_RED, &red);
	sensor_channel_get(light_sensor, SENSOR_CHAN_GREEN, &green);
	sensor_channel_get(light_sensor, SENSOR_CHAN_BLUE, &blue);
	LOG_INF("Light: %d; r=%d, g=%d, b=%d", intensity.val1, red.val1,
			green.val1, blue.val1);

	err = sensor_sample_fetch(weather_sensor);
	if (err) {
		LOG_ERR("Weather sensor fetch failed: %d", err);
		return;
	}

	sensor_channel_get(weather_sensor, SENSOR_CHAN_AMBIENT_TEMP, &tem);
	sensor_channel_get(weather_sensor, SENSOR_CHAN_PRESS, &pre);
	sensor_channel_get(weather_sensor, SENSOR_CHAN_HUMIDITY, &hum);
	LOG_INF("Weather: tem=%d.%d, pre=%d.%d, hum=%d.%d", tem.val1, tem.val2,
			pre.val1, pre.val2, hum.val1, hum.val2);

	/* Send sensor data to Golioth */
	snprintk(json_buf, sizeof(json_buf), SENSOR_JSON_FMT, intensity.val1, red.val1,
			green.val1, blue.val1, tem.val1, tem.val2, pre.val1,
			pre.val2, hum.val1, hum.val2);

	err = golioth_stream_push_cb(client, "sensor",
			GOLIOTH_CONTENT_FORMAT_APP_JSON,
			json_buf, strlen(json_buf),
			async_error_handler, NULL);
	if (err) LOG_ERR("Failed to send sensor data to Golioth: %d", err);

	struct light_settings ls;
	get_light_settings(&ls);

	if (ls.ctrl_auto) {
		if (intensity.val1 < ls.thresh) {
			set_light_on_off(RELAY_ON);
		}
		else {
			set_light_on_off(RELAY_OFF);
		}
	}

	struct temp_settings ts;
	get_temp_settings(&ts);

	if (ts.ctrl_auto) {
		if (temp_threshold_triggered(&tem, &ts.tem)) {
			set_vent_on_off(RELAY_ON);
		}
		else {
			set_vent_on_off(RELAY_OFF);
		}
	}
}

K_WORK_DEFINE(sensor_work, sensor_work_handler);

void app_work_observe(void) {
	int err = golioth_lightdb_observe_cb(client, APP_STATE_DESIRED_ENDP,
			GOLIOTH_CONTENT_FORMAT_APP_JSON, app_state_desired_handler, NULL);
	if (err) {
	   LOG_WRN("failed to observe lightdb path: %d", err);
	}
}

void app_work_init(struct golioth_client* work_client) {
	int err;

	client = work_client;

	/* Initialize relays */
	err = gpio_pin_configure_dt(&relay_light, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Unable to configure relay_light");
	}

	err = gpio_pin_configure_dt(&relay_vent, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Unable to configure relay_vent");
	}

	/* Initialize LightDB State handlers */
	app_state_init(client);
}

void app_work_submit(void) {
	k_work_submit(&sensor_work);
}

