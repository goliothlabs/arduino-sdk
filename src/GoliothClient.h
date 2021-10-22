/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef GoliothClient_h
#define GoliothClient_h

#include <Arduino.h>
#include <Client.h>
#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
#include "Golioth.h"

class GoliothClient
{
private:
  const char *psk_id;
  const char *psk;
  Client *net;
  MqttClient *mqtt_client;

  void (*_onLightDBMessage)(String path, String payload);
  void (*_onDesiredVersionChanged)(String package, String version, String hash);
  void (*_onDownloadArtifact)(String package, String version, uint8_t *buf, size_t bufSize, int current, int total);
  void (*_onHello)(String name);
  void (*_onEcho)(String payload);

  String joinPath(String prefix, const char *path);
  void sendLog(const char *level, const char *msg);

public:
  GoliothClient();
  static GoliothClient *getInstance()
  {
    static GoliothClient *instance;
    if (!instance)
    {
      instance = new GoliothClient();
    }
    return instance;
  }

  inline void setPSKId(const char *psk_id) { this->psk_id = psk_id; }
  inline void setPSK(const char *psk) { this->psk = psk; }
  inline void setClient(Client &client)
  {
    this->net = &client;
    this->mqtt_client->setClient(client);
  }

  void poll();
  bool connect();
  bool connected();
  void onMessage(int length);

  void listenHello();
  inline void onHello(void (*callback)(String)) { this->_onHello = callback; }
  void listenEcho();
  inline void onEcho(void (*callback)(String)) { this->_onEcho = callback; }
  void sendEcho(const char *value);

  inline void onLightDBMessage(void (*callback)(String, String)) { this->_onLightDBMessage = callback; }
  void listenLightDBStateAtPath(const char *path);
  void setLightDBStateAtPath(const char *path, const char *value);
  void deleteLightDBStateAtPath(const char *path);

  void sendLightDBStream(const char *path, const char *value);

  inline void onDesiredVersionChanged(void (*callback)(String, String, String)) { this->_onDesiredVersionChanged = callback; }
  inline void onDownloadArtifact(void (*callback)(String, String, uint8_t *, size_t, int, int)) { this->_onDownloadArtifact = callback; }
  void listenDesiredVersion();
  void downloadArtifact(const char *package, const char *version);

  void logDebug(const char *msg);
  void logDebug(String msg);
  void logInfo(const char *msg);
  void logInfo(String msg);
  void logWarn(const char *msg);
  void logWarn(String msg);
  void logError(const char *msg);
  void logError(String msg);
};

#endif // GoliothClient_h