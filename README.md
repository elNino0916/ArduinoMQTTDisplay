# 🌡️ Arduino MQTT Display

> Turn your **Arduino Uno R4 WiFi** into a smart temperature and humidity display using its built-in 12×8 LED Matrix!

[![Arduino](https://img.shields.io/badge/Arduino-Uno%20R4%20WiFi-00979D?style=flat&logo=arduino&logoColor=white)](https://docs.arduino.cc/hardware/uno-r4-wifi/)
[![MQTT](https://img.shields.io/badge/MQTT-enabled-660066?style=flat&logo=mqtt&logoColor=white)](https://mqtt.org/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

<p align="center">
<img width="377" height="288" alt="image (1)" src="https://github.com/user-attachments/assets/2bf78267-445c-43f5-86ae-9794b5b91e6f" />
</p>

## ✨ Features

- 📊 **Real-time Display** - Shows temperature, humidity, and clock on the Arduino Uno R4 WiFi's built-in 12×8 LED matrix
- 🔌 **MQTT Integration** - Subscribes to MQTT topics for live sensor data
- 📡 **WiFi Connectivity** - Connects to your home network automatically
- 🌙 **Night Mode** - Automatically dims or disables display during nighttime hours
- 💾 **Persistent Storage** - Saves last known values to EEPROM
- 🔄 **Auto-recovery** - Reconnects to WiFi and MQTT broker automatically
- ⏰ **Network Time Sync** - Displays accurate time using NTP
- 🎮 **Serial Commands** - Debug and control via serial interface
- 🧪 **Simulation Mode** - Test display with simulated values

## 🎯 What It Does

This project transforms your Arduino Uno R4 WiFi into a smart home display that:

1. **Connects** to your WiFi network
2. **Subscribes** to MQTT topics for temperature and humidity data
3. **Displays** sensor readings on the LED matrix in a rotating cycle:
   - Temperature (°C or °F)
   - Humidity (%)
   - Clock (HH:MM)
4. **Persists** the latest values so they survive power cycles
5. **Publishes** online/offline status to MQTT

## 🛠️ Hardware Requirements

- **Arduino Uno R4 WiFi** (with built-in 12×8 LED Matrix)
- USB cable for programming
- Power source (USB or external)

## 📚 Dependencies

These Arduino libraries are required (install via Library Manager):

- `WiFiS3` - WiFi connectivity for Arduino Uno R4 WiFi
- `ArduinoMqttClient` - MQTT client
- `Arduino_LED_Matrix` - Control the built-in LED matrix
- `ezTime` - Network time synchronization
- `EEPROM` - Persistent storage

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/elNino0916/ArduinoMQTTDisplay.git
cd ArduinoMQTTDisplay/MQTTDisplay
```

### 2. Configure Your Credentials

Create your configuration files from the examples:

```bash
cp examples/secrets.h.example secrets.h
cp examples/config.h.example config.h
```

### 3. Edit `secrets.h`

Add your WiFi and MQTT credentials:

```cpp
const char WIFI_SSID[] = "YourWiFiName";
const char WIFI_PASS[] = "YourWiFiPassword";

const char MQTT_BROKER[] = "mqtt.example.com";
const int MQTT_PORT = 1883;

#define USE_MQTT_AUTH 1
const char MQTT_USER[] = "your_mqtt_username";
const char MQTT_PASS[] = "your_mqtt_password";
```

### 4. Edit `config.h`

Configure your MQTT topics and settings:

```cpp
const char TOPIC_TEMP[] = "home/livingroom/temperature/state";
const char TOPIC_HUM[] = "home/livingroom/humidity/state";
const char TOPIC_STATUS[] = "home/display/status/state";
const char MQTT_CLIENT_ID[] = "uno-r4-matrix";
```

Adjust timing and behavior:

```cpp
const unsigned long SHOW_MS = 8000;  // How long to show each screen (ms)
const unsigned long WIPE_MS = 450;   // Transition animation duration (ms)
```

### 5. Upload to Arduino

1. Open `MQTTDisplay.ino` in Arduino IDE
2. Select **Board**: "Arduino UNO R4 WiFi"
3. Select your **Port**
4. Click **Upload**

## 🎛️ Serial Commands

Connect via Serial Monitor (115200 baud) for debugging and control:

| Command | Description |
|---------|-------------|
| `help` | Show all available commands |
| `status` | Display current status and settings |
| `show <temp\|hum\|clock\|auto>` | Force display mode or return to auto |
| `sim temp <value\|off>` | Simulate temperature reading |
| `sim hum <value\|off>` | Simulate humidity reading |
| `sim both <temp> <hum>` | Simulate both values |
| `sim off` | Disable all simulation |
| `set <show_ms\|ui_tick_ms> <value>` | Adjust timing parameters |
| `save settings` | Save current settings to EEPROM |
| `load settings` | Load settings from EEPROM |
| `factory reset` | Reset to factory defaults |
| `reboot` | Restart the device |

## 📊 MQTT Message Format

The display expects simple numeric values as MQTT payloads:

**Temperature Topic** (e.g., `home/livingroom/temperature/state`):
```
22.5
```

**Humidity Topic** (e.g., `home/livingroom/humidity/state`):
```
65.3
```

Values are validated:
- Temperature: -20°C to 60°C
- Humidity: 0% to 100%

## 🏗️ Project Structure

```
MQTTDisplay/
├── MQTTDisplay.ino          # Main Arduino sketch
├── config.h                 # Your configuration (gitignored)
├── secrets.h                # Your credentials (gitignored)
├── user_settings.h          # User-customizable settings
├── examples/
│   ├── config.h.example     # Configuration template
│   └── secrets.h.example    # Credentials template
└── src/
    ├── app_state.cpp/h      # Application state management
    ├── connection.cpp/h     # WiFi/MQTT connection handling
    ├── font.cpp/h           # Custom font for LED matrix
    ├── matrix_io.h          # LED matrix utilities
    ├── mqtt_client.cpp/h    # MQTT message handling
    ├── persist.cpp/h        # EEPROM persistence
    ├── schedule.cpp/h       # Night mode scheduling
    ├── time_service.cpp/h   # NTP time synchronization
    └── ui.cpp/h             # Display rendering logic
```

## 🌙 Night Mode

The display automatically dims or turns off during nighttime hours. Configure in `schedule.cpp` or use `user_settings.h` to force day/night mode for testing:

```cpp
// #define FORCE_NIGHT_OFF  // Force display off
// #define FORCE_DAY_ON     // Force display always on
```

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built specifically for the [Arduino Uno R4 WiFi](https://docs.arduino.cc/hardware/uno-r4-wifi/) with its gorgeous built-in LED matrix
- Uses the excellent [ezTime](https://github.com/ropg/ezTime) library for time synchronization
- MQTT support via [ArduinoMqttClient](https://github.com/arduino-libraries/ArduinoMqttClient)
