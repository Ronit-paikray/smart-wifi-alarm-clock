#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ticker.h>

// Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Pin Configuration
#define BUZZER_PIN 25
#define SNOOZE_BUTTON_PIN 26

// Network Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// API Configuration
const char* weatherApiKey = "YOUR_OPENWEATHER_API_KEY";
const char* ndmaUrl = "https://sachet.ndma.gov.in/CapFeed";

// Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "[pool.ntp.org](http://pool.ntp.org/)", 19800, 60000); // UTC+5:30 for India
Ticker weatherTicker;
Ticker alertTicker;
Ticker displayTicker;

// Global Variables
struct Alarm {
int hour;
int minute;
bool enabled;
bool triggered;
String name;
};

Alarm alarms[5]; // Support up to 5 alarms
int alarmCount = 0;
String currentCity = "Kalahandi";
String weatherData = "";
String alertData = "";
bool alertActive = false;
int alertBeepCount = 0;
unsigned long lastAlertTime = 0;
unsigned long snoozeTime = 0;
bool snoozeActive = false;
int currentDisplayMode = 0; // 0=time, 1=weather, 2=alerts
unsigned long lastDisplaySwitch = 0;

// Weather Icons (8x8 pixels)
const unsigned char sunIcon[] PROGMEM = {
0x18, 0x18, 0x7e, 0xff, 0xff, 0x7e, 0x18, 0x18
};

const unsigned char cloudIcon[] PROGMEM = {
0x3c, 0x7e, 0x7e, 0xff, 0xff, 0xff, 0x7e, 0x3c
};

const unsigned char alertIcon[] PROGMEM = {
0x18, 0x3c, 0x7e, 0xff, 0xff, 0x7e, 0x18, 0x18
};

void setup() {
Serial.begin(115200);

// Initialize pins
pinMode(BUZZER_PIN, OUTPUT);
pinMode(SNOOZE_BUTTON_PIN, INPUT_PULLUP);

// Initialize display
if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
Serial.println(F("SSD1306 allocation failed"));
for(;;);
}

display.clearDisplay();
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);
display.println("Starting...");
display.display();

// Initialize LittleFS
if(!LittleFS.begin(true)) {
Serial.println("LittleFS Mount Failed");
return;
}

// Load saved alarms
loadAlarms();

// Connect to Wi-Fi
connectWiFi();

// Initialize NTP
timeClient.begin();

// Setup web server routes
setupWebServer();

// Start tickers
weatherTicker.attach(300, updateWeatherData); // Every 5 minutes
alertTicker.attach(60, checkAlerts); // Every minute
displayTicker.attach(1, updateDisplay); // Every second

// Initial data fetch
updateWeatherData();
checkAlerts();

Serial.println("Smart Alarm Clock Ready!");
}

void loop() {
server.handleClient();
timeClient.update();

// Check for snooze button press
if (digitalRead(SNOOZE_BUTTON_PIN) == LOW) {
handleSnooze();
delay(200); // Debounce
}

// Check alarms
checkAlarms();

// Handle alert beeping
if (alertActive && alertBeepCount < 5) {
if (millis() - lastAlertTime > 2000) {
beepBuzzer(2);
alertBeepCount++;
lastAlertTime = millis();
}
}

delay(100);
}

void connectWiFi() {
WiFi.begin(ssid, password);
display.clearDisplay();
display.setCursor(0, 0);
display.println("Connecting to WiFi...");
display.display();

while (WiFi.status() != WL_CONNECTED) {
delay(1000);
Serial.print(".");
}

Serial.println();
Serial.print("Connected to WiFi. IP address: ");
Serial.println(WiFi.localIP());

display.clearDisplay();
display.setCursor(0, 0);
display.println("WiFi Connected!");
display.print("IP: ");
display.println(WiFi.localIP());
display.display();
delay(2000);
}

void setupWebServer() {
// Serve the main page
server.on("/", HTTP_GET, handleRoot);

// API endpoints
server.on("/api/alarms", HTTP_GET, handleGetAlarms);
server.on("/api/alarms", HTTP_POST, handleSetAlarm);
server.on("/api/alarms", HTTP_DELETE, handleDeleteAlarm);
server.on("/api/weather", HTTP_GET, handleGetWeather);
server.on("/api/location", HTTP_POST, handleSetLocation);
server.on("/api/time", HTTP_GET, handleGetTime);

server.begin();
Serial.println("HTTP server started");
}

