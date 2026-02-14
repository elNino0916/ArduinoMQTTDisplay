#include "ui.h"
#include "font.h"
#include "time_service.h"
#include "matrix_io.h"

const uint8_t WipeAnim::order[12] = {5,6,4,7,3,8,2,9,1,10,0,11};

void clearFrame(AppState& s) {
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 12; c++)
      s.frame[r][c] = 0;
}

void setPixel(AppState& s, int x, int y, bool on) {
  if (x < 0 || x >= 12 || y < 0 || y >= 8) return;
  s.frame[y][x] = on ? 1 : 0;
}

void draw3x5(AppState& s, const uint8_t glyph[5], int x0, int y0) {
  for (int row = 0; row < 5; row++) {
    uint8_t bits = pgm_read_byte(&glyph[row]);
    for (int col = 0; col < 3; col++) {
      bool on = (bits >> (2 - col)) & 1;
      setPixel(s, x0 + col, y0 + row, on);
    }
  }
}

void drawTwoDigits(AppState& s, int value, int x0, int y0) {
  value = constrain(value, 0, 99);
  int tens = value / 10;
  int ones = value % 10;
  draw3x5(s, digitFont(tens), x0, y0);
  draw3x5(s, digitFont(ones), x0 + 4, y0);
}

void drawTempTenths(AppState& s, float tempC, int x0, int y0) {
  int temp10 = (int)roundf(tempC * 10.0f);
  int whole = temp10 / 10;
  int tenths = abs(temp10 % 10);

  whole = constrain(whole, 0, 99);
  drawTwoDigits(s, whole, x0, y0);

  setPixel(s, x0 + 7, y0 + 4, true);
  draw3x5(s, digitFont(tenths), x0 + 8, y0);
}

void drawNoDataGlyph(AppState& s, int x0, int y0) {
  draw3x5(s, F_X, x0, y0);
}

bool isStale(unsigned long now, unsigned long lastMs) {
  if (lastMs == 0) return true;
  return (now - lastMs) > STALE_MS;
}

void drawStaleIndicator(AppState& s, unsigned long now, bool stale) {
  if (!stale) return;
  if (((now / 400) % 2) != 0) return;

  // Explicit stale badge: tiny "!" in the top-left corner.
  setPixel(s, 0, 0, true);
  setPixel(s, 0, 1, true);
  setPixel(s, 0, 3, true);
  setPixel(s, 1, 0, true);
}


void render(AppState& s) {
  matrixRenderBitmap(s, s.frame);
}

void renderFrame(AppState& s, uint8_t f[8][12]) {
  matrixRenderBitmap(s, f);
}

void copyFrame(uint8_t dst[8][12], uint8_t src[8][12]) {
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 12; c++)
      dst[r][c] = src[r][c];
}

void drawProgressBar(AppState& s, unsigned long now, unsigned long elapsed, unsigned long total) {
  float p = (total == 0) ? 1.0f : (float)elapsed / (float)total;
  if (p < 0) p = 0;
  if (p > 1) p = 1;

  // Minimal timer cue in the bottom-right (3px) to preserve main data area.
  int step = (int)floorf(p * 3.0f);
  if (step < 0) step = 0;
  if (step > 2) step = 2;

  for (int x = 9; x <= 11; x++) setPixel(s, x, 7, false);
  for (int i = 0; i <= step; i++) setPixel(s, 9 + i, 7, true);
  if (p > 0.90f && ((now / 200) % 2) == 0) setPixel(s, 11, 7, false);
}

static void drawHumLevelBar(AppState& s, int hum) {
  // Horizontal humidity level bar at the bottom-left.
  int lit = (hum * 8 + 50) / 100;
  lit = constrain(lit, 0, 8);
  for (int x = 0; x < 8; x++) setPixel(s, x, 7, x < lit);
}

