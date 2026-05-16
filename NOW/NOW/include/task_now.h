#ifndef TASK_NOW_H
#define TASK_NOW_H

#include "global.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <QuickEspNow.h>

#define USE_BROADCAST 0 // Set this to 1 to use broadcast communication
#if USE_BROADCAST != 1
static uint8_t receiver[] = { 0x08, 0xF9, 0xE0, 0xAC, 0x42, 0xDC };
#define DEST_ADDR receiver
#else //USE_BROADCAST != 1
#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS 
#endif //USE_BROADCAST != 1

extern QueueHandle_t wifiQueue;

const unsigned int SEND_MSG_MSEC = 2000;

void taskHandleNow2(void *pvParameters);

#endif // TASK_7_NOW_H