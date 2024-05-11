/*
 * Copyright (c) 2022-2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_SENSORS_H__
#define __APP_SENSORS_H__

/** The `app_sensors.c` file performs the important work of this application
 * which is to read sensor values and report them to the Golioth LightDB Stream
 * as time-series data.
 * https://docs.golioth.io/firmware/zephyr-device-sdk/light-db-stream/
 */

#include <stdint.h>
#include <golioth/client.h>


struct relay_state {
	int8_t light;
	int8_t vent;
};

void app_sensors_set_client(struct golioth_client *sensors_client);
void app_sensors_read_and_stream(void);
void app_sensors_init(void);
int manual_light_on_off(uint8_t state);
int manual_vent_on_off(uint8_t state);
void get_relay_state(struct relay_state *r_state);

#define RELAY_STR_ON "On"
#define RELAY_STR_OFF "Off"

#define LABEL_TEMP	 "Temperature"
#define LABEL_INTENSITY	 "Intensity"
#define LABEL_VENT	 "Ventilation"
#define LABEL_GROWLIGHT	 "Grow Light"
#define LABEL_BATTERY	 "Battery"
#define LABEL_FIRMWARE	 "Firmware"
#define SUMMARY_TITLE	 "Greenhouse:"

/**
 * Each Ostentus slide needs a unique key. You may add additional slides by
 * inserting elements with the name of your choice to this enum.
 */
typedef enum {
	SLIDE_TEMP,
	SLIDE_INTENSITY,
	SLIDE_VENT,
	SLIDE_GROWLIGHT,
#ifdef CONFIG_ALUDEL_BATTERY_MONITOR
	BATTERY_V,
	BATTERY_LVL,
#endif
	FIRMWARE
} slide_key;

#endif /* __APP_SENSORS_H__ */

