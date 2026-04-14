
#include "blinktest.h"
#include "task_4_AP_Webserver.h"
#include "task_2_ledNeoPixel.h"
#include <LittleFS.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

//  xTaskCreate(blinkTest,"LED Control",2048,NULL,1,NULL);
  xTaskCreatePinnedToCore(taskHandleNeoPixel,"NeoPixel Task",2048,NULL,1,&ledTaskHandle,0);
  xTaskCreatePinnedToCore(webServerTask,"WebServerTask",8192,NULL,1,NULL,1);
}

void loop() {
  // Không cần dùng loop khi dùng task
}
