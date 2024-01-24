/*
 * Copyright (c) 2022-2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** The `app_work.c` file performs the important work of this application which
 * is to read sensor values and report them to the Golioth LightDB Stream as
 * time-series data.
 *
 * https://docs.golioth.io/firmware/zephyr-device-sdk/light-db-stream/
 */

#ifndef __APP_WORK_H__
#define __APP_WORK_H__

#include <stdint.h>
#include <net/golioth/system_client.h>


struct relay_state {
	int8_t light;
	int8_t vent;
};

void app_work_init(struct golioth_client *work_client);
void app_work_sensor_read(void);
int manual_light_on_off(uint8_t state);
int manual_vent_on_off(uint8_t state);
void get_relay_state(struct relay_state *r_state);

#endif /* __APP_WORK_H__ */

