#include "blinktest.h"

void blinkTest(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  
  while(1) {                        
    digitalWrite(LED_GPIO, HIGH);  // turn the LED ON
    vTaskDelay(1000);
    digitalWrite(LED_GPIO, LOW);  // turn the LED OFF
    vTaskDelay(1000);
  }
}