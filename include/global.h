#ifndef GLOBAL_H
#define GLOBAL_H

#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
typedef struct {
    float temperature;
    float humidity;
    float confidence;
    char status[16];
} SensorData_t;

extern QueueHandle_t xQueue_SensorData;
extern SemaphoreHandle_t xSemaphore_NeoPixelUpdate;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

// Global sensor data for TinyML task
extern float glob_temperature;
extern float glob_humidity;

extern WiFiClient espClient;
extern PubSubClient client;

#endif // GLOBAL_H