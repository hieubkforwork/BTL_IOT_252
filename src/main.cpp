
#include "blinktest.h"
#include "task_2_ledNeoPixel.h"
#include "task_5_TinyML.h"
#include "task_4_AP_Webserver.h"
#include <LittleFS.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  xTaskCreate(taskReadSensor, "Sensor Reader", 3072, NULL, 3, NULL);     
  xTaskCreate(taskHandleNeoPixel, "NeoPixel Control", 4096, NULL, 2, NULL);
  xTaskCreate(blinkTest, "LED Control", 2048, NULL, 1, NULL);
  xTaskCreate(tiny_ml_task, "TinyML Task", 4096, NULL, 2, NULL);
  xTaskCreatePinnedToCore(webServerTask, "WebServerTask", 8192, NULL, 1, NULL, 1);
}
 
void loop() {

}
