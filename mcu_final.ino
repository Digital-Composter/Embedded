#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
#include <SoftwareSerial.h>

#define LORA_AURORA_V2_NSS 5
#define LORA_AURORA_V2_RST 14
#define LORA_AURORA_V2_DIO0 2

#define RE 27
#define DE 26

#define heatFan 25
#define waterPump 32
#define mixMotor 33
#define ptcHeater 13
#define coolFan 12

const uint32_t TIMEOUT = 500UL;

const byte interrogate[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09};

byte values[13];

// Rx pin, Tx pin // Software serial for RS485 communication
SoftwareSerial mod(17, 16);

String incomingMessage = "";
uint8_t address_MCU = 0x02;
uint8_t address_B = 0x03;

int state = 0;
int days = 0;
float vc1, vc2, vc3, c1, c2, c3, lw1, lw2, lw3, w1, w2, w3, h1, h2, h3, vh1, vh2, vh3, moist_min, moist_max;
float heater_pwm, exhaust_pwm;

int temp_val, moist_val, ph_val;

float target_temp;
String phase = "";

void setup() {
  Serial.begin(4800);
  mod.begin(4800);

  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  pinMode(heatFan, OUTPUT);
  pinMode(waterPump, OUTPUT);
  pinMode(mixMotor, OUTPUT);
  pinMode(ptcHeater, OUTPUT);
  pinMode(coolFan, OUTPUT);
  digitalWrite(heatFan, HIGH);
  digitalWrite(waterPump, HIGH);
  digitalWrite(mixMotor, HIGH);

  Serial.println();
  Serial.println();
  Serial.println();

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  LoRa.setPins(LORA_AURORA_V2_NSS, LORA_AURORA_V2_RST, LORA_AURORA_V2_DIO0);

  while (!LoRa.begin(920E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!\n");
}

void loop() {
  receiveFromGateway_A();
}

void receiveFromGateway_A() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    uint8_t targetAddress = LoRa.read(); 
    if (targetAddress == address_MCU) {   
      while (LoRa.available()) {
        incomingMessage += (char)LoRa.read();
      }
      Serial.println("\nData received from MCU");
      parseLoRaMessage(incomingMessage);

      Serial.printf("state: %d\n", state);
      Serial.printf("vc1: %.3f, vc2: %.3f, vc3: %.3f\n", vc1, vc2, vc3);
      Serial.printf("c1: %.3f, c2: %.3f, c3: %.3f\n", c1, c2, c3);
      Serial.printf("lw1: %.3f, lw2: %.3f, lw3: %.3f\n", lw1, lw2, lw3);
      Serial.printf("w1: %.3f, w2: %.3f, w3: %.3f\n", w1, w2, w3);
      Serial.printf("h1: %.3f, h2: %.3f, h3: %.3f\n", h1, h2, h3);
      Serial.printf("vh1: %.3f, vh2: %.3f, vh3: %.3f\n", vh1, vh2, vh3);
      Serial.printf("moist_min: %.3f, moist_max: %.3f\n", moist_min, moist_max);
      Serial.printf("days: %d\n", days);
      Serial.printf("heater: %.3f, exhaust: %.3f\n", heater_pwm, exhaust_pwm);

      if (state == 1){
        readPR3001ECTHPHN01();

        Serial.printf("\nTemperature: %d, Moisture: %d, pH: %d, Days: %d\n", temp_val, moist_val, ph_val, days);
        phase = determinePhase(temp_val, days);
        Serial.print("Phase: ");
        Serial.print(phase);
        setTargetTemp(phase);
        Serial.printf("\nTarget Temperature: %.3f\n", target_temp);
            
        transmitToGateway_B(address_B, temp_val, moist_val, ph_val, phase, target_temp); 

        controlActuators(heater_pwm, exhaust_pwm, moist_min, moist_max, moist_val);
        
        incomingMessage = "";
      }
      else {
        readPR3001ECTHPHN01();

        Serial.printf("\nTemperature: %d, Moisture: %d, pH: %d, Days: %d\n", temp_val, moist_val, ph_val, days);
        phase = determinePhase(temp_val, days);
        Serial.print("Phase: ");
        Serial.print(phase);
        setTargetTemp(phase);
        Serial.printf("\nTarget Temperature: %.3f\n", target_temp);

        analogWrite(ptcHeater, 0);
        analogWrite(coolFan, 0);
        digitalWrite(heatFan, HIGH);
        digitalWrite(mixMotor, HIGH);
        digitalWrite(waterPump, HIGH);
            
        transmitToGateway_B(address_B, temp_val, moist_val, ph_val, phase, target_temp);
        incomingMessage = "";
      }
    }
  }
}

