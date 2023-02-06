// Demo/src/BURT_arm.cpp
#include "BURT_arm.h"

void Arm::moveTo(float x, float y, float z) {
	Serial.print("Moving to (");
	Serial.print(x);
	Serial.print(", ");
	Serial.print(y);
	Serial.print(", ");
	Serial.print(z);
	Serial.println(")");
}
