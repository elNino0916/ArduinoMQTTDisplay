#pragma once

#include "app_state.h"

void initTimeService(AppState& s);
void timeServiceTick(AppState& s);
bool timeIsValid(const AppState& s);
int berlinHour();
int berlinMinute();
int berlinSecond();
int berlinWeekday0(); // 0=Sun..6=Sat
void printBerlinTimeLine(const AppState& s);
