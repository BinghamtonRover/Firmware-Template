# Teensy Demo Arduino Sketch

This repository is a demo for how to integrate Protobuf, CAN, and extra code in your Arduino project. The rest of this document will specify the structure and reasoning exactly, and this repository can serve as a demonstration of following these instructions.

## Overview

Firmware code is written as Arduino sketches. An Arduino sketch is defined as a folder (say, `Demo`), with a `.ino` file of the same name (like `Demo/Demo.ino`). 

- **Specialized code**: Interfacing with hardware can be complex and might depend on several other libraries
- **Protobuf**: Firmware has to interface with the Subsystems Pi. We use Protobuf to keep our data definitions in sync
- **CAN bus**: To send and receive data, we use the CAN bus protocol. 

## Specialized code

The [Arduino sketch specification](https://arduino.github.io/arduino-cli/0.20/sketch-specification/#additional-code-files) states that all code inside your sketch's `src` folder (`Demo/src`) are compiled. Say you're writing code to move the arm. You can make an `Arm` class: 
```cpp
// Demo/src/BURT_arm.h
#include <Arduino.h>  // needed for Arduino code, like Serial

class Arm {
	public:
		void moveTo(float x, float y, float z);
};
```
```cpp
// Demo/src/BURT_arm.cpp
#include "BURT_arm.h"

void Arm::moveTo(float x, float y, float z) { /* ... */ }
```
And then simply include it in your sketch: 
```cpp
// Demo/Demo.ino
#include "src/BURT_arm.h"

Arm arm;

void setup() { 
  arm.moveTo(0, 0, 0); 
}
```

## Protobuf overview

Google's [Protocol Buffers](https://protobuf.dev/), referred to as "Protobuf", allows us to synchronize our data formates across various platforms, languages, and implementations. However, the C++ installation of `protoc`, the Protobuf compiler, does not play nicely with Arduino's framework. To get around that issue, we use [`nanopb`](https://github.com/nanopb/nanopb) instead -- a smaller and simpler version of Protobuf. 

Integrating Protobuf into your code is a 3-step process: 

### Including the `.proto` files

Protobuf defines data in its own language, in a file with the `.proto` extension. We keep our Protobuf files in a [separate repository](https://github.com/BinghamtonRover/Protobuf), so you'll first need to include that in your code. The following commands assume your sketch is named `<SKETCH>` -- for example, this repository's sketch name is `Demo`. 

To keep our Protobuf files in sync, add them as a submodule: 
```bash
# Clones the Protobuf repository into <SKETCH>/src/Protobuf. They won't be compiled
git submodule add https://github.com/BinghamtonRover/Protobuf <SKETCH>/src/Protobuf
```

### Installing Protobuf and `nanopb`

You'll need to install Google's Protobuf compiler [here](https://github.com/protocolbuffers/protobuf/releases/latest). For C++, you only need to download the `protoc` files for your platform. Once downloaded, extract the files and put them in your PATH. 

Now you need to install nanopb. There are two parts to `nanopb`: 

1. The executable that runs `protoc` to generate your code
2. The libraries that the generated code should include

The C++ libraries are included for you (see below), but you'll have to get the Python-based executable yourself:
```bash
# Make sure to install Python if you don't have it already
python3 -m pip install nanopb
```

### Using the Protobuf files

Finally, you can generate the C++ code for your Proto files. These files will be stored in your sketch's `src` folder -- which compiles _everything_ -- so you only want to generate the files you need. Going off the Arm example, say you want to generate `arm.proto`: 

To generate Protobuf code: 
```bash
# Be sure to create the generated folder if it doesn't exist
nanopb_generator -I <SKETCH>/src/Protobuf -D <SKETCH>/src/generated arm.proto
```

Now in your sketch: 
```cpp
// Demo/Demo.ino
#include "src/generated/arm.pb.h"
```

You now have access to all the data defined in the Proto files. Whatever is declared as a `message` in there will be generated as a `struct` in the `.pb.h` files.

## Using CAN bus

Our firmware communicates with a Raspberry Pi using the [CAN bus protocol](https://en.wikipedia.org/wiki/CAN_bus). Our `CAN` repository provides an Arduino library that bundles everything you'll need for CAN bus and Protobuf. Follow the instructions in the README there to install that library. 

There are examples in that repository, but here's the simple gist: 

### Initialization

To start, include the library, initialize on startup and update it on every loop

```cpp
// Demo/Demo.ino
#include <BURT_can.h>

void setup() {
	BurtCan::setup();
	// ...
}

void loop() {
	BurtCan::update();
	// ...
}
```

### Sending messages

You can easily send a Protobuf message using the `send` function. Say you want to send some Protobuf-generated struct `ElectricalData`: 

```cpp
#include "src/generated/electrical.pb.h"

// See the CAN repository for details
#define ELECTRICAL_DATA_ID 0x23

void sendData() {
	ElectricalData data;
	data.v5_voltage = 5.0;
	// The 2nd parameter is always MessageName_fields
	BurtCan::send(ELECTRICAL_DATA_ID, ElectricalData_fields, &data);
}

void loop() {
  BurtCan::update();
  sendData();
  delay(10);
}
```

### Receiving messages

You can define a callback to run when a new CAN frame of a specific ID is received. You have to decode it as the correct Protobuf message yourself, based on the ID. Assuming the data is a Protobuf message, say `ScienceCommand`: 

```cpp
#include <BURT_proto.h>

#include "src/generated/science.pb.h"

void handleScienceCommand(const CanMessage& message) {
	// The 2nd parameter is always MessageName_fields
	ScienceCommand command = BurtProto::decode<ScienceCommand>(
		message.buf, ScienceCommand_fields
	);

	if (command.dig) Serial.println("Digging!");
}
```

Now all that's left is to register that handler to run:

```cpp
// See the CAN repository for details
#define SCIENCE_COMMAND_ID 0xC3

void setup() {
	BurtCan::setup();
	BurtCan::registerHandler(SCIENCE_COMMAND_ID, handleScienceCommand);
	// more handlers here...
}
```

Now `handleScienceCommand` will run whenever a new CAN frame is received with ID `0xC3`, decode that frame as a `ScienceCommand` instance, and check whether it should dig.
