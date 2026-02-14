#include <cstdint>

#include "config.h"
#include "src/app_state.h"
#include "src/mqtt_client.h"
#include "src/connection.h"
#include "src/time_service.h"
#include "src/schedule.h"
#include "src/ui.h"
#include "src/persist.h"
#include "src/matrix_io.h"
#include <string.h>
#include <stdlib.h>

static bool serialForceScreen = false;
static ScreenMode serial_forced_mode = SCREEN_TEMP;

static void reboot_board() {
#if defined(ARDUINO_ARCH_RENESAS)
  NVIC_SystemReset();
#else
#if SERIAL_DEBUG
  Serial.println("Reboot command not supported on this architecture");
#endif
#endif
}

static void forceScreen(ScreenMode mode, unsigned long now) {
  serialForceScreen = true;
  serial_forced_mode = mode;
  app.mode = mode;
  app.wipe.active = false;
  app.screenStartMs = now;
  app.lastUiTickMs = 0;
}

static void clearForcedScreen(unsigned long now) {
  serialForceScreen = false;
  app.screenStartMs = now;
  app.lastUiTickMs = 0;
}

static void printSerialHelp() {
  Serial.println("Commands:");
  Serial.println("  reboot");
  Serial.println("  factory reset");
  Serial.println("  status");
  Serial.println("  show <temp|hum|clock|auto>");
  Serial.println("  sim temp <value|off>");
  Serial.println("  sim hum <value|off>");
  Serial.println("  sim both <temp> <hum>");
  Serial.println("  sim off");
  Serial.println("  set <show_ms|ui_tick_ms|display_refresh_ms> <value>");
  Serial.println("  get <show_ms|ui_tick_ms|display_refresh_ms|all>");
  Serial.println("  save settings");
  Serial.println("  load settings");
  Serial.println("  help");
}

static void printSerialStatus() {
  Serial.print("ForcedScreen=");
  Serial.print(serialForceScreen ? "1" : "0");
  Serial.print(" Mode=");
  if (serialForceScreen) {
    if (serial_forced_mode == SCREEN_TEMP) Serial.print("temp");
    else if (serial_forced_mode == SCREEN_HUM) Serial.print("hum");
    else Serial.print("clock");
  } else {
    Serial.print("auto");
  }

  Serial.print(" show_ms=");
  Serial.print(app.showMs);
  Serial.print(" ui_tick_ms=");
  Serial.print(app.uiTickMs);
  Serial.print(" display_refresh_ms=");
  Serial.print(app.displayRefreshMs);

  Serial.print(" sim_temp=");
  if (app.simTempEnabled) Serial.print(app.simTemp, 1);
  else Serial.print("off");

  Serial.print(" sim_hum=");
  if (app.simHumEnabled) Serial.print(app.simHum, 1);
  else Serial.print("off");
  Serial.println();
}

static int splitArgs(char* line, char* argv[], int maxArgs) {
  int argc = 0;
  char* p = line;
  while (*p && argc < maxArgs) {
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) break;
    argv[argc++] = p;
    while (*p && *p != ' ' && *p != '\t') p++;
    if (!*p) break;
    *p = '\0';
    p++;
  }
  return argc;
}

static bool parseU32(const char* s, uint32_t& out) {
  char* end = nullptr;
  unsigned long v = strtoul(s, &end, 10);
  if (end == s || *end != '\0') return false;
  out = (uint32_t)v;
  return true;
}

static bool parseFloatInRange(const char* s, float minV, float maxV, float& out) {
  char* end = nullptr;
  float v = strtof(s, &end);
  if (end == s || *end != '\0') return false;
  if (v < minV || v > maxV) return false;
  out = v;
  return true;
}

