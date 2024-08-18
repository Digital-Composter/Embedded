// Skenario 1, Terlalu dingin
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
#include <SoftwareSerial.h>

#define LORA_AURORA_V2_NSS 5
#define LORA_AURORA_V2_RST 14
#define LORA_AURORA_V2_DIO0 2
//#define LORA_AURORA_V2_EN 32

//Pin PR3001
#define RE 27
#define DE 26

#define heatFan 25
#define waterPump 33
#define mixMotor 32
#define ptcHeater 13
#define coolFan 12

const uint32_t TIMEOUT = 500UL;

const byte temp[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0xD5, 0xCA};
const byte PH[] = {0x01, 0x03, 0x00, 0x03, 0x00, 0x01, 0x74, 0x0A};

byte values[11];

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

  //Setup untuk input
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  //Setup untuk output
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
    uint8_t targetAddress = LoRa.read(); // Read the first byte as address
    if (targetAddress == address_MCU) {    // Check if the packet is for ESP_A from ESP_C
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
        moist_val = 50;
        temp_val = 15;
        ph_val = ph();

        Serial.printf("\nTemperature: %d, Moisture: %d, pH: %d, Days: %d\n", temp_val, moist_val, ph_val, days);
        phase = determinePhase(temp_val, days);
        Serial.print("Phase: ");
        Serial.print(phase);
        setTargetTemp(phase);
        Serial.printf("\nTarget Temperature: %.3f\n", target_temp);
            
        transmitToGateway_B(address_B, temp_val, moist_val, ph_val, phase, target_temp);  // Step 2: Immediately transmit `sensor_a` to ESP_B

        controlActuators(heater_pwm, exhaust_pwm, moist_min, moist_max, moist_val);
        
        incomingMessage = "";
      }
      else {
        moist_val = 50;
        temp_val = 15;
        ph_val = ph();

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
            
        transmitToGateway_B(address_B, temp_val, moist_val, ph_val, phase, target_temp);  // Step 2: Immediately transmit `sensor_a` to ESP_B
        incomingMessage = "";
      }
    }
  }
}

void transmitToGateway_B(int8_t address, int temp, int moist, int ph, String phase, float target_temp) {

  LoRa.beginPacket();
  
  LoRa.write(address);
  
  LoRa.print("temp:"); LoRa.print(temp); LoRa.print(",");
  LoRa.print("moist:"); LoRa.print(moist); LoRa.print(",");
  LoRa.print("pH:"); LoRa.print(ph); LoRa.print(",");
  LoRa.print("phase:"); LoRa.print(phase); LoRa.print(",");
  LoRa.print("targettemp:"); LoRa.print(target_temp);

  LoRa.endPacket();

  Serial.println("Data transmitted to Gateway B\n");

  delay(500); // Short delay to ensure transmission is complete
}

void parseLoRaMessage(String message) {
  state = getValue(message, "state").toInt();
  vc1 = getValue(message, "vc1").toFloat();
  vc2 = getValue(message, "vc2").toFloat();
  vc3 = getValue(message, "vc3").toFloat();
  c1 = getValue(message, "jc1").toFloat();
  c2 = getValue(message, "jc2").toFloat();
  c3 = getValue(message, "jc3").toFloat();
  lw1 = getValue(message, "lw1").toFloat();
  lw2 = getValue(message, "lw2").toFloat();
  lw3 = getValue(message, "lw3").toFloat();
  w1 = getValue(message, "jw1").toFloat();
  w2 = getValue(message, "jw2").toFloat();
  w3 = getValue(message, "jw3").toFloat();
  h1 = getValue(message, "h1").toFloat();
  h2 = getValue(message, "h2").toFloat();
  h3 = getValue(message, "h3").toFloat();
  vh1 = getValue(message, "vh1").toFloat();
  vh2 = getValue(message, "vh2").toFloat();
  vh3 = getValue(message, "vh3").toFloat();
  moist_min = getValue(message, "moistmin").toFloat();  // Adjusted key to match input data
  moist_max = getValue(message, "moistmax").toFloat();  // Adjusted key to match input data
  days = getValue(message, "days").toInt();
  heater_pwm = getValue(message, "heater").toFloat();
  exhaust_pwm = getValue(message, "exhaust").toFloat();
}

// Helper function to extract the value associated with a key from the message
String getValue(String data, String key) {
  // Add a delimiter to ensure exact key matching
  int startIndex = data.indexOf(key + ":") + key.length() + 1;
  int endIndex = data.indexOf(",", startIndex);

  if (startIndex == -1 || startIndex < key.length()) {
    return ""; // Return an empty string if key is not found
  }

  if (endIndex == -1) {
    endIndex = data.length();  // If it's the last element, read till the end of the string
  }
  
  return data.substring(startIndex, endIndex);
}

String determinePhase(float temp, int waktu) {
    if (temp >= 10 && temp <= 25 && waktu >= 20 && waktu <= 35) {
        return "Maturasi";
    } else if (temp >= 25 && temp <= 45 && waktu >= 10 && waktu <= 30) {
        return "Mesofilik 2";
    } else if (temp >= 55 && temp <= 75 && waktu >= 3 && waktu <= 8) {
        return "Thermofilik 2";
    } else if (temp >= 45 && temp <= 75 && waktu >= 3 && waktu <= 15) {
        if (waktu >= 5 && waktu <= 8) {
            return "Thermofilik 2";
        } else {
            return "Thermofilik 1";
        }
    } else if (temp >= 10 && temp <= 75 && waktu >= 1 && waktu <= 4) {
        if (waktu == 4) {
            return "Thermofilik 1";
        } else {
            return "Mesofilik 1";
        }

    } else if (temp >= 10 && temp <= 45 && waktu < 10) {
      if (waktu >= 5 && waktu <= 8) {
            return "Thermofilik 2";
        } else {
            return "Mesofilik 1";
        }
    } else if (temp >= 45 && temp <= 75 && waktu > 15) {
        return "Thermofilik 1";
    } else if (temp >= 20 && temp <= 45 && waktu > 30) {
        return "Mesofilik 2";
    } else if (temp >= 10 && temp <= 20 && waktu > 35) {
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

int16_t temperature() {
  float TEMP3and4;
  uint32_t startTime = 0;
  uint8_t  byteCount = 0;
// switch RS-485 to transmit mode
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
// write out the message
  mod.write(temp, sizeof(temp));
// wait for the transmission to complete
  mod.flush();
// switch RS-485 to receive mode
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  startTime = millis();
  while ( millis() - startTime <= TIMEOUT ) {
    if (mod.available() && byteCount < sizeof(values) ) {
      values[byteCount++] = mod.read();
      //printHexByte(values[byteCount - 1]);
    }
    TEMP3and4 = (((values[3] * 256.0) + values[4])/10); // converting hexadecimal to decimal
  }
    //Serial.println();
    return TEMP3and4;
}

int16_t ph() {
  float PH3and4;
  uint32_t startTime = 0;
  uint8_t  byteCount = 0;

  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  mod.write(PH, sizeof(PH));
  mod.flush();
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  startTime = millis();
  while ( millis() - startTime <= TIMEOUT ) {
    if (mod.available() && byteCount < sizeof(values) ) {
     values[byteCount++] = mod.read();
      //printHexByte(values[byteCount - 1]);
    }
    PH3and4 = (((values[3] * 256.0) + values[4])/10) + 1; // converting hexadecimal to decimal
  }
  //Serial.println();
  return PH3and4;
}
