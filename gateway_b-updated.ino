#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>

#define LORA_AURORA_V2_NSS 15
#define LORA_AURORA_V2_RST 0
#define LORA_AURORA_V2_DIO0 27
#define LORA_AURORA_V2_EN 32

WiFiMulti wifiMulti;

String incomingMessage = "";
uint8_t address_B = 0x03;

int temp_val, moist_val, ph_val, days;
float target_temp;
String phase = "";

void setup() {
  pinMode(LORA_AURORA_V2_EN, OUTPUT);
  digitalWrite(LORA_AURORA_V2_EN, HIGH);

  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  LoRa.setPins(LORA_AURORA_V2_NSS, LORA_AURORA_V2_RST, LORA_AURORA_V2_DIO0);

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  wifiMulti.addAP("BERLIMA", "ziaqwj73");

  while (!LoRa.begin(920E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!\n");
}

void loop() {
  receiveFromMCU();
}

void receiveFromMCU() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    uint8_t targetAddress = LoRa.read();
    if (targetAddress == address_B) {
      while (LoRa.available()) {
        incomingMessage += (char)LoRa.read();
      }
      Serial.println("Message received");
      parseLoRaMessage(incomingMessage);

      Serial.printf("\ntemperature: %d\n", temp_val);
      Serial.printf("moisture: %d\n", moist_val);
      Serial.printf("pH: %d\n", ph_val);

      Serial.println("Phase: ");
      Serial.print(phase);

      Serial.printf("\nTarget Temperature: %.3f\n\n", target_temp);

      postRealtime(temp_val, moist_val, ph_val, phase);
      postFuzzy(temp_val, target_temp);
      postRecords();
      
      incomingMessage = "";
    }
  }
}

void parseLoRaMessage(String message) {
  temp_val = getValue(message, "temp").toInt();
  moist_val = getValue(message, "moist").toInt();
  ph_val = getValue(message, "pH").toInt();
  phase = getValue(message, "phase");
  target_temp = getValue(message, "targettemp").toFloat();
}

String getValue(String data, String key) {
  int startIndex = data.indexOf(key + ":") + key.length() + 1;
  int endIndex = data.indexOf(",", startIndex);

  if (startIndex == -1 || startIndex < key.length()) {
    return "";
  }

  if (endIndex == -1) {
    endIndex = data.length();
  }
  
  return data.substring(startIndex, endIndex);
}

void postRealtime(int temp_val, int moist_val, int ph_val, String phase) {
  if (wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;

    http.begin("https://dicompos-backend-prod-45yiaz3vqa-et.a.run.app/data/realtime");

    http.addHeader("Content-Type", "application/json");

    String httpRequestData = "{\"temp\":" + String(temp_val) +
                            ",\"moist\":" + String(moist_val) +
                            ",\"ph\":" + String(ph_val) +
                            ",\"phase\":\"" + phase + "\"}"; 

    int httpResponseCode = http.POST(httpRequestData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}

void postFuzzy(int temp_val, float target_temp) {
  if (wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;

    http.begin("https://dicompos-backend-prod-45yiaz3vqa-et.a.run.app/fuzzy");

    http.addHeader("Content-Type", "application/json");

    String httpRequestData = "{\"currentTemperature\":" + String(temp_val) +
                             ",\"targetTemperature\":" + String(target_temp) + "}";

    int httpResponseCode = http.POST(httpRequestData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}

void postRecords() {
  String log = "MCU";
  if (wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;

    http.begin("https://dicompos-backend-prod-45yiaz3vqa-et.a.run.app/data/records");

    http.addHeader("Content-Type", "application/json");

    String httpRequestData = "{\"log\":" + log + "}";

    int httpResponseCode = http.POST(httpRequestData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}
