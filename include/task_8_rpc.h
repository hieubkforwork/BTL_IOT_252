#ifndef TASK_8_RPC_H
#define TASK_8_RPC_H
#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "DHT20.h"



void setupRPC();
void taskRPC(void *pvParameters);
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void taskSensor(void *pvParameters);

#endif // TASK_8_RPC_H