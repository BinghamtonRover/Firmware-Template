#include "src/utils/BURT_utils.h"

#include "src/BURT_methane.h"
#include "src/science.pb.h"

// See the CAN repository for details
#define SCIENCE_DATA_ID 0x27
#define SCIENCE_COMMAND_ID 0xC3

#define METHANE_PIN 12

// These are "forward declarations". We need them a few lines down, but they're usually
// long and complex, so we define them at the bottom of the file. This forward declaration
// tells C++ that we *will* define them later, but here are their signatures *now*.
void handleScienceCommand(const uint8_t* data, int length);
void shutdown();

MethaneSensor methaneSensor(METHANE_PIN);
BurtSerial serial(Device::Device_SCIENCE, handleScienceCommand, shutdown);
BurtCan<Can3> can(SCIENCE_COMMAND_ID, Device::Device_SCIENCE, handleScienceCommand, shutdown);

void setup() {
  can.setup();
  serial.setup();
}

void loop() {
  can.update();
  serial.update();
  sendData();
  delay(10);
}

void shutdown() {
  Serial.println("Device disconnected!");
}

void handleScienceCommand(const uint8_t* data, int length) {
  // The 2nd parameter is always MessageName_fields
  auto command = BurtProto::decode<ScienceCommand>(data, length, ScienceCommand_fields);
  if (command.state == ScienceState_COLLECT_DATA) Serial.println("Digging!");
}

void sendData() { 
  ScienceData data;
  data.methane = methaneSensor.read(); 
  // The 2nd parameter is always MessageName_fields
  can.send(SCIENCE_DATA_ID, &data, ScienceData_fields);
}
