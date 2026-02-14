#include "mqtt_client.h"

static AppState* gAppState = nullptr;

void mqttBindState(AppState& s) {
  gAppState = &s;
}

void onMqttMessage(int) {
  if (!gAppState) return;
  AppState& s = *gAppState;

  const String topicStr = s.mqttClient.messageTopic();
  const char* topic = topicStr.c_str();

  char buf[32];
  size_t n = 0;
  while (s.mqttClient.available() && n < sizeof(buf) - 1) {
    buf[n++] = (char)s.mqttClient.read();
  }
  buf[n] = '\0';

  while (n && (buf[n-1] == '\r' || buf[n-1] == '\n' || buf[n-1] == ' ' || buf[n-1] == '\t')) {
    buf[--n] = '\0';
  }

  char* endPtr = nullptr;
  float v = strtof(buf, &endPtr);
  if (endPtr == buf) return;

  if (strcmp(topic, TOPIC_TEMP) == 0) {
    if (s.simTempEnabled) return;
    if (v < TEMP_MIN_C || v > TEMP_MAX_C) return;
    bool changed = isnan(s.lastTemp) || fabsf(s.lastTemp - v) >= PERSIST_DELTA;
    s.lastTemp = v;
    s.lastTempUpdateMs = millis();
    if (changed) s.tempUpdatedSincePersist = true;
  } else if (strcmp(topic, TOPIC_HUM) == 0) {
    if (s.simHumEnabled) return;
    if (v < HUM_MIN || v > HUM_MAX) return;
    bool changed = isnan(s.lastHum) || fabsf(s.lastHum - v) >= PERSIST_DELTA;
    s.lastHum = v;
    s.lastHumUpdateMs = millis();
    if (changed) s.humUpdatedSincePersist = true;
  }
}

void mqttConfigureOnce(AppState& s) {
  s.mqttClient.setId(MQTT_CLIENT_ID);
  s.mqttClient.setCleanSession(true);
  s.mqttClient.setKeepAliveInterval(30UL * 1000UL);
  s.mqttClient.setConnectionTimeout(MQTT_CONNECT_TIMEOUT_MS);
#if USE_MQTT_AUTH
  s.mqttClient.setUsernamePassword(MQTT_USER, MQTT_PASS);
#endif

  s.mqttClient.beginWill(TOPIC_STATUS, MQTT_STATUS_RETAIN, MQTT_STATUS_QOS);
  s.mqttClient.print(MQTT_STATUS_OFFLINE);
  s.mqttClient.endWill();
}

bool mqttSubscribeOnce(AppState& s) {
  bool ok1 = s.mqttClient.subscribe(TOPIC_TEMP, MQTT_SUB_QOS);
  bool ok2 = s.mqttClient.subscribe(TOPIC_HUM, MQTT_SUB_QOS);
  return ok1 && ok2;
}

void mqttPublishStatusOnline(AppState& s) {
  s.mqttClient.beginMessage(TOPIC_STATUS, MQTT_STATUS_RETAIN, MQTT_STATUS_QOS);
  s.mqttClient.print(MQTT_STATUS_ONLINE);
  s.mqttClient.endMessage();
}
