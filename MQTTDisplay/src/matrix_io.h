#pragma once

#include "app_state.h"

void matrixRenderBitmap(AppState& s, uint8_t bitmap[8][12]);
void matrixRenderBitmapConst(AppState& s, const uint8_t bitmap[8][12]);
bool matrixInit(AppState& s);
