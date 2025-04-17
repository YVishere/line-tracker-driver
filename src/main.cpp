#include <Arduino.h>
#include "display.h"

void setup() {
  Serial.begin(115200);
  initDisplay(false);
  
  drawSdJpeg("/bsr/Jonathan.jpeg", 130, 0);
  delay(5000);
}

void loop() {
  // rotateColors();
  // delay(42);
  // drawSdJpeg("/test.jpg", 0, 0);

  HeapAnim();
}
