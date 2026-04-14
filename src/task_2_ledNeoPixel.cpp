#include "task_2_ledNeoPixel.h"

/**
 * @brief taskHandleNeoPixel
 * 
 *	Redefine NeoPixel (RGB LED) color patterns to represent different humidity levels (at least 3 levels/colors).
 *	Utilize semaphore synchronization technique for updating and displaying color changes.
 *	Clearly show the mapping between humidity value ranges and colors.
 * 
 * @param pvParameters 
 */

TaskHandle_t ledTaskHandle = NULL;

 void taskHandleNeoPixel(void *pvParameters)
{
    uint32_t notifyValue;

    while (1)
    {
        // Chờ notification từ WebTask
        if (xTaskNotifyWait(
                0x00,                 // clear bit on entry
                ULONG_MAX,           // clear all bits on exit
                &notifyValue,        // nhận giá trị notify
                portMAX_DELAY) == pdTRUE)
        {
            // Check đúng bit mình cần
            if (notifyValue & WIFI_CONNECTED_NOTIFY_BIT)
            {
                Serial.println("Test Received WiFi Connected -> LED ON ");
            }
        }
    }
  }