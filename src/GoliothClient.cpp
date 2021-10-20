/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GoliothClient.h"

GoliothClient::GoliothClient()
{
  this->mqtt_client = new MqttClient(NULL);
}

/*GoliothClient::GoliothClient(Client *net)
{
  this->net = net;
  this->mqtt_client = new MqttClient(net);
}

GoliothClient::GoliothClient(Client &net) : GoliothClient(&net)
{
}

GoliothClient::GoliothClient(Client *net, const char *psk_id, const char *psk) : GoliothClient(net)
{
  this->psk_id = psk_id;
  this->psk = psk;
}

GoliothClient::GoliothClient(Client &net, const char *psk_id, const char *psk) : GoliothClient(&net, psk_id, psk)
{
}*/

void GoliothClient::poll()
{
  this->mqtt_client->poll();
}

void onGoliothMessage(int size)
{
  GoliothClient::getInstance()->onMessage(size);
}

bool GoliothClient::connect()
{
  this->mqtt_client->setUsernamePassword(this->psk_id, this->psk);
  this->mqtt_client->setId(this->psk_id);

  bool connected = this->mqtt_client->connect(GOLIOTH_MQTT_HOST, GOLIOTH_MQTT_PORT);
  if (connected)
  {
    this->mqtt_client->onMessage(onGoliothMessage);
  }
  return connected;
}

void GoliothClient::onMessage(int size)
{
  String topic = this->mqtt_client->messageTopic();
  String payload = this->mqtt_client->readString();
  if (!topic.startsWith("/"))
  {
    topic = "/" + topic;
  }
  if (topic.startsWith("/hello") && this->_onHello != NULL)
  {
    this->_onHello(payload);
  }
  if (topic.startsWith("/echo") && this->_onHello != NULL)
  {
    this->_onEcho(payload);
  }
  if (topic.startsWith(LIGHTDB_STATE_PREFIX) && this->_onLightDBMessage != NULL)
  {
    topic.replace(LIGHTDB_STATE_PREFIX, "");
    this->_onLightDBMessage(topic, payload);
  }
}

bool GoliothClient::connected()
{
  return this->mqtt_client->connected();
}

String GoliothClient::joinPath(String prefix, const char *topic)
{
  String path = String(topic);
  if (path.startsWith("/"))
  {
    path = path.substring(1);
  }
  if (path.length() == 0)
  {
    return prefix + "#";
  }
  return prefix + path;
}

void GoliothClient::listenHello()
{
  this->mqtt_client->subscribe("/hello");
}

void GoliothClient::listenEcho()
{
  this->mqtt_client->subscribe("/echo");
}

void GoliothClient::sendEcho(const char *value)
{
  this->mqtt_client->beginMessage("/echo");
  this->mqtt_client->print(value);
  this->mqtt_client->endMessage();
}

void GoliothClient::listenLightDBStateAtPath(const char *path)
{
  this->mqtt_client->subscribe(this->joinPath(LIGHTDB_STATE_PREFIX, path));
}

void GoliothClient::setLightDBStateAtPath(const char *path, const char *value)
{
  this->mqtt_client->beginMessage(this->joinPath(LIGHTDB_STATE_PREFIX, path));
  this->mqtt_client->print(value);
  this->mqtt_client->endMessage();
}

void GoliothClient::deleteLightDBStateAtPath(const char *path)
{
  this->mqtt_client->beginMessage(this->joinPath(LIGHTDB_STATE_PREFIX, path));
  this->mqtt_client->print("");
  this->mqtt_client->endMessage();
}

void GoliothClient::sendLightDBStream(const char *path, const char *value)
{
  this->mqtt_client->beginMessage(this->joinPath(LIGHTDB_STREAM_PREFIX, path));
  this->mqtt_client->print(value);
  this->mqtt_client->endMessage();
}

void GoliothClient::sendLog(const char *level, const char *value)
{
  this->mqtt_client->beginMessage("/logs/");
  this->mqtt_client->print("{ \"level\": \"" + String(level) + "\", \"msg\": \"" + String(value) + "\" }");
  this->mqtt_client->endMessage();
}

void GoliothClient::logDebug(const char *value)
{
  this->sendLog("DEBUG", value);
}

void GoliothClient::logDebug(String value)
{
  this->logDebug(value.c_str());
}

void GoliothClient::logInfo(const char *value)
{
  this->sendLog("INFO", value);
}

void GoliothClient::logInfo(String value)
{
  this->logInfo(value.c_str());
}

void GoliothClient::logWarn(const char *value)
{
  this->sendLog("WARN", value);
}

void GoliothClient::logWarn(String value)
{
  this->logWarn(value.c_str());
}

void GoliothClient::logError(const char *value)
{
  this->sendLog("ERROR", value);
}

void GoliothClient::logError(String value)
{
  this->logError(value.c_str());
}