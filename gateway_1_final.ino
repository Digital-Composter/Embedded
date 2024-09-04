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
  Serial.printf("\nvc1: %.2f, vc2: %.2f, vc3: %.2f\n", vc1, vc2, vc3);
  Serial.printf("c1: %.2f, c2: %.2f, c3: %.2f\n", c1, c2, c3);
  Serial.printf("lw1: %.2f, lw2: %.2f, lw3: %.2f\n", lw1, lw2, lw3);
  Serial.printf("w1: %.2f, w2: %.2f, w3: %.2f\n", w1, w2, w3);
  Serial.printf("h1: %.2f, h2: %.2f, h3: %.2f\n", h1, h2, h3);
  Serial.printf("vh1: %.2f, vh2: %.2f, vh3: %.2f\n", vh1, vh2, vh3);
  Serial.printf("moist_min: %.2f, moist_max: %.2f", moist_min, moist_max);
  Serial.printf("\ndays: %d", days);
  Serial.printf("\nheater_pwm: %.2f, exhaust_pwm: %.2f\n", heater_pwm, exhaust_pwm);

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

  Serial.println("LoRa data sent\n");


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
          vc1 = round(doc["data"][0]["vc1"].as<float>() * 100.0) / 100.0;
          vc2 = round(doc["data"][0]["vc2"].as<float>() * 100.0) / 100.0;
          vc3 = round(doc["data"][0]["vc3"].as<float>() * 100.0) / 100.0;
          c1 = round(doc["data"][0]["c1"].as<float>() * 100.0) / 100.0;
          c2 = round(doc["data"][0]["c2"].as<float>() * 100.0) / 100.0;
          c3 = round(doc["data"][0]["c3"].as<float>() * 100.0) / 100.0;
          lw1 = round(doc["data"][0]["lw1"].as<float>() * 100.0) / 100.0;
          lw2 = round(doc["data"][0]["lw2"].as<float>() * 100.0) / 100.0;
          lw3 = round(doc["data"][0]["lw3"].as<float>() * 100.0) / 100.0;
          w1 = round(doc["data"][0]["w1"].as<float>() * 100.0) / 100.0;
          w2 = round(doc["data"][0]["w2"].as<float>() * 100.0) / 100.0;
          w3 = round(doc["data"][0]["w3"].as<float>() * 100.0) / 100.0;
          h1 = round(doc["data"][0]["h1"].as<float>() * 100.0) / 100.0;
          h2 = round(doc["data"][0]["h2"].as<float>() * 100.0) / 100.0;
          h3 = round(doc["data"][0]["h3"].as<float>() * 100.0) / 100.0;
          vh1 = round(doc["data"][0]["vh1"].as<float>() * 100.0) / 100.0;
          vh2 = round(doc["data"][0]["vh2"].as<float>() * 100.0) / 100.0;
          vh3 = round(doc["data"][0]["vh3"].as<float>() * 100.0) / 100.0;
          moist_min = round(doc["data"][0]["moist_min"].as<float>() * 100.0) / 100.0;
          moist_max = round(doc["data"][0]["moist_max"].as<float>() * 100.0) / 100.0;
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
          heater_pwm = round(doc["data"][0]["heater_pwm"].as<float>() * 100.0) / 100.0;
          exhaust_pwm = round(doc["data"][0]["exhaust_pwm"].as<float>() * 100.0) / 100.0;
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
  
  LoRa.print("a:"); LoRa.print(state); LoRa.print(",");
  LoRa.print("b:"); LoRa.print(vc1); LoRa.print(",");
  LoRa.print("c:"); LoRa.print(vc2); LoRa.print(",");
  LoRa.print("d:"); LoRa.print(vc3); LoRa.print(",");
  LoRa.print("e:"); LoRa.print(c1); LoRa.print(",");
  LoRa.print("f:"); LoRa.print(c2); LoRa.print(",");
  LoRa.print("g:"); LoRa.print(c3); LoRa.print(",");
  LoRa.print("h:"); LoRa.print(lw1); LoRa.print(",");
  LoRa.print("i:"); LoRa.print(lw2); LoRa.print(",");
  LoRa.print("j:"); LoRa.print(lw3); LoRa.print(",");
  LoRa.print("k:"); LoRa.print(w1); LoRa.print(",");
  LoRa.print("l:"); LoRa.print(w2); LoRa.print(",");
  LoRa.print("m:"); LoRa.print(w3); LoRa.print(",");
  LoRa.print("n:"); LoRa.print(h1); LoRa.print(",");
  LoRa.print("o:"); LoRa.print(h2); LoRa.print(",");
  LoRa.print("p:"); LoRa.print(h3); LoRa.print(",");
  LoRa.print("q:"); LoRa.print(vh1); LoRa.print(",");
  LoRa.print("r:"); LoRa.print(vh2); LoRa.print(",");
  LoRa.print("s:"); LoRa.print(vh3); LoRa.print(",");
  LoRa.print("t:"); LoRa.print(moist_min); LoRa.print(",");
  LoRa.print("u:"); LoRa.print(moist_max); LoRa.print(",");
  LoRa.print("v:"); LoRa.print(days); LoRa.print(",");
  LoRa.print("w:"); LoRa.print(heater); LoRa.print(",");
  LoRa.print("x:"); LoRa.print(exhaust); LoRa.print(",");

  LoRa.endPacket();
}
