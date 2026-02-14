#pragma once

#include "app_state.h"

void loadPersisted(AppState& s);
void maybePersist(AppState& s, unsigned long now);
bool loadRuntimeSettings(AppState& s);
void saveRuntimeSettings(const AppState& s);
void factoryResetPersisted();
