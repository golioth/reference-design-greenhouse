/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_STATE_H__
#define __APP_STATE_H__

#include <net/golioth/system_client.h>

#define APP_STATE_DESIRED_ENDP "desired"

int app_state_desired_handler(struct golioth_req_rsp *rsp);

#endif /* __APP_STATE_H__ */
