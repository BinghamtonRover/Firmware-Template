#include <BURT_can.h>
#include <BURT_proto.h>
#include <BURT_serial.h>

#include "src/BURT_methane.h"
#include "src/science.pb.h"

// See the CAN repository for details
#define SCIENCE_DATA_ID 0x27
#define SCIENCE_COMMAND_ID 0xC3

#define METHANE_PIN 12

MethaneSensor methaneSensor(METHANE_PIN);
BurtSerial serial(handleScienceCommand);

void handleScienceCommand(const uint8_t* data, int length) {
  // The 2nd parameter is always MessageName_fields
  auto command = BurtProto::decode<ScienceCommand>(data, ScienceCommand_fields);
  if (command.dig) Serial.println("Digging!");
}

void sendData() { 
  ScienceData data;
  data.methane = methaneSensor.read(); 
  // The 2nd parameter is always MessageName_fields
  BurtCan::send(SCIENCE_DATA_ID, ScienceData_fields, &data);
}

void setup() {
  BurtCan::setup();
  BurtCan::registerHandler(SCIENCE_COMMAND_ID, handleScienceCommand);
  // more handlers here...
}

void loop() {
  BurtCan::update();
  sendData();
  serial.parseSerial();
  delay(10);
}
