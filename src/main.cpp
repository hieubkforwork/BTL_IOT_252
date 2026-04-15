
#include "blinktest.h"
#include "task_2_ledNeoPixel.h"
#include "task_5_TinyML.h"

void setup() {
  Serial.begin(115200);

  xTaskCreate(taskReadSensor, "Sensor Reader", 3072, NULL, 3, NULL);     
  xTaskCreate(taskHandleNeoPixel, "NeoPixel Control", 4096, NULL, 2, NULL);
  xTaskCreate(blinkTest, "LED Control", 2048, NULL, 1, NULL);
  xTaskCreate(tiny_ml_task, "TinyML Task", 4096, NULL, 2, NULL);
}
 
void loop() {

}
