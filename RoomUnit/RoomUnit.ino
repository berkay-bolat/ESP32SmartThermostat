#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define VPIN_TEMP V1
#define VPIN_HUMIDITY V2
#define VPIN_TARGET_TEMP V3
#define VPIN_SYS_ACTIVE V4
#define TFT_CS 15
#define TFT_RST 5
#define TFT_DC 4
#define T_IRQ 34
#define T_CS 33
#define T_DIN 23  //MOSI
#define T_CLK 18  //SCK
#define T_DO 19   //MISO
#define SCREEN_TIMEOUT 300000
#define BACKLIGHT_PIN 32
#define DHTPIN 13
#define DHTTYPE DHT22
#define BATTERY_PIN 35
#define MAX_BATTERY_VOLTAGE 4.2 
#define MIN_BATTERY_VOLTAGE 3.0 

#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <analogWrite.h>
#include <esp_now.h>

void connectWifi();
void connectBlynk();
void fetchWeatherData();
void translateWeather();
void updateDisplay(float temperature, float humidity, float targetTemp);
void updateDisplayValues(float temperature, float humidity, float targetTemp);
void updateDisplayDate();
void updateDisplayBattery();
void updateDisplayClock();
void drawButtons();
void sendRoomData();
void drawBatterySymbol(int x, int y);
void drawWiFiSymbol(int x, int y, uint16_t color);
void readPreferences();
void writePreferences();
void setUpESPNOW();
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

uint8_t receiverAddress[] = {0x??, 0x??, 0x??, 0x??, 0x??, 0x??}; // Enter your device MAC address
const char BLYNK_AUTH_TOKEN[] = "YOUR_BLYNK_AUTH_TOKEN";
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* country = "YOUR_COUNTRY_LABEL";
const char* city = "YOUR_CITY";
const char* apiKey = "YOUR_API_KEY";
int isSystemActive;
int isSystemActive2;
bool screen;
bool usingWifi;
unsigned long lastTouchTime;
const unsigned long debounceDelay = 200L;
float weatherTemperature;
float weatherHumidity;
float weatherWind;
float temperature;
float humidity;
float targetTemp;
float batteryLevel;
String day;
String month;
String year;
String hour;
String minute;
String weatherDescription = "Bilinmiyor";

DHT dht(DHTPIN, DHTTYPE);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(T_CS, T_IRQ);
BlynkTimer timer;
Preferences preferences;

BLYNK_WRITE(V3) {

  targetTemp = param.asFloat();
}

BLYNK_WRITE(V4) {

  int pinValue = param.asInt();
  isSystemActive = (pinValue == 1);
}

void setup() {

  Serial.begin(115200);
  preferences.begin("oda_unitesi", false);
  dht.begin();

  delay(2000L);

  readPreferences();
  ledcAttachPin(BACKLIGHT_PIN, 0);
  ledcSetup(0, 5000, 16);
  ledcWrite(0, 65535);

  isSystemActive2 = isSystemActive;
  usingWifi = true;
  screen = true;
       
  tft.begin();
  tft.setRotation(1);
  
  Serial.println("\nOLED Screen Initialized!");

  while(!ts.begin()) {

    Serial.println("Couldn't Initialized Touchscreen! Retrying...");
  }

  Serial.println("Touchscreen Initialized!");

  tft.fillScreen(ILI9341_BLACK);
  drawBatterySymbol(290, 6);
  connectWifi();
  setUpESPNOW();

  if(usingWifi) {

    connectBlynk();
  }
  
  tft.setTextSize(4);
  tft.setCursor(20, 100);
  tft.print("HOSGELDINIZ!");
  fetchWeatherData();

  delay(2000L);

  updateDisplay(temperature, humidity, targetTemp);
  drawButtons();
  lastTouchTime = millis();

  timer.setInterval(30000L, []() {

    if(WiFi.status() != WL_CONNECTED) {

      connectWifi();
    }

    if(usingWifi) {

      if(!Blynk.connected()) {

        connectBlynk();
      }
    }
  });

  timer.setInterval(60000L, []() {

    fetchWeatherData();
    updateDisplayValues(temperature, humidity, targetTemp);
    updateDisplayDate();
    updateDisplayClock();
    writePreferences();
  });

  timer.setInterval(420000L, []() {

    sendRoomData(temperature, humidity, targetTemp);
  });

  timer.setInterval(1800000L, []() {

    updateDisplayBattery();
  });
}