void handleRoot() {
String html = R"(
<!DOCTYPE html>
<html>
<head>
<title>Smart Alarm Clock</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
.container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
.header { text-align: center; color: #333; }
.section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
.alarm-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; margin: 5px 0; background: #f9f9f9; border-radius: 5px; }
input, select, button { padding: 8px; margin: 5px; border: 1px solid #ddd; border-radius: 3px; }
button { background: #4CAF50; color: white; cursor: pointer; }
button:hover { background: #45a049; }
.delete-btn { background: #f44336; }
.delete-btn:hover { background: #da190b; }
.weather-info { background: #e3f2fd; padding: 10px; border-radius: 5px; }
.alert-box { background: #ffebee; border-left: 4px solid #f44336; padding: 10px; margin: 10px 0; }
</style>
</head>
<body>
<div class="container">
<h1 class="header">🕒 Smart Alarm Clock</h1>

```
    <div class="section">
        <h3>⏰ Current Time</h3>
        <div id="currentTime">Loading...</div>
    </div>

    <div class="section">
        <h3>📍 Location Settings</h3>
        <input type="text" id="cityInput" placeholder="Enter city name" value="">
        <button onclick="setLocation()">Update Location</button>
    </div>

    <div class="section">
        <h3>🌤️ Weather</h3>
        <div id="weatherInfo" class="weather-info">Loading...</div>
    </div>

    <div class="section">
        <h3>⏰ Alarms</h3>
        <div>
            <input type="time" id="alarmTime">
            <input type="text" id="alarmName" placeholder="Alarm name">
            <button onclick="addAlarm()">Add Alarm</button>
        </div>
        <div id="alarmList"></div>
    </div>

    <div class="section">
        <h3>🚨 Disaster Alerts</h3>
        <div id="alertInfo">No active alerts</div>
    </div>
</div>

<script>
    function updateTime() {
        fetch('/api/time')
            .then(response => response.json())
            .then(data => {
                document.getElementById('currentTime').innerHTML =
                    `<strong>${data.time}</strong><br>Date: ${data.date}`;
            });
    }

    function updateWeather() {
        fetch('/api/weather')
            .then(response => response.json())
            .then(data => {
                if (data.error) {
                    document.getElementById('weatherInfo').innerHTML =
                        `<span style="color: red;">${data.error}</span>`;
                } else {
                    document.getElementById('weatherInfo').innerHTML =
                        `<strong>${data.city}</strong><br>
                         Temperature: ${data.temperature}°C<br>
                         Humidity: ${data.humidity}%<br>
                         Condition: ${data.condition}`;
                }
            });
    }

    function loadAlarms() {
        fetch('/api/alarms')
            .then(response => response.json())
            .then(data => {
                const alarmList = document.getElementById('alarmList');
                alarmList.innerHTML = '';
                data.alarms.forEach((alarm, index) => {
                    const alarmDiv = document.createElement('div');
                    alarmDiv.className = 'alarm-item';
                    alarmDiv.innerHTML = `
                        <div>
                            <strong>${String(alarm.hour).padStart(2, '0')}:${String(alarm.minute).padStart(2, '0')}</strong>
                            - ${alarm.name}
                            <br><small>${alarm.enabled ? '✅ Enabled' : '❌ Disabled'}</small>
                        </div>
                        <div>
                            <button onclick="toggleAlarm(${index})">${alarm.enabled ? 'Disable' : 'Enable'}</button>
                            <button class="delete-btn" onclick="deleteAlarm(${index})">Delete</button>
                        </div>
                    `;
                    alarmList.appendChild(alarmDiv);
                });
            });
    }

    function addAlarm() {
        const time = document.getElementById('alarmTime').value;
        const name = document.getElementById('alarmName').value || 'Alarm';

        if (!time) {
            alert('Please select a time');
            return;
        }

        const [hour, minute] = time.split(':');

        fetch('/api/alarms', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                hour: parseInt(hour),
                minute: parseInt(minute),
                name: name,
                enabled: true
            })
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                document.getElementById('alarmTime').value = '';
                document.getElementById('alarmName').value = '';
                loadAlarms();
            } else {
                alert('Error adding alarm');
            }
        });
    }

    function toggleAlarm(index) {
        // Implementation for toggling alarm
        fetch(`/api/alarms?index=${index}&action=toggle`, {
            method: 'POST'
        })
        .then(() => loadAlarms());
    }

    function deleteAlarm(index) {
        if (confirm('Delete this alarm?')) {
            fetch(`/api/alarms?index=${index}`, {
                method: 'DELETE'
            })
            .then(() => loadAlarms());
        }
    }

    function setLocation() {
        const city = document.getElementById('cityInput').value;
        if (!city) {
            alert('Please enter a city name');
            return;
        }

        fetch('/api/location', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ city: city })
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                updateWeather();
            }
        });
    }

    // Initialize and set up periodic updates
    updateTime();
    updateWeather();
    loadAlarms();

    setInterval(updateTime, 1000);
    setInterval(updateWeather, 300000); // Every 5 minutes
    setInterval(loadAlarms, 30000); // Every 30 seconds
</script>

```

</body>
</html>
)";

server.send(200, "text/html", html);
}

void handleGetAlarms() {
DynamicJsonDocument doc(2048);
JsonArray alarmArray = doc.createNestedArray("alarms");

for (int i = 0; i < alarmCount; i++) {
JsonObject alarm = alarmArray.createNestedObject();
alarm["hour"] = alarms[i].hour;
alarm["minute"] = alarms[i].minute;
alarm["enabled"] = alarms[i].enabled;
alarm["name"] = alarms[i].name;
}

String json;
serializeJson(doc, json);
server.send(200, "application/json", json);
}

void handleSetAlarm() {
if (server.hasArg("plain")) {
DynamicJsonDocument doc(1024);
deserializeJson(doc, server.arg("plain"));

```
if (alarmCount < 5) {
  alarms[alarmCount].hour = doc["hour"];
  alarms[alarmCount].minute = doc["minute"];
  alarms[alarmCount].enabled = doc["enabled"];
  alarms[alarmCount].name = doc["name"].as<String>();
  alarms[alarmCount].triggered = false;
  alarmCount++;

  saveAlarms();
  server.send(200, "application/json", "{\\"success\\":true}");
} else {
  server.send(400, "application/json", "{\\"error\\":\\"Maximum alarms reached\\"}");
}

```

} else {
// Handle toggle
if (server.hasArg("index") && server.hasArg("action")) {
int index = server.arg("index").toInt();
if (index >= 0 && index < alarmCount) {
if (server.arg("action") == "toggle") {
alarms[index].enabled = !alarms[index].enabled;
saveAlarms();
}
}
}
server.send(200, "application/json", "{\"success\":true}");
}
}

void handleDeleteAlarm() {
if (server.hasArg("index")) {
int index = server.arg("index").toInt();
if (index >= 0 && index < alarmCount) {
// Shift alarms
for (int i = index; i < alarmCount - 1; i++) {
alarms[i] = alarms[i + 1];
}
alarmCount--;
saveAlarms();
}
}
server.send(200, "application/json", "{\"success\":true}");
}

void handleGetWeather() {
DynamicJsonDocument doc(1024);

if (weatherData.length() > 0) {
DynamicJsonDocument weatherDoc(1024);
deserializeJson(weatherDoc, weatherData);

```
doc["city"] = currentCity;
doc["temperature"] = weatherDoc["main"]["temp"];
doc["humidity"] = weatherDoc["main"]["humidity"];
doc["condition"] = weatherDoc["weather"][0]["description"];

```

} else {
doc["error"] = "Weather data not available";
}

String json;
serializeJson(doc, json);
server.send(200, "application/json", json);
}

void handleSetLocation() {
if (server.hasArg("plain")) {
DynamicJsonDocument doc(512);
deserializeJson(doc, server.arg("plain"));

```
currentCity = doc["city"].as<String>();

// Save to LittleFS
File file = LittleFS.open("/location.txt", "w");
if (file) {
  file.println(currentCity);
  file.close();
}

updateWeatherData();
server.send(200, "application/json", "{\\"success\\":true}");

```

}
}

void handleGetTime() {
DynamicJsonDocument doc(512);

String timeStr = timeClient.getFormattedTime();
time_t epochTime = timeClient.getEpochTime();
struct tm *ptm = gmtime(&epochTime);
String dateStr = String(ptm->tm_mday) + "/" + String(ptm->tm_mon + 1) + "/" + String(ptm->tm_year + 1900);

doc["time"] = timeStr;
doc["date"] = dateStr;

String json;
serializeJson(doc, json);
server.send(200, "application/json", json);
}

void updateWeatherData() {
if (WiFi.status() == WL_CONNECTED) {
HTTPClient http;
String url = "http://api.openweathermap.org/data/2.5/weather?q=" + currentCity + "&appid=" + weatherApiKey + "&units=metric";

```
http.begin(url);
int httpCode = http.GET();

if (httpCode > 0) {
  weatherData = http.getString();
  Serial.println("Weather updated");
} else {
  Serial.println("Weather update failed");
}

http.end();

```

}
}

void checkAlerts() {
if (WiFi.status() == WL_CONNECTED) {
HTTPClient http;
http.begin(ndmaUrl);
int httpCode = http.GET();

```
if (httpCode > 0) {
  String payload = http.getString();

  // Simple keyword search for location-based alerts
  if (payload.indexOf("Kesinga") != -1 || payload.indexOf("Kalahandi") != -1) {
    if (!alertActive) {
      alertActive = true;
      alertBeepCount = 0;
      lastAlertTime = millis();
      alertData = "Disaster alert for your area!";
      Serial.println("ALERT: Disaster alert detected!");
    }
  }
}

http.end();

```

}
}

void checkAlarms() {
int currentHour = timeClient.getHours();
int currentMinute = timeClient.getMinutes();

for (int i = 0; i < alarmCount; i++) {
if (alarms[i].enabled && !alarms[i].triggered) {
if (alarms[i].hour == currentHour && alarms[i].minute == currentMinute) {
if (!snoozeActive) {
alarms[i].triggered = true;
triggerAlarm(i);
}
}
}

```
// Reset triggered flag at the next minute
if (alarms[i].triggered && (currentMinute != alarms[i].minute || currentHour != alarms[i].hour)) {
  alarms[i].triggered = false;
}

```

}

// Handle snooze
if (snoozeActive && millis() > snoozeTime) {
snoozeActive = false;
// Find the most recent alarm and trigger it again
for (int i = 0; i < alarmCount; i++) {
if (alarms[i].triggered) {
triggerAlarm(i);
break;
}
}
}
}

void triggerAlarm(int alarmIndex) {
Serial.println("ALARM TRIGGERED: " + alarms[alarmIndex].name);

// Continuous beeping until snooze or turned off
for (int i = 0; i < 10; i++) {
beepBuzzer(1);
delay(100);
if (digitalRead(SNOOZE_BUTTON_PIN) == LOW) {
handleSnooze();
return;
}
}
}

void handleSnooze() {
if (snoozeActive) return;

snoozeActive = true;
snoozeTime = millis() + 300000; // 5 minutes snooze
Serial.println("Snooze activated for 5 minutes");

// Stop current alarm sound
digitalWrite(BUZZER_PIN, LOW);
}

void beepBuzzer(int count) {
for (int i = 0; i < count; i++) {
digitalWrite(BUZZER_PIN, HIGH);
delay(100);
digitalWrite(BUZZER_PIN, LOW);
delay(100);
}
}

void updateDisplay() {
display.clearDisplay();

// Cycle through different display modes
if (millis() - lastDisplaySwitch > 5000) {
currentDisplayMode = (currentDisplayMode + 1) % 3;
lastDisplaySwitch = millis();
}

switch (currentDisplayMode) {
case 0:
displayTime();
break;
case 1:
displayWeather();
break;
case 2:
displayAlerts();
break;
}

display.display();
}

void displayTime() {
// Time display
display.setTextSize(2);
display.setCursor(10, 10);
display.println(timeClient.getFormattedTime());

// Next alarm
display.setTextSize(1);
display.setCursor(0, 35);
display.print("Next: ");

int nextAlarmHour = -1, nextAlarmMinute = -1;
for (int i = 0; i < alarmCount; i++) {
if (alarms[i].enabled) {
if (nextAlarmHour == -1 ||
(alarms[i].hour < nextAlarmHour) ||
(alarms[i].hour == nextAlarmHour && alarms[i].minute < nextAlarmMinute)) {
nextAlarmHour = alarms[i].hour;
nextAlarmMinute = alarms[i].minute;
}
}
}

if (nextAlarmHour != -1) {
display.print(String(nextAlarmHour).length() == 1 ? "0" + String(nextAlarmHour) : String(nextAlarmHour));
display.print(":");
display.print(String(nextAlarmMinute).length() == 1 ? "0" + String(nextAlarmMinute) : String(nextAlarmMinute));
} else {
display.print("None");
}

// Date
display.setCursor(0, 55);
time_t epochTime = timeClient.getEpochTime();
struct tm *ptm = gmtime(&epochTime);
display.print(String(ptm->tm_mday) + "/" + String(ptm->tm_mon + 1) + "/" + String(ptm->tm_year + 1900));
}

void displayWeather() {
display.setTextSize(1);
display.setCursor(0, 0);
display.print("Weather: ");
display.println(currentCity);

if (weatherData.length() > 0) {
DynamicJsonDocument doc(1024);
deserializeJson(doc, weatherData);

```
display.setCursor(0, 15);
display.print("Temp: ");
display.print(doc["main"]["temp"].as<float>(), 1);
display.println("C");

display.setCursor(0, 30);
display.print("Humidity: ");
display.print(doc["main"]["humidity"].as<int>());
display.println("%");

display.setCursor(0, 45);
display.println(doc["weather"][0]["description"].as<String>());

```

} else {
display.setCursor(0, 20);
display.println("No weather data");
}
}

void displayAlerts() {
display.setTextSize(1);
display.setCursor(0, 0);
display.println("Disaster Alerts:");

if (alertActive) {
// Draw alert icon
display.drawBitmap(0, 15, alertIcon, 8, 8, 1);
display.setCursor(15, 15);
display.println("ALERT ACTIVE!");

```
display.setCursor(0, 30);
display.println("Check your area");
display.setCursor(0, 45);
display.println("for safety info");

```

} else {
display.setCursor(0, 20);
display.println("No active alerts");
display.setCursor(0, 35);
display.println("All clear");
}
}

void saveAlarms() {
File file = LittleFS.open("/alarms.json", "w");
if (file) {
DynamicJsonDocument doc(2048);
JsonArray alarmArray = doc.createNestedArray("alarms");

```
for (int i = 0; i < alarmCount; i++) {
  JsonObject alarm = alarmArray.createNestedObject();
  alarm["hour"] = alarms[i].hour;
  alarm["minute"] = alarms[i].minute;
  alarm["enabled"] = alarms[i].enabled;
  alarm["name"] = alarms[i].name;
}

serializeJson(doc, file);
file.close();
Serial.println("Alarms saved");

```

}
}

void loadAlarms() {
File file = LittleFS.open("/alarms.json", "r");
if (file) {
DynamicJsonDocument doc(2048);
deserializeJson(doc, file);

```
JsonArray alarmArray = doc["alarms"];
alarmCount = 0;

for (JsonObject alarm : alarmArray) {
  if (alarmCount < 5) {
    alarms[alarmCount].hour = alarm["hour"];
    alarms[alarmCount].minute = alarm["minute"];
    alarms[alarmCount].enabled = alarm["enabled"];
    alarms[alarmCount].name = alarm["name"].as<String>();
    alarms[alarmCount].triggered = false;
    alarmCount++;
  }
}

file.close();
Serial.println("Alarms loaded: " + String(alarmCount));

```

}

// Load location
File locFile = LittleFS.open("/location.txt", "r");
if (locFile) {
currentCity = locFile.readStringUntil('\n');
currentCity.trim();
locFile.close();
}
}