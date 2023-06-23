#include <AltSoftSerial.h>

AltSoftSerial mySerial;
const int inputPin = D4;   // Input pin for data reading

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
}

void loop () {
}
