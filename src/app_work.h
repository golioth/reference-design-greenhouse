/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_WORK_H__
#define __APP_WORK_H__

void app_work_init(struct golioth_client* work_client);
enum golioth_settings_status app_work_settings(const char *key, const struct
		golioth_settings_value *value);
void app_work_submit(void);

#endif /* __APP_WORK_H__ */
