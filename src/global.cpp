#include "global.h"

SemaphoreHandle_t xSemaphore_NeoPixelUpdate = NULL;
QueueHandle_t xQueue_SensorData = NULL;

// Global sensor data for TinyML task
float glob_temperature = 25.0;
float glob_humidity = 55.0;
