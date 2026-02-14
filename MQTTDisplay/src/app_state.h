#pragma once

#include "../config.h"

enum ConnState : uint8_t {
  CONN_WIFI_WARMUP,
  CONN_WIFI_BEGIN,
  CONN_WIFI_WAIT,
  CONN_WIFI_BACKOFF,
  CONN_MQTT_ANIM,
  CONN_MQTT_TRY_ONCE,
  CONN_MQTT_FAIL_SHOW,
  CONN_MQTT_SESSION_BACKOFF,
  CONN_OK
};

enum ScreenMode { SCREEN_TEMP, SCREEN_HUM, SCREEN_CLOCK };

struct WipeAnim {
  bool active = false;
  uint8_t from[8][12];
  uint8_t to[8][12];
  uint8_t out[8][12];
  uint8_t step = 0;
  unsigned long nextStepMs = 0;
  unsigned long stepIntervalMs = 0;
  ScreenMode nextMode = SCREEN_TEMP;

  static const uint8_t order[12];
};

struct AppState {
  ArduinoLEDMatrix matrix;
  WiFiClient wifiClient;
  MqttClient mqttClient;

  ConnState connState = CONN_WIFI_WARMUP;
  unsigned long stateStartMs = 0;
  unsigned long lastAnimTickMs = 0;
  unsigned long mqttSessionStartMs = 0;
  unsigned long mqttBackoffUntilMs = 0;
  unsigned long mqttBackoffMs = 0;
  unsigned long mqttLastTryMs = 0;
  unsigned long wifiBackoffUntilMs = 0;
  unsigned long wifiBackoffMs = 0;

  unsigned long screenStartMs = 0;
  unsigned long lastUiTickMs = 0;
  unsigned long lastRenderMs = 0;
  ScreenMode mode = SCREEN_TEMP;

  int wifiAnimStep = 0;
  unsigned long mqttPhase = 0;

  bool timeValid = false;

  float lastTemp = NAN;
  float lastHum  = NAN;
  unsigned long lastTempUpdateMs = 0;
  unsigned long lastHumUpdateMs = 0;
  bool tempUpdatedSincePersist = false;
  bool humUpdatedSincePersist = false;
  unsigned long lastPersistMs = 0;
  unsigned long showMs = SHOW_MS;
  unsigned long uiTickMs = UI_TICK_MS;
  unsigned long displayRefreshMs = DISPLAY_REFRESH_MS;

  bool simTempEnabled = false;
  bool simHumEnabled = false;
  float simTemp = NAN;
  float simHum = NAN;
  unsigned long simLastRefreshMs = 0;

  uint8_t frame[8][12];

  bool displayOffForSchedule = false;
  int lastBlinkMinute = -1;
  unsigned long ledPulseUntilMs = 0;

  WipeAnim wipe;

  AppState() : mqttClient(wifiClient) {}
};

extern AppState app;

void initAppState(AppState& s);
