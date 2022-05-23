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

void GoliothClient::poll()
{
  if (this->mqtt_client == NULL)
  {
    return;
  }
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
  MqttClient *client = this->mqtt_client;
  String topic = client->messageTopic();
  if (!topic.startsWith("/"))
  {
    topic = "/" + topic;
  }
  if (topic.startsWith("/hello") && this->_onHello != NULL)
  {
    String payload = "";
    while (client->available())
    {
      payload += client->readString();
    }
    this->_onHello(payload);
  }
  if (topic.startsWith("/echo") && this->_onHello != NULL)
  {
    String payload = "";
    while (client->available())
    {
      payload += client->readString();
    }
    this->_onEcho(payload);
  }
  if (topic.startsWith(LIGHTDB_STATE_PREFIX) && this->_onLightDBMessage != NULL)
  {
    topic.replace(LIGHTDB_STATE_PREFIX, "");
    String payload = "";
    while (client->available())
    {
      payload += client->readString();
    }
    this->_onLightDBMessage(topic, payload);
  }
  if (topic.startsWith("/.u/desired") && this->_onDesiredVersionChanged != NULL)
  {
    String payload = "";
    while (client->available())
    {
      payload += client->readString();
    }
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload);
    JsonArray components = doc["components"].as<JsonArray>();
    for (JsonVariant c : components)
    {
      JsonObject component = c.as<JsonObject>();
      String version = "";
      String pkg = "main";
      String hash = "";
      for (JsonPair kv : component)
      {
        String value = kv.value().as<String>();
        if (strcmp(kv.key().c_str(), "package") == 0)
        {
          pkg = value;
        }
        if (strcmp(kv.key().c_str(), "hash") == 0)
        {
          hash = value;
        }
        if (strcmp(kv.key().c_str(), "version") == 0)
        {
          version = value;
        }
      }
      this->_onDesiredVersionChanged(pkg, version, hash);
    }
  }
  if (topic.startsWith("/.u/c/") && this->_onDownloadArtifact != NULL)
  {
    topic.replace("/.u/c/", "");
    int atIndex = topic.indexOf("@");
    String pkg = topic.substring(0, atIndex);
    String version = topic.substring(atIndex + 1);
    String payload = "";
    uint8_t buf[1024];
    int totalRead = 0;
    int remaining = size;
    this->_onDownloadArtifact(pkg, version, buf, 0, totalRead, size);
    while (remaining > 0)
    {
      size_t bytesRead = client->readBytes(buf, 1024);
      if (!bytesRead) // reading a byte with timeout
        break;
      totalRead += bytesRead;
      this->_onDownloadArtifact(pkg, version, buf, bytesRead, totalRead, size);
      remaining--;
    }
  }
  if (topic.startsWith(RPC_PREFIX) && this->function_registered)
  {
    String payload = "";
    while (client->available())
    {
      payload += client->readString();
    }
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload);
    String method = doc["method"].as<String>();
    for (int i = 0; i < this->num_registered_functions; i++)
    {
      if (this->callbacks[i].method == method)
      {
        String id = doc["id"].as<String>();
        JsonArray params = doc["params"].as<JsonArray>();
        this->callbacks[i].callback(id, params);
      }
    }
  }
}

bool GoliothClient::connected()
{
  if (this->mqtt_client == NULL)
  {
    return false;
  }
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

void GoliothClient::onRemoteFunction(const char *name, void(*callback)(String callID, JsonArray params))
{
  if (!this->function_registered){
    this->mqtt_client->subscribe(RPC_PREFIX);
    this->function_registered = true;
  }
  this->callbacks[this->num_registered_functions] = goliothCallback{
    method: String(name),
    callback: callback
  };
  this->num_registered_functions++;
}

void GoliothClient::ackRemoteFunction(String id, uint status_code)
{
  this->mqtt_client->beginMessage(this->joinPath(RPC_FULL_PREFIX, id.c_str()));
  this->mqtt_client->print(String(status_code, 10));
  this->mqtt_client->endMessage();
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

void GoliothClient::listenDesiredVersion()
{
  this->mqtt_client->subscribe("/.u/desired");
}

void GoliothClient::downloadArtifact(const char *package, const char *version)
{
  this->mqtt_client->unsubscribe("/.u/c/" + String(package) + "@" + String(version));
  this->mqtt_client->subscribe("/.u/c/" + String(package) + "@" + String(version));
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