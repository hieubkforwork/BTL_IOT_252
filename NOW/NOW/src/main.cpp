#include "global.h"
#include "task_now.h"


void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(taskHandleNow2, "NOW Task", 4096,NULL,1,NULL,1);
}

void loop() {
  // put your main code here, to run repeatedly:
}
