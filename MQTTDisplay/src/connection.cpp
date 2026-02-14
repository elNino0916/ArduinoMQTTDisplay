#include "connection.h"
#include "mqtt_client.h"
#include "ui.h"

static unsigned long nextBackoff(unsigned long current, unsigned long baseMs, unsigned long maxMs) {
  if (current == 0) return baseMs;
  unsigned long next = current * 2;
  if (next < current) next = maxMs;
  if (next > maxMs) next = maxMs;
  return next;
}

void goState(AppState& s, ConnState next, unsigned long now) {
  s.connState = next;
  s.stateStartMs = now;
  s.lastAnimTickMs = 0;
#if SERIAL_DEBUG
  const char* name = "UNKNOWN";
  switch (next) {
    case CONN_WIFI_WARMUP: name = "CONN_WIFI_WARMUP"; break;
    case CONN_WIFI_BEGIN: name = "CONN_WIFI_BEGIN"; break;
    case CONN_WIFI_WAIT: name = "CONN_WIFI_WAIT"; break;
    case CONN_WIFI_BACKOFF: name = "CONN_WIFI_BACKOFF"; break;
    case CONN_MQTT_ANIM: name = "CONN_MQTT_ANIM"; break;
    case CONN_MQTT_TRY_ONCE: name = "CONN_MQTT_TRY_ONCE"; break;
    case CONN_MQTT_FAIL_SHOW: name = "CONN_MQTT_FAIL_SHOW"; break;
    case CONN_MQTT_SESSION_BACKOFF: name = "CONN_MQTT_SESSION_BACKOFF"; break;
    case CONN_OK: name = "CONN_OK"; break;
    default: break;
  }
  Serial.print("State -> ");
  Serial.println(name);
#endif
}

