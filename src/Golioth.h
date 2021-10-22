/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef Golioth_h
#define Golioth_h

#include "GoliothClient.h"

#ifndef GOLIOTH_MQTT_HOST
#define GOLIOTH_MQTT_HOST "mqtt.golioth.io"
#endif

#ifndef GOLIOTH_MQTT_PORT
#define GOLIOTH_MQTT_PORT 8883
#endif

#define LIGHTDB_STATE_PREFIX "/.d/"
#define LIGHTDB_STREAM_PREFIX "/.s/"
#define DFU_DESIRED_PREFIX "/.u/desired"
#define DFU_ARTIFACT_DOWNLOAD_PREFIX "/.u/c/"

#endif // Golioth_h