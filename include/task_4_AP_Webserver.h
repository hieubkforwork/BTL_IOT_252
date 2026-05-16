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
#include "global.h"

// ===== HARDWARE =====
#define LED_PIN GPIO_NUM_5

// ===== WIFI NOTIFY FLAGS =====
#define WIFI_CONNECTED_NOTIFY_BIT   (1 << 0)
#define WIFI_CONNECTING_NOTIFY_BIT  (1 << 1)
#define WIFI_LOST_NOTIFY_BIT        (1 << 2)

// ===== GLOBAL OBJECTS =====
extern AsyncWebServer server;
extern PubSubClient client;

// ===== TASK HANDLE =====
extern TaskHandle_t ledTaskHandle;

// ===== WIFI / FILESYSTEM =====
void mountFlash(void *pvParameters);
void settingsWifi(void *pvParameters);
void handleWiFiNotify();

// ===== WEB SERVER =====
void webBackend(void *pvParameters);
void webServerTask(void *pvParameters);

// ===== MQTT CORE =====
void mqttCallback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();      // 🔥 NEW
void mqttLoop();           // 🔥 NEW
void publishTelemetry();   // 🔥 NEW

#endif