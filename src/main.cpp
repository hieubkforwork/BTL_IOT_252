
#include "blinktest.h"
#include "task_4_AP_Webserver.h"
#include "task_2_ledNeoPixel.h"
#include "task_7_NOW.h"
#include "task_8_rpc.h"
#include <LittleFS.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  setupRPC();
 // xTaskCreate(blinkTest,"LED Control",2048,NULL,1,NULL);
 // xTaskCreatePinnedToCore(taskHandleNeoPixel,"NeoPixel Task",2048,NULL,1,&ledTaskHandle,0);
 //xTaskCreatePinnedToCore(taskHandleWebServer,"WebServerTask",8192,NULL,1,NULL,1);
 //xTaskCreatePinnedToCore(taskHandleNow,"ESP-NOW Task",4096,NULL,1,NULL,1);
 xTaskCreatePinnedToCore(taskRPC,"RPC Task",8192,NULL,1,NULL,0);
 xTaskCreatePinnedToCore(taskSensor,"Sensor Task",4096,NULL,1,NULL,1);
}

void loop() {
  // Không cần dùng loop khi dùng task
}
