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

uint8_t address_MCU = 0x02;
int state = 0;
int days = 0;
float vc1, vc2, vc3, c1, c2, c3, lw1, lw2, lw3, w1, w2, w3, h1, h2, h3, vh1, vh2, vh3, moist_min, moist_max;
float heater_pwm, exhaust_pwm;

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
  getState();
  getControl();
  getDays();
  getFuzzy();

  Serial.printf("state: %d", state);
  Serial.printf("\nvc1: %.3f, vc2: %.3f, vc3: %.3f\n", vc1, vc2, vc3);
  Serial.printf("c1: %.3f, c2: %.3f, c3: %.3f\n", c1, c2, c3);
  Serial.printf("lw1: %.3f, lw2: %.3f, lw3: %.3f\n", lw1, lw2, lw3);
  Serial.printf("w1: %.3f, w2: %.3f, w3: %.3f\n", w1, w2, w3);
  Serial.printf("h1: %.3f, h2: %.3f, h3: %.3f\n", h1, h2, h3);
  Serial.printf("vh1: %.3f, vh2: %.3f, vh3: %.3f\n", vh1, vh2, vh3);
  Serial.printf("moist_min: %.3f, moist_max: %.3f", moist_min, moist_max);
  Serial.printf("\ndays: %d", days);
  Serial.printf("\nheater_pwm: %f, exhaust_pwm: %f\n\n", heater_pwm, exhaust_pwm);

  sendLoRaMessage( address_MCU, 
                   state, 
                   vc1, vc2, vc3, 
                   c1, c2, c3, 
                   lw1, lw2, lw3, 
                   w1, w2, w3, 
                   h1, h2, h3, 
                   vh1, vh2, vh3, 
                   moist_min, moist_max,
                   days, 
                   heater_pwm, exhaust_pwm ); 

  Serial.println("LoRa data sent");


  delay(500);
}

void getState() {
  if (wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://dicompos-backend-prod-45yiaz3vqa-et.a.run.app/state");

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          state = doc["data"][0]["state"];
        }
      }
    } else {
      Serial.printf("[HTTP GET] failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void getControl() {
  if (wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://dicompos-backend-prod-45yiaz3vqa-et.a.run.app/control");

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          vc1 = doc["data"][0]["vc1"];
          vc2 = doc["data"][0]["vc2"];
          vc3 = doc["data"][0]["vc3"];
          c1 = doc["data"][0]["c1"];
          c2 = doc["data"][0]["c2"];
          c3 = doc["data"][0]["c3"];
          lw1 = doc["data"][0]["lw1"];
          lw2 = doc["data"][0]["lw2"];
          lw3 = doc["data"][0]["lw3"];
          w1 = doc["data"][0]["w1"];
          w2 = doc["data"][0]["w2"];
          w3 = doc["data"][0]["w3"];
          h1 = doc["data"][0]["h1"];
          h2 = doc["data"][0]["h2"];
          h3 = doc["data"][0]["h3"];
          vh1 = doc["data"][0]["vh1"];
          vh2 = doc["data"][0]["vh2"];
          vh3 = doc["data"][0]["vh3"];
          moist_min = doc["data"][0]["moist_min"];
          moist_max = doc["data"][0]["moist_max"];
        }
      }
    } else {
      Serial.printf("[HTTP GET] failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void getDays() {
  if (wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://dicompos-backend-prod-45yiaz3vqa-et.a.run.app/state/days");

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {    
          days = doc["data"]["days"] | 0;
        }
      }
    } else {
      Serial.printf("[HTTP GET] failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void getFuzzy() {
  if (wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://dicompos-backend-prod-45yiaz3vqa-et.a.run.app/fuzzy");

    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          heater_pwm = doc["data"][0]["heater_pwm"];
          exhaust_pwm = doc["data"][0]["exhaust_pwm"];
        }
      }
    } else {
      Serial.printf("[HTTP GET] failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void sendLoRaMessage( int8_t address, 
                      int state, 
                      float vc1, float vc2, float vc3, 
                      float c1, float c2, float c3, 
                      float lw1, float lw2, float lw3, 
                      float w1, float w2, float w3, 
                      float h1, float h2, float h3, 
                      float vh1, float vh2, float vh3, 
                      float moist_min, float moist_max,
                      int days, 
                      float heater, float exhaust ) 
{
  LoRa.beginPacket();
  
  LoRa.write(address);
  
  LoRa.print("state:"); LoRa.print(state); LoRa.print(",");
  LoRa.print("vc1:"); LoRa.print(vc1); LoRa.print(",");
  LoRa.print("vc2:"); LoRa.print(vc2); LoRa.print(",");
  LoRa.print("vc3:"); LoRa.print(vc3); LoRa.print(",");
  LoRa.print("jc1:"); LoRa.print(c1); LoRa.print(",");
  LoRa.print("jc2:"); LoRa.print(c2); LoRa.print(",");
  LoRa.print("jc3:"); LoRa.print(c3); LoRa.print(",");
  LoRa.print("lw1:"); LoRa.print(lw1); LoRa.print(",");
  LoRa.print("lw2:"); LoRa.print(lw2); LoRa.print(",");
  LoRa.print("lw3:"); LoRa.print(lw3); LoRa.print(",");
  LoRa.print("jw1:"); LoRa.print(w1); LoRa.print(",");
  LoRa.print("jw2:"); LoRa.print(w2); LoRa.print(",");
  LoRa.print("jw3:"); LoRa.print(w3); LoRa.print(",");
  LoRa.print("h1:"); LoRa.print(h1); LoRa.print(",");
  LoRa.print("h2:"); LoRa.print(h2); LoRa.print(",");
  LoRa.print("h3:"); LoRa.print(h3); LoRa.print(",");
  LoRa.print("vh1:"); LoRa.print(vh1); LoRa.print(",");
  LoRa.print("vh2:"); LoRa.print(vh2); LoRa.print(",");
  LoRa.print("vh3:"); LoRa.print(vh3); LoRa.print(",");
  LoRa.print("moistmin:"); LoRa.print(moist_min); LoRa.print(",");
  LoRa.print("moistmax:"); LoRa.print(moist_max); LoRa.print(",");
  LoRa.print("days:"); LoRa.print(days); LoRa.print(",");
  LoRa.print("heater:"); LoRa.print(heater); LoRa.print(",");
  LoRa.print("exhaust:"); LoRa.print(exhaust); LoRa.print(",");

  LoRa.endPacket();
}
