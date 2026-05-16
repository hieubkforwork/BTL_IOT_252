#ifndef   TASK_1_LEDBLINK_H
#define   TASK_1_LEDBLINK_H
#include <Arduino.h>
#include "freertos/semphr.h"

enum State {
    BOOTING,         
    WIFI_CONNECTING, 
    WIFI_CONNECTED,  
    WIFI_LOST       
};
void taskHandleLed(void *pvParameters);
extern State currentState;
extern SemaphoreHandle_t xStateSemaphore;
extern TaskHandle_t ledTaskHandle;
#endif // TASK_1_LEDBLINK_H