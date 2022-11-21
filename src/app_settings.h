/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__

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
void get_light_settings(struct light_settings *ls);
void get_temp_settings(struct temp_settings *ts);
int app_register_settings(struct golioth_client *settings_client);

#endif /* __APP_SETTINGS_H__ */
