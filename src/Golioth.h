/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef Golioth_h
#define Golioth_h

#include "GoliothClient.h"

#ifndef GOLIOTH_MQTT_HOST
#define GOLIOTH_MQTT_HOST "mqtt.golioth.net"
#endif

#ifndef GOLIOTH_MQTT_PORT
#define GOLIOTH_MQTT_PORT 8883
#endif

#define LIGHTDB_STATE_PREFIX "/.d/"
#define LIGHTDB_STREAM_PREFIX "/.s/"

#endif // Golioth_h