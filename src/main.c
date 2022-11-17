/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_greenhouse, LOG_LEVEL_DBG);

#include <net/golioth/fw.h>
#include <net/golioth/settings.h>
#include <net/golioth/system_client.h>
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>
#include "app_dfu.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

K_SEM_DEFINE(connected, 0, 1);

static int32_t _loop_delay_s = 60;
static k_tid_t _system_thread = 0;

static const struct gpio_dt_spec golioth_led = GPIO_DT_SPEC_GET(
		DT_ALIAS(golioth_led), gpios);
static const struct gpio_dt_spec user_btn = GPIO_DT_SPEC_GET(
		DT_ALIAS(sw1), gpios);
static struct gpio_callback button_cb_data;
const struct gpio_dt_spec relay0 = GPIO_DT_SPEC_GET(DT_NODELABEL(relay_0), gpios);
const struct gpio_dt_spec relay1 = GPIO_DT_SPEC_GET(DT_NODELABEL(relay_1), gpios);
const struct device *light_sensor = DEVICE_DT_GET(DT_NODELABEL(apds9960));
const struct device *weather_sensor = DEVICE_DT_GET(DT_NODELABEL(bme280));

#define JSON_FMT	"{\"light\":{\"int\":%d,\"r\":%d,\"g\":%d,\"b\":%d},\"weather\":{\"tem\":%d.%d,\"pre\":%d.%d,\"hum\":%d.%d}}"

static struct k_work sensor_work;

enum golioth_settings_status on_setting(
		const char *key,
		const struct golioth_settings_value *value)
{
	LOG_DBG("Received setting: key = %s, type = %d", key, value->type);
	if (strcmp(key, "LOOP_DELAY_S") == 0) {
		/* This setting is expected to be numeric, return an error if it's not */
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_INT64) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* This setting must be in range [1, 100], return an error if it's not */
		if (value->i64 < 1 || value->i64 > 100) {
			return GOLIOTH_SETTINGS_VALUE_OUTSIDE_RANGE;
		}

		/* Setting has passed all checks, so apply it to the loop delay */
		_loop_delay_s = (int32_t)value->i64;
		LOG_INF("Set loop delay to %d seconds", _loop_delay_s);

		k_wakeup(_system_thread);
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	/* If the setting is not recognized, we should return an error */
	return GOLIOTH_SETTINGS_KEY_NOT_RECOGNIZED;
}

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);

	app_dfu_observe();

	int err = golioth_settings_register_callback(client, on_setting);

	if (err) {
		LOG_ERR("Failed to register settings callback: %d", err);
	}
}

void button_pressed(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	LOG_DBG("Button pressed at %d", k_cycle_get_32());
	k_wakeup(_system_thread);
}

static int async_error_handler(struct golioth_req_rsp *rsp) {
	if (rsp->err) {
		LOG_ERR("Async task failed: %d", rsp->err);
		return rsp->err;
	}
	return 0;
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
	snprintk(json_buf, sizeof(json_buf), JSON_FMT, intensity.val1, red.val1,
			green.val1, blue.val1, tem.val1, tem.val2, pre.val1,
			pre.val2, hum.val1, hum.val2);

	err = golioth_stream_push_cb(client, "sensor",
			GOLIOTH_CONTENT_FORMAT_APP_JSON,
			json_buf, strlen(json_buf),
			async_error_handler, NULL);
	if (err) LOG_ERR("Failed to send sensor data to Golioth: %d", err);
}

void main(void)
{
	int counter = 0;
	int err;

	LOG_DBG("Start Hello sample");

	/* Get system thread id so loop delay change event can wake main */
	_system_thread = k_current_get();

	err = gpio_pin_configure_dt(&golioth_led, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Unable to configure LED for Golioth Logo");
	}

	app_dfu_init(client);

	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
		net_connect();
	}

	client->on_connect = golioth_on_connect;
	golioth_system_client_start();

	k_sem_take(&connected, K_FOREVER);
	gpio_pin_set_dt(&golioth_led, 1);

	app_dfu_report_state_to_golioth();

	/* Set up user button */
	err = gpio_pin_configure_dt(&user_btn, GPIO_INPUT);
	if (err != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
				err, user_btn.port->name, user_btn.pin);
		return;
	}

	err = gpio_pin_interrupt_configure_dt(&user_btn,
	                                      GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
				err, user_btn.port->name, user_btn.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(user_btn.pin));
	gpio_add_callback(user_btn.port, &button_cb_data);

	err = gpio_pin_configure_dt(&relay0, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		LOG_ERR("Unable to configure relay0");
	}

	err = gpio_pin_configure_dt(&relay1, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		LOG_ERR("Unable to configure relay1");
	}

	k_sleep(K_SECONDS(1));

	k_work_init(&sensor_work, sensor_work_handler);

	while (true) {
		LOG_INF("Sending hello! %d", counter);

		err = golioth_send_hello(client);
		if (err) {
			LOG_WRN("Failed to send hello!");
		}
		++counter;

		k_work_submit(&sensor_work);

		gpio_pin_toggle_dt(&relay0);
		gpio_pin_toggle_dt(&relay1);

		k_sleep(K_SECONDS(_loop_delay_s));
	}
}
