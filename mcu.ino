#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>

#define LORA_AURORA_V2_NSS 5
#define LORA_AURORA_V2_RST 14
#define LORA_AURORA_V2_DIO0 2
//#define LORA_AURORA_V2_EN 32

String incomingMessage = "";
uint8_t address_MCU = 0x02;
//uint8_t address_B = 0x03;

void setup() {
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
      Serial.println("Message received");
      Serial.print(incomingMessage);
      parseLoRaMessage(incomingMessage);
            
      //transmitToGateway_B();  // Step 2: Immediately transmit `sensor_a` to ESP_B
      incomingMessage = "";
    }
  }
}

// void transmitToGateway_B() {
//     int sensor_a = analogRead(34); // Example `sensor_a` data, replace with actual sensor reading

//     // Transmit `sensor_a` data to ESP_B
//     LoRa.beginPacket();
//     LoRa.write(address_B);   // Add target address (ESP_B)
//     LoRa.print(sensor_a);    // Add sensor data
//     LoRa.endPacket();

//     Serial.println("Transmitted sensor_a to ESP_B: " + String(sensor_a));

//     delay(1000); // Short delay to ensure transmission is complete
// }

void parseLoRaMessage(String message) {
  int state = getValue(message, "state").toInt();
  float vc1 = getValue(message, "vc1").toFloat();
  float vc2 = getValue(message, "vc2").toFloat();
  float vc3 = getValue(message, "vc3").toFloat();
  float c1 = getValue(message, "jc1").toFloat();
  float c2 = getValue(message, "jc2").toFloat();
  float c3 = getValue(message, "jc3").toFloat();
  float lw1 = getValue(message, "lw1").toFloat();
  float lw2 = getValue(message, "lw2").toFloat();
  float lw3 = getValue(message, "lw3").toFloat();
  float w1 = getValue(message, "jw1").toFloat();
  float w2 = getValue(message, "jw2").toFloat();
  float w3 = getValue(message, "jw3").toFloat();
  float h1 = getValue(message, "h1").toFloat();
  float h2 = getValue(message, "h2").toFloat();
  float h3 = getValue(message, "h3").toFloat();
  float vh1 = getValue(message, "vh1").toFloat();
  float vh2 = getValue(message, "vh2").toFloat();
  float vh3 = getValue(message, "vh3").toFloat();
  float moist_min = getValue(message, "moistmin").toFloat();  // Adjusted key to match input data
  float moist_max = getValue(message, "moistmax").toFloat();  // Adjusted key to match input data
  int days = getValue(message, "days").toInt();
  float heater = getValue(message, "heater").toFloat();
  float exhaust = getValue(message, "exhaust").toFloat();

  // Print parsed values
  Serial.printf("state: %d\n", state);
  Serial.printf("vc1: %.3f, vc2: %.3f, vc3: %.3f\n", vc1, vc2, vc3);
  Serial.printf("c1: %.3f, c2: %.3f, c3: %.3f\n", c1, c2, c3);
  Serial.printf("lw1: %.3f, lw2: %.3f, lw3: %.3f\n", lw1, lw2, lw3);
  Serial.printf("w1: %.3f, w2: %.3f, w3: %.3f\n", w1, w2, w3);
  Serial.printf("h1: %.3f, h2: %.3f, h3: %.3f\n", h1, h2, h3);
  Serial.printf("vh1: %.3f, vh2: %.3f, vh3: %.3f\n", vh1, vh2, vh3);
  Serial.printf("moist_min: %.3f, moist_max: %.3f\n", moist_min, moist_max);
  Serial.printf("days: %d\n", days);
  Serial.printf("heater: %.3f, exhaust: %.3f\n", heater, exhaust);
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
