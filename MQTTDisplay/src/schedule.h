#pragma once

#include "app_state.h"

bool shouldDisplayBeOffNow(const AppState& s);
void applyDisplayOffIfNeeded(AppState& s, bool off);
void nightLedMinuteBlink(AppState& s, unsigned long now);