void loop() {

  if(usingWifi) {

    Blynk.run();
  }
  
  timer.run();

  if(isSystemActive == 1) {

    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if(humidity > 99.9) {

      humidity = 99.9;
    }

    if(isnan(temperature) || isnan(humidity)) {
    
      Serial.println("Couldn't Read DHT Sensor Data!");
      delay(1000L);
    
      return;
    }

    if(ts.touched() && screen && (millis() - lastTouchTime) > debounceDelay) {

      lastTouchTime = millis();
      TS_Point p = ts.getPoint();

      Serial.println(p.x);
      Serial.println(p.y);

      int x = map(p.x, 340, 3870, 0, 320);
      int y = map(p.y, 340, 3775, 0, 240);

      Serial.print("Touched Coordinates: X = ");
      Serial.print(x);
      Serial.print(", Y = ");
      Serial.println(y);

      if(x > 0 && x < 64 && y > 0 && y < 30) {

        targetTemp = targetTemp + 1;

        tft.fillRect(17, 127, 141, 37, ILI9341_BLACK);
        tft.setTextSize(4);
        tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
        tft.setCursor(17, 127);
        tft.print(targetTemp, 1);
        tft.setTextSize(2);
        tft.print(" ");
        tft.setTextSize(4);
        tft.print("C");
      }
      else if(x > 64 && x < 128 && y > 0 && y < 30) {

        targetTemp = targetTemp - 1;

        tft.fillRect(17, 127, 141, 37, ILI9341_BLACK);
        tft.setTextSize(4);
        tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
        tft.setCursor(17, 127);
        tft.print(targetTemp, 1);
        tft.setTextSize(2);
        tft.print(" ");
        tft.setTextSize(4);
        tft.print("C");
      }
      else if(x > 128 && x < 192 && y > 0 && y < 30) {

        isSystemActive = 0;

        if(usingWifi) {

          Blynk.virtualWrite(VPIN_SYS_ACTIVE, 0);
        }            
      }
      else if(x > 192 && x < 256 && y > 0 && y < 30) {

        targetTemp = targetTemp - 0.1;

        tft.fillRect(17, 127, 141, 37, ILI9341_BLACK);
        tft.setTextSize(4);
        tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
        tft.setCursor(17, 127);
        tft.print(targetTemp, 1);
        tft.setTextSize(2);
        tft.print(" ");
        tft.setTextSize(4);
        tft.print("C");
      }
      else if(x > 256 && x < 320 && y > 0 && y < 30) {

        targetTemp = targetTemp + 0.1;

        tft.fillRect(17, 127, 141, 37, ILI9341_BLACK);
        tft.setTextSize(4);
        tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
        tft.setCursor(17, 127);
        tft.print(targetTemp, 1);
        tft.setTextSize(2);
        tft.print(" ");
        tft.setTextSize(4);
        tft.print("C");
      } 
    }
    else if(ts.touched() && !screen && (millis() - lastTouchTime) > debounceDelay) {

      lastTouchTime = millis();
      ledcWrite(0, 65535);
      screen = true;
      delay(200L);
    }
  }
  else {

    if(ts.touched() && screen && (millis() - lastTouchTime) > debounceDelay) {

      lastTouchTime = millis();
      TS_Point p = ts.getPoint();

      Serial.println(p.x);
      Serial.println(p.y);

      int x = map(p.x, 340, 3870, 0, 320);
      int y = map(p.y, 340, 3775, 0, 240);

      Serial.print("Touched Coordinates: X = ");
      Serial.print(x);
      Serial.print(", Y = ");
      Serial.println(y);

      if(x > 128 && x < 192 && y > 0 && y < 30) {

        isSystemActive = 1;

        if(usingWifi) {

          Blynk.virtualWrite(VPIN_SYS_ACTIVE, 1);
        }
      }
    }
    else if(ts.touched() && !screen && (millis() - lastTouchTime) > debounceDelay) {

      lastTouchTime = millis();
      ledcWrite(0, 65535);
      screen = true;
      delay(200L);
    }
  }

  if(isSystemActive2 != isSystemActive) {

    if(isSystemActive) {

      tft.fillRect(140, 214, 40, 15, ILI9341_BLACK);
      tft.setTextSize(2);
      tft.setCursor(148, 214);
      tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
      tft.print("ON");
    }
    else {

      tft.fillRect(140, 214, 40, 15, ILI9341_BLACK);
      tft.setTextSize(2);
      tft.setCursor(143, 214);
      tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
      tft.print("OFF");
    }

    isSystemActive2 = isSystemActive;
  }

  if(screen && millis() - lastTouchTime > SCREEN_TIMEOUT) {

    ledcWrite(0, 1023);
    screen = false;
  }
}

