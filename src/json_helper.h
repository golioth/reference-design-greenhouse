/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __JSON_HELPER_H_
#define __JSON_HELPER_H_

#include <zephyr/data/json.h>

struct greenhouse_state {
    int8_t light;
    int8_t vent;
};

static const struct json_obj_descr greenhouse_state_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct greenhouse_state, light, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct greenhouse_state, vent, JSON_TOK_NUMBER)
};

#endif

