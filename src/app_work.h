/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_WORK_H__
#define __APP_WORK_H__

struct relay_state {
	int8_t light;
	int8_t vent;
};

void app_work_init(struct golioth_client* work_client);
void app_work_observe(void);
void app_work_submit(void);
int manual_light_on_off(uint8_t state);
int manual_vent_on_off(uint8_t state);
void get_relay_state(struct relay_state *r_state);

#endif /* __APP_WORK_H__ */
