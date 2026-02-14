#include "time_service.h"

static Timezone tzBerlin;
static bool timeSyncAttempted = false;

void initTimeService(AppState& s) {
  tzBerlin.setCache(EZTIME_CACHE_ADDR);
  bool ok = tzBerlin.setLocation("Europe/Berlin");
  if (!ok) {
    // Fallback to POSIX TZ rule for Berlin if timezoned fetch fails
    tzBerlin.setPosix("CET-1CEST,M3.5.0/2,M10.5.0/3");
  }
  tzBerlin.setDefault();
  setServer("pool.ntp.org");
  s.timeValid = false;
}

void timeServiceTick(AppState& s) {
  events();

  if (!timeSyncAttempted && WiFi.status() == WL_CONNECTED) {
    timeSyncAttempted = true;
    waitForSync();
  }

  s.timeValid = timeStatus() != timeNotSet;

#if SERIAL_DEBUG
  static bool once = false;
  if (s.timeValid && !once) {
    once = true;
    Serial.print("TZ: ");
    Serial.println(tzBerlin.getPosix());
  }
#endif
}

bool timeIsValid(const AppState& s) {
  return s.timeValid;
}

int berlinHour() {
  return tzBerlin.hour();
}

int berlinMinute() {
  return tzBerlin.minute();
}

int berlinSecond() {
  return tzBerlin.second();
}

int berlinWeekday0() {
  int wd = tzBerlin.weekday(); // 1=Sun..7=Sat
  return wd - 1;
}

void printBerlinTimeLine(const AppState& s) {
  if (!timeIsValid(s)) return;

  int hh = berlinHour();
  int mm = berlinMinute();
  int ss = berlinSecond();
  int dow = berlinWeekday0();

  static const char* DOW_NAME[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

  Serial.print("Berlin ");
  Serial.print(DOW_NAME[dow]);
  Serial.print(" ");
  if (hh < 10) Serial.print("0");
  Serial.print(hh);
  Serial.print(":");
  if (mm < 10) Serial.print("0");
  Serial.print(mm);
  Serial.print(":");
  if (ss < 10) Serial.print("0");
  Serial.println(ss);
}
