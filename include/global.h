#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

typedef struct {
  float temperature;
  float humidity;
} SensorData_t;

extern SemaphoreHandle_t xSemaphore_NeoPixelUpdate;
extern QueueHandle_t xQueue_SensorData;

// Global sensor data for TinyML task
extern float glob_temperature;
extern float glob_humidity;

#endif // GLOBAL_H