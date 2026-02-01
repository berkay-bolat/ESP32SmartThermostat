# ESP32SmartThermostat

ESP32 SMART THERMOSTAT (DUAL UNIT)

A smart room thermostat system powered by ESP32, utilizing both ESP-NOW (local redundancy) and Blynk IoT (cloud control). It consists of a touch-screen Room Unit and a relay-controlled Central Unit.

FEATURES

Dual Connectivity: Uses Wi-Fi/Blynk for remote control and ESP-NOW for direct local communication (works even if Wi-Fi is down).

Smart UI: ILI9341 Touchscreen interface designed with custom buttons and indicators.

Weather Integration: Fetches real-time weather data from WeatherAPI.com. 

Hysteresis Control: Smart temperature logic with ±0.2°C threshold to protect the boiler. 

Battery Monitoring: Visual battery level indicator for the portable Room Unit. 

HARDWARE REQUIREMENTS

Room Unit (Transmitter & UI)
    -ESP32 Development Board
    -2.8" or 3.2" TFT LCD Display (ILI9341 Driver) & XPT2046 Touch Controller (Built-in with display)
    -DHT22 Temperature & Humidity Sensor
    -Li-Po Battery (Optional)
    -Jumper Cables

Central Unit (Receiver & Controller)
    -ESP32 Development Board
    -Relay Module (5V/3.3V trigger) * Connection to Boiler/Heater
    -Jumper Cables

INSTALLATION & SETUP

Libraries: Install the following libraries via Arduino IDE Library Manager
    -WiFi
    -HTTPClient
    -BlynkSimpleEsp32
    -Adafruit_Sensor
    -Adafruit_GFX
    -Adafruit_ILI9341
    -XPT2046_Touchscreen
    -DHT
    -ArduinoJson
    -Preferences
    -analogWrite
    -esp_now

Configuration:

Open RoomUnit.ino and CentralUnit.ino. Fill in your credentials in the lines marked below:

#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
const char BLYNK_AUTH_TOKEN[] = "YOUR_BLYNK_AUTH_TOKEN";
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey = "YOUR_WEATHER_API_KEY";
uint8_t receiverAddress[] = {0x??, 0x??, 0x??, 0x??, 0x??, 0x??};

Note: You should create Blynk templates and arrange their dashboards and virtual data pins to use this system efficiently. Do not forget to set VPIN values in CentralUnit.ino and RoomUnit.ino accordingly.

HOW IT WORKS?

Room Unit: Reads temp/humidity via DHT22. Allows user to set target temp via touchscreen. Sends data to Central Unit via ESP-NOW or Blynk cloud.

Central Unit: Receives data. Upon checking current status, it controls the relay.

Redundancy: If Wi-Fi fails, the system automatically switches to ESP-NOW mode to ensure heating continues to work locally. 

DISCLAIMER

This project is still under development and involves controlling high-voltage appliances (Boilers/Heaters). Proceed with caution. The developer is not responsible for any damage caused by improper wiring.