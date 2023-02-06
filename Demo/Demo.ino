#include <BURT_can.h>
#include <BURT_proto.h>

#include "src/BURT_arm.h"

#include "src/generated/electrical.pb.h"
#include "src/generated/science.pb.h"

#define ELECTRICAL_DATA_ID 0x23
#define SCIENCE_COMMAND_ID 0xC3

Arm arm;

void sendData() {
  ElectricalData data;
  data.v5_voltage = 5.0;
  // The 2nd parameter is always MessageName_fields
  BurtCan::send(ELECTRICAL_DATA_ID, ElectricalData_fields, &data);
}

void handleScienceCommand(const CanMessage& message) {
  // The 2nd parameter is always MessageName_fields
  ScienceCommand command = BurtProto::decode<ScienceCommand>(
    message.buf, ScienceCommand_fields
  );

  if (command.dig) Serial.println("Digging!");
}

void setup() {
  BurtCan::setup();
  BurtCan::registerHandler(SCIENCE_COMMAND_ID, handleScienceCommand);
  arm.moveTo(0, 0, 0);
}

void loop() {
  BurtCan::update();
  sendData();
  delay(10);
}
