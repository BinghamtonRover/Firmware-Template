#include "src/utils/BURT_utils.h"

void handler(const uint8_t* data, int length) { 
  Serial.print("Received data: ");
  for (int i = 0; i < length; i++) {
    Serial.print(data[i], HEX);
    Serial.print(", ");
  }
  Serial.println("");
}
BurtCan can(0x069, handler);

void setup() { 
  Serial.begin(9600);
  Serial.println("Starting");
  can.setup(); 
  Serial.println("CAN testbench initialized"); 
}

void loop() { 
  can.update();
  uint8_t *sendData1 = (uint8_t*) 0x069;
  // sendRaw(index, data, sizeof data)
  can.sendRaw(1, sendData1, 4);
  delay(500);
}
