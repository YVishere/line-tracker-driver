#include <Arduino.h>
#include "display.h"

void setup() {
  Serial.begin(115200);
  initDisplay(true);

  drawSdJpeg("/bsr/Jonathan.jpg", 0, 0);
}

void loop() {
  // drawSdJpeg("/test.jpg", 0, 0);
  // delay(5000);
}