void connectWifi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
    
  if(WiFi.status() != WL_CONNECTED) {

    drawWiFiSymbol(275, 10, ILI9341_RED);
    unsigned long startTime = millis();

    while(WiFi.status() != WL_CONNECTED) {
      
      Serial.println("Waiting For Wi-Fi Connection...");

      if((millis() - startTime) > 10000L) {

        Serial.println("Couldn't Connect Wi-Fi! ESP Communication Mode Will Be Used...");
        WiFi.disconnect();
        Blynk.disconnect();
        usingWifi = false;

        return;
      }

      delay(1000L);
    }
  }

  usingWifi = true;
  drawWiFiSymbol(275, 10, ILI9341_WHITE);
  Serial.println("Wi-Fi Connected!");
}

void connectBlynk() { 

  Blynk.config(BLYNK_AUTH_TOKEN);
  unsigned long startTime = millis();
  Serial.println("Waiting For Blynk Connection...");

  while(!Blynk.connect(10000L)) {

    if((millis() - startTime) > 10000L) {

      Serial.println("Couldn't Connect Blynk! Blynk Is Disabled.");
      
      return;
    }
  }

  Serial.println("Blynk Connected!"); 
}

void fetchWeatherData() {

  if(usingWifi) {

    HTTPClient http;
    String url = "http://api.weatherapi.com/v1/current.json?key=" + String(apiKey) + "&q=" + String(city) + "," + String(country) + "&aqi=no";
    WiFiClient client;
    http.begin(client, url);

    int httpCode = http.GET();

    if(httpCode == 200) {
    
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      weatherTemperature = doc["current"]["temp_c"].as<float>();
      weatherHumidity = doc["current"]["humidity"].as<float>();

      if(weatherHumidity > 99.9) {

        weatherHumidity = 99.9;
      }

      weatherWind = doc["current"]["wind_kph"].as<float>();
      weatherDescription = doc["current"]["condition"]["text"].as<String>();
      String localTime = doc["location"]["localtime"].as<String>();
      year = localTime.substring(0, 4);
      month = localTime.substring(5, 7);
      day = localTime.substring(8, 10);
      hour = localTime.substring(11, 13);
      minute = localTime.substring(14, 16);
      translateWeather();
    }
    else {

      Serial.println("Couldn't fetch weather forecast data!");
    }

    http.end();
  }
}

