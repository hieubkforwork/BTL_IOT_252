#ifndef TASK_4_AP_WEBSERVER_H
#define TASK_4_AP_WEBSERVER_H
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> //JSON
#include <LittleFS.h>
#include <Update.h> //OTA
#include <PubSubClient.h>

#define LED_PIN GPIO_NUM_5

extern const char* ssid;
extern const char* password;
extern AsyncWebServer server;

void mountFlash(void *pvParameters);
void settingsWifi(void *pvParameters);
void webServerTask(void *pvParameters);
void webBackend(void *pvParameters);
void mqttCallback(char *topic, byte *payload, unsigned int length);
void handleMQTT();

#endif // TASK_4_AP_WEBSERVER_H
