#include "blinktest.h"

// ========== Global Task Handle Definition ==========
TaskHandle_t ledTaskHandle = NULL;  // Initialized at task creation in main.cpp

void blinkTest(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  
  while(1) {               
    digitalWrite(LED_GPIO, HIGH);  // turn the LED ON
    vTaskDelay(1000);
    digitalWrite(LED_GPIO, LOW);  // turn the LED OFF
    vTaskDelay(1000);
  }
}