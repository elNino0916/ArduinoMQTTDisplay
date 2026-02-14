#pragma once

#include "app_state.h"

void mqttBindState(AppState& s);
void onMqttMessage(int);
void mqttConfigureOnce(AppState& s);
bool mqttSubscribeOnce(AppState& s);
void mqttPublishStatusOnline(AppState& s);
