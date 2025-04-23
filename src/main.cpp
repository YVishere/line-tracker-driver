#include <Arduino.h>
#include "display.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  initDisplay(false);
  
  // drawSdJpeg("/bsr/Jonathan.jpeg", 130, 0);
  HeapAnim();
  Serial.println("Setup done.");
}

void loop() {
  // rotateColors();
  // delay(42);
  // drawSdJpeg("/test.jpg", 0, 0);

  // HeapAnim();
}