static bool applySetCommand(int argc, char* argv[]) {
  if (argc != 3) return false;

  uint32_t value = 0;
  if (!parseU32(argv[2], value)) {
    Serial.println("ERR invalid number");
    return true;
  }

  if (strcmp(argv[1], "show_ms") == 0) {
    if (value < 500 || value > 120000) {
      Serial.println("ERR show_ms range 500..120000");
      return true;
    }
    app.showMs = value;
    app.screenStartMs = millis();
    Serial.print("OK show_ms=");
    Serial.println(app.showMs);
    return true;
  }

  if (strcmp(argv[1], "ui_tick_ms") == 0) {
    if (value < 16 || value > 2000) {
      Serial.println("ERR ui_tick_ms range 16..2000");
      return true;
    }
    app.uiTickMs = value;
    app.lastUiTickMs = 0;
    Serial.print("OK ui_tick_ms=");
    Serial.println(app.uiTickMs);
    return true;
  }

  if (strcmp(argv[1], "display_refresh_ms") == 0) {
    if (value < 4 || value > 1000) {
      Serial.println("ERR display_refresh_ms range 4..1000");
      return true;
    }
    app.displayRefreshMs = value;
    app.lastRenderMs = 0;
    Serial.print("OK display_refresh_ms=");
    Serial.println(app.displayRefreshMs);
    return true;
  }

  Serial.println("ERR unknown setting");
  return true;
}

static bool applyGetCommand(int argc, char* argv[]) {
  if (argc != 2) return false;
  if (strcmp(argv[1], "show_ms") == 0) {
    Serial.print("show_ms=");
    Serial.println(app.showMs);
    return true;
  }
  if (strcmp(argv[1], "ui_tick_ms") == 0) {
    Serial.print("ui_tick_ms=");
    Serial.println(app.uiTickMs);
    return true;
  }
  if (strcmp(argv[1], "display_refresh_ms") == 0) {
    Serial.print("display_refresh_ms=");
    Serial.println(app.displayRefreshMs);
    return true;
  }
  if (strcmp(argv[1], "all") == 0) {
    printSerialStatus();
    return true;
  }
  Serial.println("ERR unknown setting");
  return true;
}

static void setSimTemp(float value, unsigned long now) {
  bool changed = !app.simTempEnabled || isnan(app.lastTemp) || fabsf(app.lastTemp - value) >= PERSIST_DELTA;
  app.simTempEnabled = true;
  app.simTemp = value;
  app.lastTemp = value;
  app.lastTempUpdateMs = now;
  app.simLastRefreshMs = now;
  if (changed) app.tempUpdatedSincePersist = true;
}

static void setSimHum(float value, unsigned long now) {
  bool changed = !app.simHumEnabled || isnan(app.lastHum) || fabsf(app.lastHum - value) >= PERSIST_DELTA;
  app.simHumEnabled = true;
  app.simHum = value;
  app.lastHum = value;
  app.lastHumUpdateMs = now;
  app.simLastRefreshMs = now;
  if (changed) app.humUpdatedSincePersist = true;
}

static bool applySimCommand(int argc, char* argv[], unsigned long now) {
  if (argc == 2 && strcmp(argv[1], "off") == 0) {
    app.simTempEnabled = false;
    app.simHumEnabled = false;
    Serial.println("OK simulation off");
    return true;
  }

  if (argc == 3 && strcmp(argv[1], "temp") == 0) {
    if (strcmp(argv[2], "off") == 0) {
      app.simTempEnabled = false;
      Serial.println("OK sim temp off");
      return true;
    }
    float value = 0.0f;
    if (!parseFloatInRange(argv[2], TEMP_MIN_C, TEMP_MAX_C, value)) {
      Serial.println("ERR temp out of range");
      return true;
    }
    setSimTemp(value, now);
    Serial.print("OK sim temp=");
    Serial.println(app.simTemp, 1);
    return true;
  }

  if (argc == 3 && strcmp(argv[1], "hum") == 0) {
    if (strcmp(argv[2], "off") == 0) {
      app.simHumEnabled = false;
      Serial.println("OK sim hum off");
      return true;
    }
    float value = 0.0f;
    if (!parseFloatInRange(argv[2], HUM_MIN, HUM_MAX, value)) {
      Serial.println("ERR hum out of range");
      return true;
    }
    setSimHum(value, now);
    Serial.print("OK sim hum=");
    Serial.println(app.simHum, 1);
    return true;
  }

  if (argc == 4 && strcmp(argv[1], "both") == 0) {
    float t = 0.0f;
    float h = 0.0f;
    if (!parseFloatInRange(argv[2], TEMP_MIN_C, TEMP_MAX_C, t)) {
      Serial.println("ERR temp out of range");
      return true;
    }
    if (!parseFloatInRange(argv[3], HUM_MIN, HUM_MAX, h)) {
      Serial.println("ERR hum out of range");
      return true;
    }
    setSimTemp(t, now);
    setSimHum(h, now);
    Serial.print("OK sim both temp=");
    Serial.print(app.simTemp, 1);
    Serial.print(" hum=");
    Serial.println(app.simHum, 1);
    return true;
  }

  return false;
}

