#ifndef TASK_4_AP_WEBSERVER_H
#define TASK_4_AP_WEBSERVER_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN GPIO_NUM_5
#define WIFI_CONNECTED_NOTIFY_BIT (1 << 0)

extern AsyncWebServer server;

// Handle của task LED
extern TaskHandle_t ledTaskHandle;

void mountFlash(void *pvParameters);
void settingsWifi(void *pvParameters);
void webServerTask(void *pvParameters);
void webBackend(void *pvParameters);
void mqttCallback(char *topic, byte *payload, unsigned int length);
void handleMQTT();
void handleWiFiNotify();

#endif