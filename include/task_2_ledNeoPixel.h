#ifndef TASK_2_LEDNEOPIXEL_H
#define TASK_2_LEDNEOPIXEL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern TaskHandle_t ledTaskHandle;
#define WIFI_CONNECTED_NOTIFY_BIT (1 << 0)
void taskHandleNeoPixel(void *pvParameters);

#endif // TASK_2_LEDNEOPIXEL_H