static void applySensorSimulation(unsigned long now) {
  if (!app.simTempEnabled && !app.simHumEnabled) return;
  if (now - app.simLastRefreshMs < 1000) return;
  app.simLastRefreshMs = now;
  if (app.simTempEnabled) {
    app.lastTemp = app.simTemp;
    app.lastTempUpdateMs = now;
  }
  if (app.simHumEnabled) {
    app.lastHum = app.simHum;
    app.lastHumUpdateMs = now;
  }
}

static void applyFactoryReset(unsigned long now) {
  factoryResetPersisted();

  app.showMs = SHOW_MS;
  app.uiTickMs = UI_TICK_MS;
  app.displayRefreshMs = DISPLAY_REFRESH_MS;
  app.lastTemp = NAN;
  app.lastHum = NAN;
  app.lastTempUpdateMs = 0;
  app.lastHumUpdateMs = 0;
  app.tempUpdatedSincePersist = false;
  app.humUpdatedSincePersist = false;
  app.lastPersistMs = 0;
  app.simTempEnabled = false;
  app.simHumEnabled = false;
  app.simTemp = NAN;
  app.simHum = NAN;
  app.simLastRefreshMs = 0;

  clearForcedScreen(now);
  app.wipe.active = false;
  app.screenStartMs = now;
  app.lastUiTickMs = 0;
}

