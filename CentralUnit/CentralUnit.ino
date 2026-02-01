#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define VPIN_TEMP V1
#define VPIN_TARGET_TEMP V3
#define VPIN_SYS_ACTIVE V4
#define VPIN_SYS_ACTIVE_STR V7
#define VPIN_COMBI_ACTIVE V5
#define RELAY_PIN 25

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <esp_now.h>

void connectWifi();
void connectBlynk();
void toggleOnOff();
void toggleCombi();
void processRoomData();
void setUpESPNOW();
void onDataRecv(const uint8_t *macAddr, const uint8_t *incomingData, int dataLen);

uint8_t dataBuffer[12];
const char BLYNK_AUTH_TOKEN[] = "YOUR_BLYNK_AUTH_TOKEN";
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
bool usingWifi;
int isSystemActive;
int isCombiActive;
float temperature;
float targetTemp;

BlynkTimer timer;

BLYNK_WRITE(VPIN_TEMP) {

  temperature = param.asFloat();
}

BLYNK_WRITE(VPIN_TARGET_TEMP) {

  targetTemp = param.asFloat();
}

BLYNK_WRITE(VPIN_SYS_ACTIVE) {

  isSystemActive = param.asInt();
  Serial.print("Retreived system value: ");
  Serial.println(isSystemActive);
}

void setup() {
  
  Serial.begin(115200);

  delay(2000L);
  
  isSystemActive = 1;
  isCombiActive = 0;
  usingWifi = true;

  connectWifi();
  setUpESPNOW();

  Serial.println(WiFi.macAddress());
  
  if(usingWifi) {

    connectBlynk();
  }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  if(usingWifi) {

    Blynk.virtualWrite(VPIN_COMBI_ACTIVE, "OFF");
  }
  
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

  timer.setInterval(420000L, []() {

    processRoomData();
  });
}

void loop() {
  
  if(usingWifi) {

    Blynk.run();
  }
  
  timer.run();
}

void connectWifi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
    
  if(WiFi.status() != WL_CONNECTED) {

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

void toggleCombi(bool state) {

  digitalWrite(RELAY_PIN, state ? LOW : HIGH);

  if(usingWifi) {

    Blynk.virtualWrite(VPIN_COMBI_ACTIVE, state ? "ON" : "OFF");
  }
  
  if(state) {

    isCombiActive = 1;
  }
  else {

    isCombiActive = 0;
  }

  Serial.println(state ? "Combi Active!" : "Combi Disabled!");
}

void processRoomData() {

  if(!usingWifi) {

    memcpy(&temperature, dataBuffer, sizeof(float));
    memcpy(&targetTemp, dataBuffer + 4, sizeof(float));
    memcpy(&isSystemActive, dataBuffer + 8, sizeof(int));

    Serial.println("Values Retreived With ESP Communication Mode.");
  }
  
  if(isSystemActive == 1) {

    if(usingWifi) {

      Blynk.virtualWrite(VPIN_SYS_ACTIVE_STR, "ON");
    }
    
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    Serial.print("Target Temperature: ");
    Serial.println(targetTemp);

    if((temperature < (targetTemp - 0.2)) && (isCombiActive == 0)) {

      toggleCombi(true);
    } 
    else if((temperature > (targetTemp + 0.2)) && (isCombiActive == 1)) {

      toggleCombi(false);
    }

    Serial.print("Combi: ");
    Serial.println(isCombiActive);
  }
  else {
  
    Serial.println("System Off.");
    toggleCombi(false);

    if(usingWifi) {

      Blynk.virtualWrite(VPIN_SYS_ACTIVE_STR, "OFF");
    }
  }
}

void setUpESPNOW() {

  WiFi.mode(WIFI_STA);
  unsigned long startTime = millis();

  while(esp_now_init() != ESP_OK) {

    Serial.println("Enabling ESP Communication Mode...");
    yield();

    if((millis() - startTime) > 10000L) {

      Serial.println("Couldn't Enable ESP Communication Mode! Resetting System...");
      ESP.restart();
      
      return;
    }

    delay(1000L);
  }

  esp_now_register_recv_cb(onDataRecv);
}

void onDataRecv(const uint8_t *macAddr, const uint8_t *incomingData, int dataLen) {

  memcpy(dataBuffer, incomingData, sizeof(dataBuffer));
} 