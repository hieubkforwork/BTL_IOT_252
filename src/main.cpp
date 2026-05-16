
#include "blinktest.h"
#include "task_2_ledNeoPixel.h"
#include "task_5_TinyML.h"
#include "task_4_AP_Webserver.h"
#include "task_6_CoreIOT.h"
#include <LittleFS.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  xBinarySemaphoreInternet = xSemaphoreCreateBinary();
  xSemaphoreGive(xBinarySemaphoreInternet);

  // Queue length = 1 because xQueueOverwrite is used to keep only the latest sensor sample.
  xQueue_SensorData = xQueueCreate(1, sizeof(SensorData_t));

  if (xQueue_SensorData == NULL) {
      Serial.println("Queue create FAILED!");
  } else {
      Serial.println("Queue created successfully");
  }

  // Priorities chosen so sensor acquisition and ML inference are higher than display/visual feedback.
  xTaskCreate(taskReadSensor, "Sensor Reader", 3072, NULL, 4, NULL);
  xTaskCreate(tiny_ml_task, "TinyML Task", 4096, NULL, 3, NULL);
  xTaskCreatePinnedToCore(webServerTask, "WebServerTask", 8192, NULL, 2, NULL, 1);
  xTaskCreate(coreiot_task, "CoreIOT", 8192, NULL, 2, NULL);
  xTaskCreate(taskHandleNeoPixel, "NeoPixel Control", 4096, NULL, 1, NULL);
  xTaskCreate(blinkTest, "LED Control", 2048, NULL, 1, &ledTaskHandle);
}
 
void loop() {

}
