
#include "blinktest.h"

void setup() {
  Serial.begin(115200);

  xTaskCreate(blinkTest,"LED Control",2048,NULL,1,NULL);
}

void loop() {
  // Không cần dùng loop khi dùng task
}
