# 🕒 Smart Wi-Fi Alarm Clock with Alerts, Weather, and Web Interface

A **comprehensive IoT-based smart alarm clock** built using the **ESP32**, featuring real-time synchronization, configurable alarms, disaster alert monitoring, live weather updates, and a responsive web interface — all in one compact device.

## 🌟 Features

### 🕒 Real-Time Clock
- NTP client synchronization for accurate timekeeping
- OLED display showing current time and date
- Configured for India Standard Time (UTC+5:30)

### ⏰ Smart Alarms
- Configure up to 5 alarms via web interface
- Individual alarm naming, enable/disable, and snooze functionality
- Persistent storage using **LittleFS** (non-volatile memory)
- Hardware **snooze button** (GPIO 26) for 5-minute snooze

### 🌐 Web Interface
- Responsive HTML UI accessible from any browser
- Manage alarms, set weather location, and view live alerts
- Built-in REST API endpoints for system integration

### 📡 Wi-Fi Connectivity
- Automatic connection to saved network
- Real-time connection status display on OLED
- Auto-reconnection on network loss

### 🌤️ Live Weather Updates
- Integration with **OpenWeatherMap API**
- City-based temperature, humidity, and weather conditions
- Configurable location via web interface

### 🚨 Disaster Alert System
- Real-time alerts from NDMA (https://sachet.ndma.gov.in/CapFeed)
- Location-specific keyword scanning (e.g., "Kesinga", "Kalahandi")
- Visual warnings on OLED with audio alerts (max 5 buzzes per alert)
- Smart alert management to prevent repeated notifications

### 🖥️ OLED Display
- Rotating display modes:
  - ⏰ Time & next alarm information
  - 🌤️ Weather conditions
  - ⚠️ Alert status
- Clean iconography and intuitive UI elements

### ⏱️ Efficient Operation
- Non-blocking architecture using `Ticker` and `millis()` timing
- No `delay()` calls for smooth multitasking
- Optimized memory usage and performance

## 🛠️ Hardware Requirements

| Component | Description | Connection |
|-----------|-------------|------------|
| ESP32 Dev Board | Main microcontroller | - |
| 0.96" I2C OLED Display | Primary display | SDA: GPIO21, SCL: GPIO22 |
| Piezo Buzzer | Audio alerts | GPIO25 |
| Push Button | Snooze functionality | GPIO26 |
| Power Supply | Micro USB or battery | - |

## 📦 Dependencies

Install these libraries in your Arduino IDE:

```
WiFi (ESP32 core)
WebServer (ESP32 core)
NTPClient
WiFiUdp
ArduinoJson
LittleFS
HTTPClient
Adafruit GFX
Adafruit SSD1306
Ticker
```

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/Ronit-paikray/smart-wifi-alarm-clock.git
cd smart-wifi-alarm-clock
```

### 2. Configure Credentials
Update the following variables in the main code:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* weatherApiKey = "YOUR_OPENWEATHER_API_KEY";
```

### 3. Upload to ESP32
1. Open the project in Arduino IDE
2. Select board: **ESP32 Dev Module**
3. Upload the sketch
4. Upload filesystem data (Tools > ESP32 Sketch Data Upload)

### 4. Access Web Interface
1. Open Serial Monitor (baud: 115200)
2. Note the displayed IP address
3. Navigate to the IP address in your browser

## 🌐 Web Interface Features

The responsive web interface provides:

- **📍 Location Settings**: Configure weather location
- **⏰ Alarm Management**: Add, edit, and manage up to 5 alarms
- **⚠️ Alert Monitoring**: View current disaster alerts
- **🕒 System Status**: Real-time clock and system information

## 🔧 API Endpoints

The device exposes REST API endpoints for integration:

- `GET /api/time` - Current time and date
- `GET /api/alarms` - List all alarms
- `POST /api/alarms` - Create new alarm
- `PUT /api/alarms/{id}` - Update alarm
- `DELETE /api/alarms/{id}` - Delete alarm
- `GET /api/weather` - Current weather data
- `GET /api/alerts` - Active disaster alerts

## 📱 Usage

1. **Setting Alarms**: Use the web interface to configure alarms with custom names and times
2. **Snooze Function**: Press the hardware button (GPIO 26) to snooze active alarms for 5 minutes
3. **Weather Updates**: Configure your city in the web interface for local weather information
4. **Alert Monitoring**: The system automatically monitors for location-specific disaster alerts

## 🎯 Use Cases

This smart alarm clock is particularly useful for:

- **Disaster-prone areas**: Combines personal utility with early warning systems
- **Remote locations**: Provides reliable timekeeping and weather information
- **Smart homes**: Integrates with home automation systems via API
- **Educational projects**: Demonstrates IoT concepts and real-world applications

## 🔮 Future Enhancements

- [ ] OTA (Over-the-Air) update capability
- [ ] Voice and tone alarm customization
- [ ] Progressive Web App (PWA) support
- [ ] MQTT integration for home automation
- [ ] Multiple timezone support
- [ ] Battery level monitoring
- [ ] Sleep mode for power optimization

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👨‍💻 Author

**Ronit Paikray**  
*Cybersecurity Enthusiast | IoT Developer | Creator of The Desi Digital Defender*

## 🙏 Acknowledgments

- OpenWeatherMap for weather API
- NDMA for disaster alert feeds
- ESP32 community for extensive documentation
- Adafruit for excellent display libraries

---

**⭐ If you find this project useful, please consider giving it a star!**