void translateWeather() {

  if(weatherDescription == "Clear") {

    weatherDescription = "Acik";
  }
  else if(weatherDescription == "Partly cloudy") {

    weatherDescription = "Parcali Bulutlu";
  }  
  else if(weatherDescription == "Cloudy") {
    
    weatherDescription = "Bulutlu";
  }
  else if(weatherDescription == "Overcast") {
    
    weatherDescription = "Kapali";
  }
  else if(weatherDescription == "Rain") {
    
    weatherDescription = "Yagmurlu";
  }
  else if(weatherDescription == "Thunderstorm") {
    
    weatherDescription = "Gok Gurultulu Firtinali";
  }
  else if(weatherDescription == "Snow") {
    
    weatherDescription = "Karli";
  }
  else if(weatherDescription == "Hail") {
    
    weatherDescription = "Dolu";
  }
  else if(weatherDescription == "Sleet") {
    
    weatherDescription = "Karla Karisik Yagmurlu";
  }
  else if(weatherDescription == "Mist") {
    
    weatherDescription = "Puslu";
  }
  else if(weatherDescription == "Fog") {
    
    weatherDescription = "Sisli";
  }
  else if(weatherDescription == "Drizzle") {
    
    weatherDescription = "Ciselemeli Yagmur";
  }
  else if(weatherDescription == "Dust") {
    
    weatherDescription = "Toz Firtinali";
  }
  else if(weatherDescription == "Sand") {
    
    weatherDescription = "Kum Firtinali";
  }
  else if(weatherDescription == "Tornado") {
    
    weatherDescription = "Kasirgali";
  }
  else if(weatherDescription == "Squall") {
    
    weatherDescription = "Sert Ruzgarli";
  }
  else if(weatherDescription == "Blizzard") {
    
    weatherDescription = "Kar Firtinali";
  }
  else if(weatherDescription == "Freezing drizzle") {
    
    weatherDescription = "Dondurucu Ciselemeli";
  }
  else if(weatherDescription == "Freezing rain") {
    
    weatherDescription = "Dondurucu Yagmurlu";
  }
  else if(weatherDescription == "Freezing fog") {
    
    weatherDescription = "Dondurucu Sisli";
  }
  else if(weatherDescription == "Thundershowers") {
    
    weatherDescription = "Firtinali Yagmurlu";
  }
  else if(weatherDescription == "Heavy rain") {
    
    weatherDescription = "Siddetli Yagmurlu";
  }
  else if(weatherDescription == "Light rain") {
    
    weatherDescription = "Hafif Yagmurlu";
  }
  else if(weatherDescription == "Heavy snow") {
    
    weatherDescription = "Siddetli Karli";
  }
  else if(weatherDescription == "Light snow") {
    
    weatherDescription = "Hafif Karli";
  }
  else if(weatherDescription == "Heavy hail") {
    
    weatherDescription = "Siddetli Dolu";
  }
  else if(weatherDescription == "Light hail") {
    
    weatherDescription = "Hafif Dolu";
  }
  else if(weatherDescription == "Heavy sleet") {
    
    weatherDescription = "Siddetli Karlı Yagmurlu";
  }
  else if(weatherDescription == "Light sleet") {
    
    weatherDescription = "Hafif Karlı Yagmurlu";
  }
  else {
    
    weatherDescription = "Bilgi Bekleniyor...";
  }
}

