#ifndef BLINKTEST_H
#define BLINKTEST_H
#include <Arduino.h>

#define LED_GPIO GPIO_NUM_48
float glob_temperature = 0.0f;
float glob_humidity = 0.0f;
SemaphoreHandle_t xSemaphore_NeoPixelUpdate = NULL;
QueueHandle_t xQueue_SensorData = NULL;
SemaphoreHandle_t xI2CSemaphore = NULL;
#endif // BLINKTEST_H