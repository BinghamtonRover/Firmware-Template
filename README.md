# Teensy Demo Arduino Sketch

This repository is a demo for how to integrate Protobuf, CAN, and extra code in your Arduino project. The rest of this document will specify the structure and reasoning exactly, and this repository can serve as a demonstration of following these instructions.

## Overview

Firmware code is written as Arduino sketches. An Arduino sketch is defined as a folder (say, `Demo`), with a `.ino` file of the same name (like `Demo/Demo.ino`). 

- **Specialized code**: Interfacing with hardware can be complex and might depend on several other libraries
- **Protobuf**: Firmware has to interface with the Subsystems Pi. We use Protobuf to keep our data definitions in sync
- **CAN bus**: To send and receive data, we use the CAN bus protocol. 

## Specialized code

Often, hardware-specific code can get complex and unwieldly, and is better relegated to separate C++ files. The same is true for overly complex logic. Try to reserve the `.ino` file for high-level logic, and relegate the low-level implementation to other C++ files. The [Arduino sketch specification](https://arduino.github.io/arduino-cli/0.20/sketch-specification/#additional-code-files) states that all code inside your sketch's `src` folder (eg, `Demo/src`) are compiled, so we’ll put them there and `#include` them later. For example, say you're writing code for the EA team – you might want a `MethaneSensor` class: 
```cpp
// Demo/src/BURT_methane.h
#include <Arduino.h>  // needed for Arduino code, like Serial

class MethaneSensor {
  private: 
  	int pin;
  
	public:
  	MethaneSensor(int pin) : pin(pin);
		float read();
};
```
```cpp
// Demo/src/BURT_methane.cpp
#include "BURT_methane.h"

void MethaneSensor::read() { return analogRead(pin) }  // insert real logic here
```
And then simply include it in your sketch: 
```cpp
// Demo/Demo.ino
#include "src/BURT_methane.h"

#define METHANE_PIN 12

MethaneSensor methaneSensor(METHANE_PIN);

void sendData() { 
  float methane = methaneSensor.read(); 
  // ...
}
```

The goal is to make the `.ino` file more readable, and no hardware-specific logic should be there. If you find yourself including a lot of header files, you can simplify them: 

```cpp
// Demo/BURT_science.h
#include "src/BURT_methane.h"
// include more BURT_x.h libraries here
```

```cpp
// Demo/Demo.ino
#include "BURT_science.h"  // This file includes all necessary classes
```



## Protobuf overview

Google's [Protocol Buffers](https://protobuf.dev/), referred to as "Protobuf", allows us to synchronize our data formats across various platforms, languages, and implementations. However, the C++ installation of `protoc`, the Protobuf compiler, does not play nicely with Arduino's framework. To get around that issue, we use [`nanopb`](https://github.com/nanopb/nanopb) instead -- a smaller and simpler version of Protobuf. 

Integrating Protobuf into your code is a 3-step process: 

### Including the `.proto` files

Protobuf defines data in its own language, in a file with the `.proto` extension. We keep our Protobuf files in a [separate repository](https://github.com/BinghamtonRover/Protobuf), so you'll first need to include that in your code. The following commands assume your sketch is named `Demo` -- adapt them for your repository. 

To keep our Protobuf files in sync, add them as a submodule: 
```bash
# Clones the Protobuf repository into a new Protobuf folder. They won't be compiled
git submodule add https://github.com/BinghamtonRover/Protobuf
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

Finally, you can generate the C++ code for your Proto files. These files will be stored in your sketch's `src` folder -- which compiles _everything_ -- so you only want to generate the files you need. Continuing the science example, say you want to generate `science.proto`: 

To generate Protobuf code: 
```bash
# Be sure to create the generated folder if it doesn't exist
nanopb_generator -I Protobuf -D Demo science.proto
```

This generates a file `Demo/science.pb.h`. Now, in your sketch: 
```cpp
// Demo/Demo.ino
#include "science.pb.h"
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

You can easily send a Protobuf message using the `send` function. Say you want to send some Protobuf-generated struct `ScienceData`: 

```cpp
#include "src/BURT_methane.h"
#include "src/electrical.pb.h"

#define SCIENCE_DATA_ID 0x27  // See the CAN repository for details

#define METHANE_PIN 12

MethaneSensor methaneSensor(12);

void sendData() {
	ScienceData data;
	data.methane = methaneSensor.read();
	// The 2nd parameter is always MessageName_fields
	BurtCan::send(SCIENCE_DATA_ID, ScienceData_fields, &data);
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
#include "src/science.pb.h"

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

Now `handleScienceCommand` will run whenever a new CAN frame is received with ID `0x83`, decode that frame as a `ScienceCommand` instance, and check whether it should dig.