void transmitToGateway_B(int8_t address, int temp, int moist, int ph, String phase, float target_temp) {

  LoRa.beginPacket();
  
  LoRa.write(address);
  
  LoRa.print("a:"); LoRa.print(temp); LoRa.print(",");
  LoRa.print("b:"); LoRa.print(moist); LoRa.print(",");
  LoRa.print("c:"); LoRa.print(ph); LoRa.print(",");
  LoRa.print("d:"); LoRa.print(phase); LoRa.print(",");
  LoRa.print("e:"); LoRa.print(target_temp); LoRa.print(",");

  LoRa.endPacket();

  Serial.println("Data transmitted to Gateway B\n");

  delay(500);
}

void parseLoRaMessage(String message) {
  state = getValue(message, "a").toInt();
  vc1 = getValue(message, "b").toFloat();
  vc2 = getValue(message, "c").toFloat();
  vc3 = getValue(message, "d").toFloat();
  c1 = getValue(message, "e").toFloat();
  c2 = getValue(message, "f").toFloat();
  c3 = getValue(message, "g").toFloat();
  lw1 = getValue(message, "h").toFloat();
  lw2 = getValue(message, "i").toFloat();
  lw3 = getValue(message, "j").toFloat();
  w1 = getValue(message, "k").toFloat();
  w2 = getValue(message, "l").toFloat();
  w3 = getValue(message, "m").toFloat();
  h1 = getValue(message, "n").toFloat();
  h2 = getValue(message, "o").toFloat();
  h3 = getValue(message, "p").toFloat();
  vh1 = getValue(message, "q").toFloat();
  vh2 = getValue(message, "r").toFloat();
  vh3 = getValue(message, "s").toFloat();
  moist_min = getValue(message, "t").toFloat();  
  moist_max = getValue(message, "u").toFloat();  
  days = getValue(message, "v").toInt();
  heater_pwm = getValue(message, "w").toFloat();
  exhaust_pwm = getValue(message, "x").toFloat();
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

String determinePhase(float temp, int waktu) {
    if (temp >= 10 && temp <= 30 && waktu >= 20 && waktu <= 35) {
        return "Maturasi";
    } else if (temp >= 20 && temp <= 50 && waktu >= 10 && waktu <= 30) {
        return "Mesofilik 2";
    } else if (temp >= 45 && temp <= 75 && waktu >= 5 && waktu <= 8) {
        return "Thermofilik 2";
    } else if (temp >= 45 && temp <= 75 && waktu >= 3 && waktu <= 15) {
        if (waktu >= 5 && waktu <= 8) {
            return "Thermofilik 2";
        } else {
            return "Thermofilik 1";
        }
    } else if (temp >= 10 && temp <= 50 && waktu >= 1 && waktu <= 5) {
        return "Mesofilik 1";

    } else if (temp >= 10 && temp <= 45 && waktu < 10) {
      if (waktu >= 5 && waktu <= 8) {
            return "Thermofilik 2";
        } else {
            return "Mesofilik 1";
        }
    } else if (temp >= 45 && temp <= 75 && waktu > 15) {
        return "Thermofilik 1";
    } else if (temp >= 20 && temp <= 50 && waktu > 30) {
        return "Mesofilik 2";
    } else if (temp >= 10 && temp <= 30 && waktu > 35) {
        return "Maturasi";

    } else {
        return "Out of Expected Range";
    }
}

void setTargetTemp(String phase) {
  if (phase == "Mesofilik 1") {
    target_temp = lw2;
  } else if (phase == "Thermofilik 1") {
    target_temp = w2;
  } else if (phase == "Thermofilik 2") {
    target_temp = h2;
  } else if (phase == "Mesofilik 2") {
    target_temp = c2;
  } else if (phase == "Maturasi") {
    target_temp = vc2;
  } else {
    target_temp = lw2;
  }
}

void controlActuators(float heater_pwm, float exhaust_pwm, float moist_min, float moist_max, int moist_val) {
    analogWrite(ptcHeater, heater_pwm);
    analogWrite(coolFan, exhaust_pwm);
    Serial.printf("Activate PTC Heater with %.3f PWM and Exhaust Fan with %.3f PWM\n", heater_pwm, exhaust_pwm);

    if (heater_pwm > exhaust_pwm && moist_val < moist_min) {
        digitalWrite(heatFan, LOW);
        digitalWrite(mixMotor, LOW);
        digitalWrite(waterPump, LOW);
        Serial.println("Condition 1: HeatFan ON, MixMotor ON, WaterPump ON");
    } else if (heater_pwm < exhaust_pwm && moist_val < moist_min) {
        digitalWrite(heatFan, HIGH);
        digitalWrite(mixMotor, LOW);
        digitalWrite(waterPump, LOW);
        Serial.println("Condition 2: HeatFan OFF, MixMotor ON, WaterPump ON");
    } else if (heater_pwm == exhaust_pwm && moist_val < moist_min) {
        digitalWrite(heatFan, HIGH);
        digitalWrite(mixMotor, LOW);
        digitalWrite(waterPump, LOW);
        Serial.println("Condition 3: HeatFan OFF, MixMotor ON, WaterPump ON");
    } else if (heater_pwm > exhaust_pwm && moist_val > moist_max) {
        digitalWrite(heatFan, LOW);
        digitalWrite(mixMotor, LOW);
        digitalWrite(waterPump, HIGH);
        Serial.println("Condition 4: HeatFan ON, MixMotor ON, WaterPump OFF");
    } else if (heater_pwm < exhaust_pwm && moist_val > moist_max) {
        digitalWrite(heatFan, LOW);
        digitalWrite(mixMotor, LOW);
        digitalWrite(waterPump, HIGH);
        Serial.println("Condition 5: HeatFan ON, MixMotor ON, WaterPump OFF");
    } else if (heater_pwm == exhaust_pwm && moist_val > moist_max) {
        digitalWrite(heatFan, LOW);
        digitalWrite(mixMotor, LOW);
        digitalWrite(waterPump, HIGH);
        Serial.println("Condition 6: HeatFan ON, MixMotor ON, WaterPump OFF");
    } else if (heater_pwm > exhaust_pwm && moist_val > moist_min && moist_val < moist_max) {
        digitalWrite(heatFan, LOW);
        digitalWrite(mixMotor, HIGH);
        digitalWrite(waterPump, HIGH);
        Serial.println("Condition 7: HeatFan ON, MixMotor OFF, WaterPump OFF");
    } else if (heater_pwm < exhaust_pwm && moist_val > moist_min && moist_val < moist_max) {
        digitalWrite(heatFan, HIGH);
        digitalWrite(mixMotor, LOW);
        digitalWrite(waterPump, HIGH);
        Serial.println("Condition 8: HeatFan OFF, MixMotor ON, WaterPump OFF");
    } else if (heater_pwm == exhaust_pwm && moist_val > moist_min && moist_val < moist_max) {
        digitalWrite(heatFan, HIGH);
        digitalWrite(mixMotor, HIGH);
        digitalWrite(waterPump, HIGH);
        Serial.println("Condition 9: HeatFan OFF, MixMotor OFF, WaterPump OFF");
    }
}

void readPR3001ECTHPHN01() {
  uint32_t startTime = 0;
  uint8_t  byteCount = 0;

  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  mod.write(interrogate, sizeof(interrogate));
    mod.flush();
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);

  startTime = millis();
  while ( millis() - startTime <= TIMEOUT ) {
    if (mod.available() && byteCount < sizeof(values) ) {
      values[byteCount++] = mod.read();
    }
    moist_val = (((values[3] * 256.0) + values[4])/10);
    temp_val = (((values[5] * 256.0) + values[6])/10);
    ph_val = (((values[9] * 256.0) + values[10])/10);
  }
}
