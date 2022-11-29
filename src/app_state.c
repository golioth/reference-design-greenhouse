/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_state, LOG_LEVEL_DBG);

#include <net/golioth/system_client.h>
#include <zephyr/data/json.h>
#include "json_helper.h"

#include "app_state.h"

int app_state_desired_handler(struct golioth_req_rsp *rsp) {
	if (rsp->err) {
		LOG_ERR("Failed to receive '%s' endpoint: %d", APP_STATE_DESIRED_ENDP, rsp->err);
		return rsp->err;
	}

	LOG_HEXDUMP_DBG(rsp->data, rsp->len, APP_STATE_DESIRED_ENDP);

	struct greenhouse_state parsed_state;

	int ret = json_obj_parse((char *)rsp->data, rsp->len,
			greenhouse_state_descr, ARRAY_SIZE(greenhouse_state_descr),
			&parsed_state);

	if (ret < 0) {
		LOG_ERR("Error parsing desired values: %d", -1);
		return ret;
	}

	if (ret & 1<<0) {
		if ((parsed_state.light >= 0) && (parsed_state.light < 2)) {
			LOG_DBG("Validated desired Light setting: %d", parsed_state.light);
		}
		else if (parsed_state.light == -1) {
			LOG_DBG("No change requested for Light");
		}
		else {
			LOG_ERR("Invalid desired Light value: %d", parsed_state.light);
		}
	}
	if (ret & 1<<1) {
		if ((parsed_state.vent >= 0) && (parsed_state.vent < 2)) {
			LOG_DBG("Validated desired Vent setting: %d", parsed_state.vent);
		}
		else if (parsed_state.vent == -1) {
			LOG_DBG("No change requested for Vent");
		}
		else {
			LOG_ERR("Invalid desired Vent value: %d", parsed_state.vent);
		}
	}

	return 0;
}

