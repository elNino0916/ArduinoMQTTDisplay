#include "schedule.h"
#include "time_service.h"
#include "matrix_io.h"

bool shouldDisplayBeOffNow(const AppState& s) {
#if defined(FORCE_NIGHT_OFF)
  return true;
#elif defined(FORCE_DAY_ON)
  return false;
#endif

  if (!timeIsValid(s)) return false;

  int hh = berlinHour();
  int dow = berlinWeekday0(); // 0=Sun..6=Sat

  if (hh < 8) return true;
  // Off from 22:00–24:00 on Sun–Thu (stay on later Fri–Sat)
  if (dow <= 4 && hh >= 22) return true;

  return false;
}

void applyDisplayOffIfNeeded(AppState& s, bool off) {
  if (off == s.displayOffForSchedule) return;
  s.displayOffForSchedule = off;
}

void nightLedMinuteBlink(AppState& s, unsigned long now) {
  if (!(timeIsValid(s) && s.displayOffForSchedule)) return;

  int mm = berlinMinute();

  if (mm != s.lastBlinkMinute) {
    s.lastBlinkMinute = mm;
    s.ledPulseUntilMs = now + LED_PULSE_MS;
    digitalWrite(LED_BUILTIN, HIGH);
  }

  if (s.ledPulseUntilMs != 0 && (long)(now - s.ledPulseUntilMs) >= 0) {
    s.ledPulseUntilMs = 0;
    digitalWrite(LED_BUILTIN, LOW);
  }
}