void connectionTick(AppState& s, unsigned long now) {
  if (s.connState == CONN_OK && WiFi.status() != WL_CONNECTED) {
    s.mqttClient.stop();
    s.wifiAnimStep = 0;
    goState(s, CONN_WIFI_WARMUP, now);
    return;
  }

  if (s.connState == CONN_OK && !s.mqttClient.connected()) {
    s.mqttPhase = 0;
    s.mqttSessionStartMs = now;
    s.mqttLastTryMs = 0;
    goState(s, CONN_MQTT_ANIM, now);
    return;
  }

  auto wifiAnim = [&]() {
    if (now - s.lastAnimTickMs >= WIFI_CONNECT_TICK_MS) {
      s.lastAnimTickMs = now;
      drawWifiBarsAnim(s, s.wifiAnimStep % 5);
      s.wifiAnimStep++;
    }
  };

  auto mqttAnim = [&]() {
    if (now - s.lastAnimTickMs >= MQTT_CONNECT_TICK_MS) {
      s.lastAnimTickMs = now;
      drawMqttAnimSmooth(s, s.mqttPhase++);
    }
  };

  switch (s.connState) {
    case CONN_WIFI_WARMUP: {
      if (s.stateStartMs == 0) {
        s.stateStartMs = now;
        drawWifiBarsAnim(s, 0);
      }
      wifiAnim();
      if (now - s.stateStartMs >= WIFI_WARMUP_ANIM_MS) goState(s, CONN_WIFI_BEGIN, now);
      break;
    }

    case CONN_WIFI_BEGIN: {
      if (USE_STATIC_IP) {
        WiFi.config(WIFI_STATIC_IP, WIFI_DNS, WIFI_GATEWAY, WIFI_SUBNET);
      }
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      goState(s, CONN_WIFI_WAIT, now);
      break;
    }

    case CONN_WIFI_WAIT: {
      wifiAnim();

      if (WiFi.status() == WL_CONNECTED) {
        s.wifiBackoffMs = 0;
        mqttConfigureOnce(s);

        s.mqttPhase = 0;
        s.mqttSessionStartMs = now;
        s.mqttLastTryMs = 0;
        goState(s, CONN_MQTT_ANIM, now);
        drawMqttAnimSmooth(s, 0);

        break;
      }

      if (now - s.stateStartMs >= WIFI_TIMEOUT_MS) {
        drawBigX(s, now);
        s.wifiBackoffMs = nextBackoff(s.wifiBackoffMs, WIFI_BACKOFF_BASE_MS, WIFI_BACKOFF_MAX_MS);
        s.wifiBackoffUntilMs = now + s.wifiBackoffMs;
        goState(s, CONN_WIFI_BACKOFF, now);
      }
      break;
    }

    case CONN_WIFI_BACKOFF: {
      drawBigX(s, now);
      if ((long)(now - s.wifiBackoffUntilMs) >= 0) {
        s.wifiAnimStep = 0;
        goState(s, CONN_WIFI_WARMUP, now);
        drawWifiBarsAnim(s, 0);
      }
      break;
    }

    case CONN_MQTT_ANIM: {
      mqttAnim();

      if (MQTT_TOTAL_TIMEOUT_MS > 0 && (now - s.mqttSessionStartMs >= MQTT_TOTAL_TIMEOUT_MS)) {
        drawBigX(s, now);
        s.mqttBackoffMs = nextBackoff(s.mqttBackoffMs, MQTT_SESSION_BACKOFF_BASE_MS, MQTT_SESSION_BACKOFF_MAX_MS);
        s.mqttBackoffUntilMs = now + s.mqttBackoffMs;
        goState(s, CONN_MQTT_SESSION_BACKOFF, now);
        break;
      }

      if (now - s.stateStartMs >= MQTT_ANIM_RUN_MS) {
        if (s.mqttLastTryMs == 0 || now - s.mqttLastTryMs >= MQTT_TRY_INTERVAL_MS) {
          s.mqttLastTryMs = now;
          bool ok = s.mqttClient.connect(MQTT_BROKER, MQTT_PORT);
          if (ok) {
            bool subOk = mqttSubscribeOnce(s);
            if (subOk) {
              mqttPublishStatusOnline(s);
              s.mqttBackoffMs = 0;
              goState(s, CONN_OK, now);
            } else {
              s.mqttClient.stop();
            }
          }
        }
      }
      break;
    }

    case CONN_MQTT_TRY_ONCE: {
      bool ok = s.mqttClient.connect(MQTT_BROKER, MQTT_PORT);
      if (ok) {
        bool subOk = mqttSubscribeOnce(s);
        if (subOk) {
          mqttPublishStatusOnline(s);
          s.mqttBackoffMs = 0;
          goState(s, CONN_OK, now);
        } else {
          s.mqttClient.stop();
          drawBigX(s, now);
          goState(s, CONN_MQTT_FAIL_SHOW, now);
        }
      } else {
        drawBigX(s, now);
        goState(s, CONN_MQTT_FAIL_SHOW, now);
      }
      break;
    }

    case CONN_MQTT_FAIL_SHOW: {
      drawBigX(s, now);
      if (now - s.stateStartMs >= MQTT_FAIL_SHOW_MS) {
        s.mqttBackoffMs = nextBackoff(s.mqttBackoffMs, MQTT_SESSION_BACKOFF_BASE_MS, MQTT_SESSION_BACKOFF_MAX_MS);
        s.mqttBackoffUntilMs = now + s.mqttBackoffMs;
        goState(s, CONN_MQTT_SESSION_BACKOFF, now);
      }
      break;
    }

    case CONN_MQTT_SESSION_BACKOFF: {
      drawBigX(s, now);
      if ((long)(now - s.mqttBackoffUntilMs) >= 0) {
        if (WiFi.status() != WL_CONNECTED) {
          s.wifiAnimStep = 0;
          goState(s, CONN_WIFI_WARMUP, now);
          drawWifiBarsAnim(s, 0);
        } else {
          s.mqttPhase = 0;
          s.mqttSessionStartMs = now;
          goState(s, CONN_MQTT_ANIM, now);
          drawMqttAnimSmooth(s, 0);
        }
      }
      break;
    }

    case CONN_OK:
    default:
      break;
  }
}
