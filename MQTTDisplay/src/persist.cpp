#include "persist.h"

struct PersistedData {
  uint32_t magic;
  float temp;
  float hum;
  uint16_t checksum;
};

const uint32_t PERSIST_MAGIC = 0x54484D44; // "THMD"
const int SENSOR_PERSIST_ADDR = 0;
const int SETTINGS_PERSIST_ADDR = 64;
const uint32_t SETTINGS_MAGIC = 0x54485354; // "THST"
const uint16_t SETTINGS_VERSION = 1;

struct PersistedSettings {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  uint32_t showMs;
  uint32_t uiTickMs;
  uint16_t checksum;
};

static uint16_t checksum16(const uint8_t* data, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 0; i < len; i++) sum = (uint16_t)(sum + data[i]);
  return sum;
}

void loadPersisted(AppState& s) {
  PersistedData data = {};
  EEPROM.get(SENSOR_PERSIST_ADDR, data);

  size_t len = sizeof(PersistedData) - sizeof(uint16_t);
  uint16_t cs = checksum16((const uint8_t*)&data, len);

  if (data.magic == PERSIST_MAGIC && cs == data.checksum) {
    s.lastTemp = data.temp;
    s.lastHum = data.hum;
    unsigned long now = millis();
    s.lastTempUpdateMs = now;
    s.lastHumUpdateMs = now;
  }
}

void maybePersist(AppState& s, unsigned long now) {
  if (!s.tempUpdatedSincePersist && !s.humUpdatedSincePersist) return;
  if (now - s.lastPersistMs < PERSIST_MIN_INTERVAL_MS) return;

  PersistedData data = {};
  data.magic = PERSIST_MAGIC;
  data.temp = s.lastTemp;
  data.hum = s.lastHum;

  size_t len = sizeof(PersistedData) - sizeof(uint16_t);
  data.checksum = checksum16((const uint8_t*)&data, len);

  EEPROM.put(SENSOR_PERSIST_ADDR, data);
  s.lastPersistMs = now;
  s.tempUpdatedSincePersist = false;
  s.humUpdatedSincePersist = false;
}

bool loadRuntimeSettings(AppState& s) {
  PersistedSettings data = {};
  EEPROM.get(SETTINGS_PERSIST_ADDR, data);

  size_t len = sizeof(PersistedSettings) - sizeof(uint16_t);
  uint16_t cs = checksum16((const uint8_t*)&data, len);

  if (data.magic != SETTINGS_MAGIC || data.version != SETTINGS_VERSION || cs != data.checksum) {
    return false;
  }

  if (data.showMs >= 500 && data.showMs <= 120000) {
    s.showMs = data.showMs;
  }
  if (data.uiTickMs >= 16 && data.uiTickMs <= 2000) {
    s.uiTickMs = data.uiTickMs;
  }
  return true;
}

void saveRuntimeSettings(const AppState& s) {
  PersistedSettings data = {};
  data.magic = SETTINGS_MAGIC;
  data.version = SETTINGS_VERSION;
  data.showMs = s.showMs;
  data.uiTickMs = s.uiTickMs;

  size_t len = sizeof(PersistedSettings) - sizeof(uint16_t);
  data.checksum = checksum16((const uint8_t*)&data, len);

  EEPROM.put(SETTINGS_PERSIST_ADDR, data);
}

void factoryResetPersisted() {
  PersistedData sensor = {};
  PersistedSettings settings = {};
  EEPROM.put(SENSOR_PERSIST_ADDR, sensor);
  EEPROM.put(SETTINGS_PERSIST_ADDR, settings);
}