void updateDisplay(float temperature, float humidity, float targetTemp) {
  
  tft.fillScreen(ILI9341_BLACK);
  drawBatterySymbol(290, 6);

  if(WiFi.status() == WL_CONNECTED) {

    drawWiFiSymbol(275, 10, ILI9341_WHITE);
  }
  else {

    drawWiFiSymbol(275, 10, ILI9341_RED);
  }
  
  tft.setTextSize(2);
  tft.setCursor(65, 35);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("Ev");
  tft.setTextSize(4);
  tft.setCursor(17, 57);
  tft.setTextColor(ILI9341_ORANGE);
  tft.print(temperature, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("C");
  tft.setCursor(17, 92);
  tft.print(humidity, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("%");
  tft.setCursor(17, 127);
  tft.print(targetTemp, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("C");
  tft.setTextSize(2);
  tft.setCursor(177, 35);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("Hava Durumu");
  tft.setTextSize(4);
  tft.setCursor(177, 57);
  tft.setTextColor(ILI9341_ORANGE);
  tft.print(weatherTemperature, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("C");
  tft.setCursor(177, 92);
  tft.print(weatherHumidity, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("%");
  tft.setCursor(177, 127);
  tft.print(weatherWind, 1);
  tft.setTextSize(2);
  
  if(weatherWind < 10) {

    tft.setCursor(253, 141);
    tft.print("kmh");
  }
  else {

    tft.setCursor(276, 141);
    tft.print("kmh");
  }
  
  tft.setCursor(17, 176);
  tft.setTextColor(ILI9341_CYAN);
  tft.print(weatherDescription);
  tft.setCursor(5, 5);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(day);
  tft.print(".");
  tft.print(month);
  tft.print(".");
  tft.print(year);
  tft.setCursor(177, 5);
  tft.print(hour);
  tft.print(":");
  tft.print(minute);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.print("Target Temperature: ");
  Serial.print(targetTemp);
  Serial.println("°C");
  Serial.print("Outdoor Temperature: ");
  Serial.print(weatherTemperature);
  Serial.println("°C");
  Serial.print("Outdoor Humidity: ");
  Serial.print(weatherHumidity);
  Serial.println("%");
  Serial.print("Outdoor Wind Speed: ");
  Serial.print(weatherWind);
  Serial.println(" km/h");
  Serial.print("Outdoor Description: ");
  Serial.println(weatherDescription);
  Serial.print("Date: ");
  Serial.print(day);
  Serial.print(".");
  Serial.print(month);
  Serial.print(".");
  Serial.println(year);
  Serial.print("Time: ");
  Serial.print(hour);
  Serial.print(":");
  Serial.println(minute);
}

void updateDisplayValues(float temperature, float humidity, float targetTemp) {

  tft.fillRect(17, 57, 141, 107, ILI9341_BLACK);
  tft.setTextSize(4);
  tft.setCursor(17, 57);
  tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
  tft.print(temperature, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("C");
  tft.setCursor(17, 92);
  tft.print(humidity, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("%");
  tft.setCursor(17, 127);
  tft.print(targetTemp, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("C");
  tft.fillRect(177, 57, 140, 107, ILI9341_BLACK);
  tft.setTextSize(4);
  tft.setCursor(177, 57);
  tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
  tft.print(weatherTemperature, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("C");
  tft.setCursor(177, 92);
  tft.print(weatherHumidity, 1);
  tft.setTextSize(2);
  tft.print(" ");
  tft.setTextSize(4);
  tft.print("%");
  tft.setCursor(177, 127);
  tft.print(weatherWind, 1);
  tft.setTextSize(2);

  if(weatherWind < 10) {

    tft.setCursor(253, 141);
    tft.print("kmh");
  }
  else {

    tft.setCursor(276, 141);
    tft.print("kmh");
  }

  tft.fillRect(17, 176, 300, 17, ILI9341_BLACK);
  tft.setCursor(17, 176);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.print(weatherDescription);
}

void updateDisplayDate() {

  tft.setTextSize(2);
  tft.setCursor(5, 5);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print(day);
  tft.print(".");
  tft.print(month);
  tft.print(".");
  tft.print(year);
}

void updateDisplayBattery() {

  tft.fillRect(288, 6, 32, 19, ILI9341_BLACK);
  drawBatterySymbol(290, 6);
}

void updateDisplayClock() {

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(177, 5);
  tft.print(hour);
  tft.print(":");
  tft.print(minute);
}

void drawButtons() {

  int thickness = 2;

  tft.fillRect(0, 25, 320, thickness, ILI9341_WHITE);
  tft.fillRect(159, 25, thickness, 140, ILI9341_WHITE);
  tft.fillRect(0, 165, 320, thickness, ILI9341_WHITE);
  tft.fillRect(0, 25, thickness, 215, ILI9341_WHITE);
  tft.fillRect(318, 25, thickness, 215, ILI9341_WHITE);
  tft.fillRect(0, 200, 320, thickness, ILI9341_WHITE);
  tft.fillRect(0, 238, 320, thickness, ILI9341_WHITE);
  tft.fillRect(63.6, 200, thickness, 40, ILI9341_WHITE);
  tft.fillRect(127.2, 200, thickness, 40, ILI9341_WHITE);
  tft.fillRect(190.8, 200, thickness, 40, ILI9341_WHITE);
  tft.fillRect(254.4, 200, thickness, 40, ILI9341_WHITE);
  tft.setTextColor(ILI9341_MAGENTA);
  tft.setCursor(10, 214);
  tft.setTextSize(2);
  tft.print("+0.1");
  tft.setCursor(73.6, 214);
  tft.print("-0.1");
  tft.setCursor(213.8, 214);
  tft.print("-1");
  tft.setCursor(277.4, 214);
  tft.print("+1");

  if(isSystemActive == 1) {

    tft.setCursor(148, 214);
    tft.setTextColor(ILI9341_GREEN);
    tft.print("ON");
  }

  if(isSystemActive == 0) {

    tft.setCursor(143, 214);
    tft.setTextColor(ILI9341_RED);
    tft.print("OFF");
  }
}

void sendRoomData(float temperature, float humidity, float targetTemp) {

  if(usingWifi) {

    if(isSystemActive) {

      Blynk.virtualWrite(VPIN_TEMP, temperature);
      Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
      Blynk.virtualWrite(VPIN_TARGET_TEMP, targetTemp);
      Blynk.virtualWrite(VPIN_SYS_ACTIVE, isSystemActive);

      Serial.println("Room Data Has Sent To Blynk.");
    }
    else {

      Blynk.virtualWrite(VPIN_SYS_ACTIVE, isSystemActive);
      Serial.println("System Off.");
    }
  }
  else {

    uint8_t dataBuffer[12];

    memcpy(dataBuffer, &temperature, sizeof(float));
    memcpy(dataBuffer + 4, &targetTemp, sizeof(float));
    memcpy(dataBuffer + 8, &isSystemActive, sizeof(int));

    esp_err_t result = esp_now_send(receiverAddress, dataBuffer, sizeof(dataBuffer));

    if(result == ESP_OK) {

      Serial.println("Room Data Has Sent With ESP Communication Mode.");
    }
    else {

      Serial.println("Couldn't Send Room Data With ESP Communication Mode!");
    }
  }
}

void drawBatterySymbol(int x, int y) {

  int analogValue = analogRead(BATTERY_PIN);
  float voltage = analogValue / 1023;
  batteryLevel = map(voltage * 100, MAX_BATTERY_VOLTAGE * 100, MIN_BATTERY_VOLTAGE * 100, 0, 100);
  batteryLevel = constrain(batteryLevel, 0, 100);
  
  if(analogValue == 0) {

    batteryLevel = 100;
  }

  if(batteryLevel <= 25) {

    tft.drawRect(x, y, 20, 10, ILI9341_RED);
    tft.fillRect(x + 20, y + 2, 2, 6, ILI9341_RED);
    int fillWidth = map(batteryLevel, 0, 100, 0, 18);
    tft.fillRect(x + 1, y + 1, fillWidth, 8, ILI9341_RED);
  }
  else if(batteryLevel > 25) {

    tft.drawRect(x, y, 20, 10, ILI9341_WHITE);
    tft.fillRect(x + 20, y + 2, 2, 6, ILI9341_WHITE);
    int fillWidth = map(batteryLevel, 0, 100, 0, 18);
    tft.fillRect(x + 1, y + 1, fillWidth, 8, ILI9341_WHITE);
  }
}

void drawWiFiSymbol(int x, int y, uint16_t color) {
  
  tft.drawCircle(x, y, 6, color);
  tft.drawCircle(x, y, 4, color);
  tft.drawCircle(x, y, 2, color);
  tft.fillCircle(x, y + 5, 1, color);
}

void readPreferences() {

  temperature = preferences.getFloat("temperature", 0.0);
  humidity = preferences.getFloat("humidity", 0.0);
  targetTemp = preferences.getFloat("targetTemp", 0.0);
  isSystemActive = preferences.getBool("isSystemActive", true);
}

void writePreferences() {

  if(isnan(temperature)) {

    temperature = 0.0; 
  }

  if(isnan(humidity)) {

    humidity = 0.0;
  }

  if(isnan(targetTemp)) {

    targetTemp = 0.0;
  }

  if(isnan(isSystemActive)) {

    isSystemActive = true;
  }

  preferences.putFloat("temperature", temperature);
  preferences.putFloat("humidity", humidity);
  preferences.putFloat("targetTemp", targetTemp);
  preferences.putBool("isSystemActive", isSystemActive);

  Serial.println("Has Written To Preferences.");
}

void setUpESPNOW() {

  WiFi.mode(WIFI_STA);
  unsigned long startTime = millis();

  while(esp_now_init() != ESP_OK) {

    Serial.println("Enabling ESP Communication Mode...");

    if((millis() - startTime) > 10000L) {

      Serial.println("Couldn't Enable ESP Communication Mode! Resetting System...");
      ESP.restart();
      
      return;
    }

    delay(1000L);
  }

  esp_now_register_send_cb(onDataSent);
  
  esp_now_peer_info_t peerInfo;

  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = ESP_IF_WIFI_STA;
  
  if(esp_now_add_peer(&peerInfo) != ESP_OK) {

    Serial.println("Couldn't Enable ESP Communication Mode Because Of Missing Peer! Resetting System...");
    ESP.restart();
      
    return;
  }
  
  Serial.println("ESP-NOW Mode Active!");
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

  Serial.print("ESPNOW Data Sending Status: ");

  if(status == ESP_NOW_SEND_SUCCESS) {

    Serial.println("Successful.");
  }
  else {

    Serial.println("Unsuccessful! Resetting ESP...");
    delay(20000L);
    ESP.restart();
  }
}