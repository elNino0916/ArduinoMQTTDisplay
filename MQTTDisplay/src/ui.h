#pragma once

#include "app_state.h"

void clearFrame(AppState& s);
void setPixel(AppState& s, int x, int y, bool on = true);
void draw3x5(AppState& s, const uint8_t glyph[5], int x0, int y0);
void drawTwoDigits(AppState& s, int value, int x0, int y0);
void drawTempTenths(AppState& s, float tempC, int x0, int y0);
void drawNoDataGlyph(AppState& s, int x0, int y0);
bool isStale(unsigned long now, unsigned long lastMs);
void drawStaleIndicator(AppState& s, unsigned long now, bool stale);
void render(AppState& s);
void renderFrame(AppState& s, uint8_t f[8][12]);
void copyFrame(uint8_t dst[8][12], uint8_t src[8][12]);
void drawProgressBar(AppState& s, unsigned long now, unsigned long elapsed, unsigned long total);
void drawBigX(AppState& s, unsigned long now);
void drawWifiBarsAnim(AppState& s, int step);
void drawMqttAnimSmooth(AppState& s, unsigned long phase);
void drawTempScreen(AppState& s, unsigned long now, unsigned long elapsed);
void drawHumScreen(AppState& s, unsigned long now, unsigned long elapsed);
void drawClockPlaceholder(AppState& s, unsigned long now, unsigned long elapsed);
void drawClockScreen(AppState& s, unsigned long now, unsigned long elapsed);
void drawClockMinimal(AppState& s, unsigned long now, unsigned long elapsed);
void startWipe(AppState& s, unsigned long now,
               void (*drawFrom)(AppState&, unsigned long, unsigned long),
               void (*drawTo)(AppState&, unsigned long, unsigned long),
               ScreenMode targetMode);
void tickWipe(AppState& s, unsigned long now);
