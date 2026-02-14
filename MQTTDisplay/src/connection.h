#pragma once

#include "app_state.h"

void goState(AppState& s, ConnState next, unsigned long now);
void connectionTick(AppState& s, unsigned long now);