static void drawTinyDigit2x4(AppState& s, int d, int x0, int y0) {
  static const uint8_t glyphs[10][4] = {
    {0b11, 0b10, 0b10, 0b11}, // 0
    {0b01, 0b01, 0b01, 0b01}, // 1
    {0b11, 0b01, 0b11, 0b10}, // 2
    {0b11, 0b01, 0b11, 0b01}, // 3
    {0b10, 0b10, 0b11, 0b01}, // 4
    {0b11, 0b10, 0b11, 0b01}, // 5
    {0b11, 0b10, 0b11, 0b11}, // 6
    {0b11, 0b01, 0b01, 0b01}, // 7
    {0b11, 0b11, 0b11, 0b11}, // 8
    {0b11, 0b11, 0b11, 0b01}  // 9
  };

  d = constrain(d, 0, 9);
  for (int row = 0; row < 4; row++) {
    uint8_t bits = glyphs[d][row];
    setPixel(s, x0 + 0, y0 + row, (bits & 0b10) != 0);
    setPixel(s, x0 + 1, y0 + row, (bits & 0b01) != 0);
  }
}

void drawBigX(AppState& s, unsigned long now) {
  (void)now;
  clearFrame(s);

  bool pulse = ((millis() / 900) % 2) == 0;
  for (int i = 0; i < 8; i++) {
    int x1 = (int)roundf(i * (11.0f / 7.0f));
    int x2 = 11 - x1;
    setPixel(s, x1, i, true);
    setPixel(s, x2, i, true);
  }

  if (pulse) {
    setPixel(s, 0, 0, true);
    setPixel(s, 11, 0, true);
    setPixel(s, 0, 7, true);
    setPixel(s, 11, 7, true);
  }

  render(s);
}

void drawWifiBarsAnim(AppState& s, int step) {
  clearFrame(s);
  setPixel(s, 0, 6, true);

  const int xs[4] = {2, 4, 6, 8};
  const int h[4]  = {1, 2, 3, 4};
  static const uint8_t wave[24] = {
    0,1,2,3,4,3,2,1,0,0,1,2,
    3,4,3,2,1,0,0,1,2,3,2,1
  };

  for (int b = 0; b < 4; b++) {
    int idx = (step + b * 4) % 24;
    int lit = wave[idx];
    if (lit > h[b]) lit = h[b];
    for (int yy = 0; yy < lit; yy++) setPixel(s, xs[b], 6 - yy, true);
  }

  // Tiny sparkle on the tallest bar at the wave peak.
  if (wave[(step + 12) % 24] >= 4) {
    setPixel(s, 8, 2, true);
  }
  render(s);
}

void drawMqttAnimSmooth(AppState& s, unsigned long phase) {
  clearFrame(s);

  const int xmin = 2;
  const int xmax = 9;
  const int span = (xmax - xmin);

  int cycleLen = span * 2;
  int p = (int)(phase % cycleLen);

  int x;
  bool forward;
  if (p <= span) {
    x = xmin + p;
    forward = true;
  } else {
    x = xmax - (p - span);
    forward = false;
  }

  int y = 3 + ((phase / 4) & 0x1);
  setPixel(s, x, y, true);
  setPixel(s, x, y + 1, true);

  // Tail and a tiny flicker nose for a "packet" feel.
  if (forward) {
    if (x - 2 >= xmin) setPixel(s, x - 2, y + 1, true);
    if ((phase & 0x1) == 0 && x + 1 <= xmax) setPixel(s, x + 1, y, true);
  } else {
    if (x + 2 <= xmax) setPixel(s, x + 2, y + 1, true);
    if ((phase & 0x1) == 0 && x - 1 >= xmin) setPixel(s, x - 1, y, true);
  }

  // Endpoints (nodes) with subtle pulse on arrival.
  bool pulseL = (x == xmin) && ((phase & 0x3) == 0);
  bool pulseR = (x == xmax) && ((phase & 0x3) == 0);
  setPixel(s, 0, 4, true);
  setPixel(s, 0, 5, true);
  setPixel(s, 11, 2, true);
  setPixel(s, 11, 3, true);
  setPixel(s, 11, 4, true);
  if (pulseL) setPixel(s, 1, 4, true);
  if (pulseR) setPixel(s, 10, 3, true);

  render(s);
}

void drawTempScreen(AppState& s, unsigned long now, unsigned long elapsed) {
  clearFrame(s);

  bool stale = isStale(now, s.lastTempUpdateMs);
  drawStaleIndicator(s, now, stale);
  if (!isnan(s.lastTemp)) {
    drawTempTenths(s, s.lastTemp, 0, 0);
  } else {
    drawNoDataGlyph(s, 4, 1);
  }

  drawProgressBar(s, now, elapsed, s.showMs);
  render(s);
}