static void handleSerialCommands(unsigned long now) {
  static char cmd[96];
  static uint8_t len = 0;

  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      cmd[len] = '\0';
      char* argv[6] = {nullptr};
      int argc = splitArgs(cmd, argv, 6);
      if (argc == 0) {
        len = 0;
        continue;
      }

      if (strcmp(argv[0], "reboot") == 0 && argc == 1) {
        Serial.println("Rebooting...");
        delay(20);
        reboot_board();
      } else if (strcmp(argv[0], "factory") == 0 && argc == 2 && strcmp(argv[1], "reset") == 0) {
        applyFactoryReset(now);
        Serial.println("OK factory reset");
      } else if (strcmp(argv[0], "show") == 0 && argc == 2) {
        if (strcmp(argv[1], "temp") == 0) {
          forceScreen(SCREEN_TEMP, now);
          Serial.println("OK show temp");
        } else if (strcmp(argv[1], "hum") == 0) {
          forceScreen(SCREEN_HUM, now);
          Serial.println("OK show hum");
        } else if (strcmp(argv[1], "clock") == 0) {
          forceScreen(SCREEN_CLOCK, now);
          Serial.println("OK show clock");
        } else if (strcmp(argv[1], "auto") == 0) {
          clearForcedScreen(now);
          Serial.println("OK show auto");
        } else {
          Serial.println("ERR show expects temp|hum|clock|auto");
        }
      } else if (strcmp(argv[0], "auto") == 0 && argc == 1) {
        clearForcedScreen(now);
        Serial.println("OK show auto");
      } else if (strcmp(argv[0], "set") == 0) {
        if (!applySetCommand(argc, argv)) Serial.println("ERR usage: set <key> <value>");
      } else if (strcmp(argv[0], "get") == 0) {
        if (!applyGetCommand(argc, argv)) Serial.println("ERR usage: get <key|all>");
      } else if (strcmp(argv[0], "sim") == 0) {
        if (!applySimCommand(argc, argv, now)) Serial.println("ERR usage: sim temp|hum|both|off ...");
      } else if (strcmp(argv[0], "save") == 0 && argc == 2 && strcmp(argv[1], "settings") == 0) {
        saveRuntimeSettings(app);
        Serial.println("OK settings saved");
      } else if (strcmp(argv[0], "load") == 0 && argc == 2 && strcmp(argv[1], "settings") == 0) {
        if (loadRuntimeSettings(app)) {
          app.screenStartMs = now;
          app.lastUiTickMs = 0;
          Serial.println("OK settings loaded");
        } else {
          Serial.println("ERR no valid saved settings");
        }
      } else if (strcmp(argv[0], "status") == 0 && argc == 1) {
        printSerialStatus();
      } else if (strcmp(argv[0], "help") == 0 && argc == 1) {
        printSerialHelp();
      } else {
        Serial.println("ERR unknown command (try: help)");
      }
      len = 0;
      continue;
    }

    if (len < (sizeof(cmd) - 1)) {
      cmd[len++] = c;
    } else {
      // Overflow guard: drop oversized command.
      len = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println("Initialized Serial");
  Serial.println("=====================");
  Serial.println("MQTT Temp/Hum Display for Uno R4 WiFi");
  Serial.println("Build: " __DATE__ " " __TIME__);
  Serial.println("© 2026 elNino0916 and contributors.");
  Serial.println("https://github.com/elNino0916/ArduinoMQTTDisplay");
  Serial.println("https://elNino0916.de");
  Serial.println("=====================");
  Serial.println("Setup start");

#if HAS_WDT
  WDT.begin(WDT_TIMEOUT_MS);
#endif

  bool matrixOk = matrixInit(app);
  Serial.print("Matrix begin: ");
  Serial.println(matrixOk ? "1" : "0");
  mqttBindState(app);
  app.mqttClient.onMessage(onMqttMessage);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

#ifdef SHOW_BOOT_SELF_TEST
  // Quick visual self-test: checkerboard for 300ms
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 12; x++) {
      app.frame[y][x] = ((x + y) % 2) ? 1 : 0;
    }
  }
  matrixRenderBitmap(app, app.frame);
  delay(300);
#endif

  initAppState(app);
  loadPersisted(app);
  loadRuntimeSettings(app);
  initTimeService(app);

  drawWifiBarsAnim(app, 0);

  app.screenStartMs = millis();
  app.lastUiTickMs = 0;

  Serial.println("Setup done");
}

