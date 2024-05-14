/*
 * Copyright (c) 2022-2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Process changes received from the Golioth Settings Service and return a code
 * to Golioth to indicate the success or failure of the update.
 *
 * In this demonstration, the device looks for the `LOOP_DELAY_S` key from the
 * Settings Service and uses this value to determine the delay between sensor
 * reads (the period of sleep in the loop of `main.c`.
 *
 * https://docs.golioth.io/firmware/zephyr-device-sdk/device-settings-service
 */

#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__

#include <stdint.h>
#include <golioth/client.h>
#include <zephyr/drivers/sensor.h>

struct light_settings {
	bool ctrl_auto;
	int64_t thresh;
};

struct temp_settings {
	bool ctrl_auto;
	struct sensor_value tem;
};

int32_t get_loop_delay_s(void);
void app_settings_register(struct golioth_client *client);

void get_light_settings(struct light_settings *ls);
void get_temp_settings(struct temp_settings *ts);

#endif /* __APP_SETTINGS_H__ */

