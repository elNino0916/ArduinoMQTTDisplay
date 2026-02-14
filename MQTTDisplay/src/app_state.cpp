#include "app_state.h"
#include "matrix_io.h"

AppState app;

void initAppState(AppState& s) {
  s.connState = CONN_WIFI_WARMUP;
  s.stateStartMs = 0;
  s.lastAnimTickMs = 0;
  s.mqttSessionStartMs = 0;
  s.mqttBackoffUntilMs = 0;
  s.mqttBackoffMs = 0;
  s.wifiBackoffUntilMs = 0;
  s.wifiBackoffMs = 0;

  s.screenStartMs = 0;
  s.lastUiTickMs = 0;
  s.lastRenderMs = 0;
  s.mode = SCREEN_TEMP;

  s.wifiAnimStep = 0;
  s.mqttPhase = 0;

  s.timeValid = false;

  s.lastTemp = NAN;
  s.lastHum = NAN;
  s.lastTempUpdateMs = 0;
  s.lastHumUpdateMs = 0;
  s.tempUpdatedSincePersist = false;
  s.humUpdatedSincePersist = false;
  s.lastPersistMs = 0;
  s.showMs = SHOW_MS;
  s.uiTickMs = UI_TICK_MS;
  s.displayRefreshMs = DISPLAY_REFRESH_MS;
  s.simTempEnabled = false;
  s.simHumEnabled = false;
  s.simTemp = NAN;
  s.simHum = NAN;
  s.simLastRefreshMs = 0;

  s.displayOffForSchedule = false;
  s.lastBlinkMinute = -1;
  s.ledPulseUntilMs = 0;

  s.wipe.active = false;
}

void matrixRenderBitmap(AppState& s, uint8_t bitmap[8][12]) {
  s.matrix.renderBitmap(bitmap, 8, 12);
}

void matrixRenderBitmapConst(AppState& s, const uint8_t bitmap[8][12]) {
  static uint8_t tmp[8][12];
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 12; c++) {
      tmp[r][c] = bitmap[r][c];
    }
  }
  s.matrix.renderBitmap(tmp, 8, 12);
}

bool matrixInit(AppState& s) {
  return s.matrix.begin();
}