void loop() {
  unsigned long now = millis();
  static unsigned long lastHeartbeatMs = 0;
  static unsigned long uiDraws = 0;
  static bool wasNightMode = false;

  handleSerialCommands(now);

#if HAS_WDT
  WDT.refresh();
#endif

#ifdef DISPLAY_DEBUG_OVERRIDE
  static bool dbg = false;
  static unsigned long lastDbgMs = 0;
  static bool reinitDone = false;
  if (now - lastDbgMs >= 1000) {
    lastDbgMs = now;
    dbg = !dbg;
    if (!reinitDone && now > 5000) {
      reinitDone = true;
      bool ok = matrixInit(app);
      Serial.print("Matrix re-begin: ");
      Serial.println(ok ? "1" : "0");
    }
    static uint8_t frameA[8][12] = {
      {1,0,1,0,1,0,1,0,1,0,1,0},
      {0,1,0,1,0,1,0,1,0,1,0,1},
      {1,0,1,0,1,0,1,0,1,0,1,0},
      {0,1,0,1,0,1,0,1,0,1,0,1},
      {1,0,1,0,1,0,1,0,1,0,1,0},
      {0,1,0,1,0,1,0,1,0,1,0,1},
      {1,0,1,0,1,0,1,0,1,0,1,0},
      {0,1,0,1,0,1,0,1,0,1,0,1}
    };
    static uint8_t frameB[8][12] = {
      {0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0}
    };
    if (dbg) {
      matrixRenderBitmap(app, frameA);
    } else {
      matrixRenderBitmap(app, frameB);
    }

    digitalWrite(LED_BUILTIN, dbg ? HIGH : LOW);
  }
  delay(1);
  return;
#endif

#if SERIAL_DEBUG
  if (now - lastHeartbeatMs >= 60000) {
    lastHeartbeatMs = now;
    Serial.print("Heartbeat ms=");
    Serial.print(now);
    Serial.print(" WiFi=");
    Serial.print((int)WiFi.status());
    Serial.print(" MQTT=");
    Serial.print(app.mqttClient.connected() ? "1" : "0");
    Serial.print(" TimeValid=");
    Serial.print(timeIsValid(app) ? "1" : "0");
    if (timeIsValid(app)) {
      Serial.print(" Berlin=");
      int hh = berlinHour();
      int mm = berlinMinute();
      if (hh < 10) Serial.print("0");
      Serial.print(hh);
      Serial.print(":");
      if (mm < 10) Serial.print("0");
      Serial.print(mm);
    }
    Serial.print(" DisplayOff=");
    Serial.print(app.displayOffForSchedule ? "1" : "0");
    Serial.print(" Mode=");
    Serial.print((int)app.mode);
    Serial.print(" Draws=");
    Serial.println(uiDraws);
  }
#endif


  connectionTick(app, now);
  timeServiceTick(app);
  applySensorSimulation(now);

  if (app.connState == CONN_OK) {
    applyDisplayOffIfNeeded(app, shouldDisplayBeOffNow(app));
  } else {
    applyDisplayOffIfNeeded(app, false);
  }

  if (!app.displayOffForSchedule) {
    nightLedMinuteBlink(app, now);
  }

  if (app.displayOffForSchedule != wasNightMode) {
    wasNightMode = app.displayOffForSchedule;
    app.screenStartMs = now;
    app.lastUiTickMs = 0;
  }

  if (app.connState == CONN_OK && app.wipe.active) {
    app.mqttClient.poll();
    tickWipe(app, now);
    maybePersist(app, now);
    delay(1);
    return;
  }

  if (app.connState == CONN_OK) {
    app.mqttClient.poll();

    unsigned long elapsed = now - app.screenStartMs;

    if (now - app.lastUiTickMs >= app.uiTickMs) {
      app.lastUiTickMs = now;

      if (app.displayOffForSchedule) {
        drawClockMinimal(app, now, elapsed);
      } else {
        ScreenMode drawMode = serialForceScreen ? serial_forced_mode : app.mode;
        if (drawMode == SCREEN_TEMP)      drawTempScreen(app, now, elapsed);
        else if (drawMode == SCREEN_HUM)  drawHumScreen(app, now, elapsed);
        else                              drawClockScreen(app, now, elapsed);
      }

      uiDraws++;
    }

    if (!serialForceScreen && !app.displayOffForSchedule && elapsed >= app.showMs) {
      if (app.mode == SCREEN_TEMP) {
        startWipe(app, now, drawTempScreen, drawHumScreen, SCREEN_HUM);
      } else if (app.mode == SCREEN_HUM) {
        startWipe(app, now, drawHumScreen, drawClockScreen, SCREEN_CLOCK);
      } else {
        startWipe(app, now, drawClockScreen, drawTempScreen, SCREEN_TEMP);
      }
    }

    maybePersist(app, now);
  }

  if (now - app.lastRenderMs >= app.displayRefreshMs) {
    app.lastRenderMs = now;
    matrixRenderBitmap(app, app.frame);
  }

  delay(1);
}