void drawHumScreen(AppState& s, unsigned long now, unsigned long elapsed) {
  clearFrame(s);

  bool stale = isStale(now, s.lastHumUpdateMs);
  drawStaleIndicator(s, now, stale);
  if (!isnan(s.lastHum)) {
    int hum = constrain((int)roundf(s.lastHum), 0, 99);
    drawTwoDigits(s, hum, 1, 0);
    draw3x5(s, F_PCT, 9, 0);
    drawHumLevelBar(s, hum);
  } else {
    drawNoDataGlyph(s, 4, 1);
  }

  drawProgressBar(s, now, elapsed, s.showMs);
  render(s);
}

void drawClockPlaceholder(AppState& s, unsigned long now, unsigned long elapsed) {
  clearFrame(s);
  bool stale = isStale(now, s.lastTempUpdateMs) || isStale(now, s.lastHumUpdateMs);
  drawStaleIndicator(s, now, stale);
  int y = 3;
  for (int i = 0; i < 4; i++) {
    int x0 = i * 3;
    setPixel(s, x0 + 0, y, true);
    setPixel(s, x0 + 1, y, true);
    setPixel(s, x0 + 2, y, true);
  }
  drawProgressBar(s, now, elapsed, s.showMs);
  render(s);
}

void drawClockScreen(AppState& s, unsigned long now, unsigned long elapsed) {
  if (!timeIsValid(s)) {
    drawClockPlaceholder(s, now, elapsed);
    return;
  }

  int hh = berlinHour();
  int mm = berlinMinute();
  (void)elapsed;

  clearFrame(s);

  bool stale = isStale(now, s.lastTempUpdateMs) || isStale(now, s.lastHumUpdateMs);
  drawStaleIndicator(s, now, stale);
  bool showHours = ((now / CLOCK_TOGGLE_MS) % 2) == 0;
  int value = showHours ? hh : mm;
  drawTwoDigits(s, value, 2, 1);
  setPixel(s, showHours ? 0 : 11, 0, true);

  drawProgressBar(s, now, elapsed, s.showMs);
  render(s);
}

void drawClockMinimal(AppState& s, unsigned long now, unsigned long elapsed) {
  clearFrame(s);

  if (!timeIsValid(s)) {
    render(s);
    return;
  }

  int hh = berlinHour();
  int mm = berlinMinute();
  (void)now;
  (void)elapsed;
  bool showHours = ((now / CLOCK_TOGGLE_MS) % 2) == 0;
  int value = showHours ? hh : mm;
  drawTwoDigits(s, value, 2, 1);
  setPixel(s, showHours ? 0 : 11, 0, true);

  render(s);
}

void startWipe(AppState& s, unsigned long now,
               void (*drawFrom)(AppState&, unsigned long, unsigned long),
               void (*drawTo)(AppState&, unsigned long, unsigned long),
               ScreenMode targetMode) {
  s.wipe.active = true;
  s.wipe.step = 0;
  s.wipe.nextStepMs = now;
  s.wipe.stepIntervalMs = WIPE_MS / 12;
  if (s.wipe.stepIntervalMs < 8) s.wipe.stepIntervalMs = 8;
  s.wipe.nextMode = targetMode;

  drawFrom(s, now, 0);
  copyFrame(s.wipe.from, s.frame);

  drawTo(s, now, 0);
  copyFrame(s.wipe.to, s.frame);

  copyFrame(s.wipe.out, s.wipe.from);
}

void tickWipe(AppState& s, unsigned long now) {
  if (!s.wipe.active) return;
  if ((long)(now - s.wipe.nextStepMs) < 0) return;

  if (s.mqttClient.connected()) s.mqttClient.poll();

  int col = WipeAnim::order[s.wipe.step];
  for (int r = 0; r < 8; r++) s.wipe.out[r][col] = s.wipe.to[r][col];
  renderFrame(s, s.wipe.out);

  s.wipe.step++;
  s.wipe.nextStepMs = now + s.wipe.stepIntervalMs;

  if (s.wipe.step >= 12) {
    s.wipe.active = false;

    s.mode = s.wipe.nextMode;
    s.screenStartMs = now;
    s.lastUiTickMs = 0;
  }
}
