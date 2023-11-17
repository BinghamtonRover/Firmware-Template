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

uint8_t* decimalToHexArray(int decimalSpeed) {
  uint8_t* speed = new uint8_t[4];
  speed[0] = (decimalSpeed & 0xFF000000) >> 24;
  speed[1] = (decimalSpeed & 0x00FF0000) >> 16;
  speed[2] = (decimalSpeed & 0x0000FF00) >> 8;
  speed[3] = (decimalSpeed & 0x000000FF);

  return speed;
}

void loop() { 
  can.update();

  for (int i = 0; i < 10000; i++) {
    can.update();
    can.sendRaw(0x303, decimalToHexArray(i), 4);
    delay(2);
  }

  delay(2000);
}
