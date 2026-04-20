#ifndef TASK_6_COREIOT_H
#define TASK_6_COREIOT_H
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "global.h"

/**
 * TASK 6: CoreIOT MQTT Cloud Integration - Header File
 * 
 * Configuration & Interface for WiFi + MQTT cloud relay.
 * Enables ESP32 to publish TinyML inference results to local MQTT broker,
 * which relays to CoreIOT cloud platform for remote monitoring.
 */

// ========== WiFi Configuration ==========
// REQUIRED: Update with your local network credentials
#define WIFI_SSID "YOUR_SSID"           
#define WIFI_PASSWORD "YOUR_PASSWORD"   

// ========== MQTT Local Broker Configuration ==========
// Typically runs on laptop/Raspberry Pi - monitor via MQTT Explorer
#define MQTT_BROKER "192.168.1.100"     // Replace with laptop IP address
#define MQTT_PORT 1883                  // Standard MQTT port (unencrypted)
#define MQTT_TOPIC "esp32/telemetry"    // Topic where ESP32 publishes sensor data

// ========== Connection State Tracking ==========
// Exported for diagnostics and monitoring
extern enum {
    STATE_INIT = 0,
    STATE_WIFI_CONNECTING,
    STATE_WIFI_CONNECTED,
    STATE_WIFI_FAILED,
    STATE_MQTT_CONNECTING,
    STATE_MQTT_CONNECTED,
    STATE_MQTT_ERROR
} wifiState, mqttState;

// ========== Function Declarations ==========

/**
 * initWiFi() - Establish WiFi connection with exponential backoff retry
 * 
 * Returns: true if connected, false if timeout (will retry from task)
 */
bool initWiFi();

/**
 * initMQTT() - Initialize MQTT client and connect to broker
 * 
 * Returns: true if connected, false if connection failed (will retry from task)
 */
bool initMQTT();

/**
 * reconnectMQTT() - Attempt MQTT reconnection with throttling
 * 
 * Throttles reconnection attempts (min 5s interval) to prevent CPU exhaustion
 * during broker downtime. Called automatically from taskCoreMQTT().
 */
void reconnectMQTT();

/**
 * publishTinyMLData() - Serialize and publish TinyML results to MQTT broker
 * 
 * Parameters:
 *   - temperature: Sensor reading (°C)
 *   - humidity: Sensor reading (%)
 *   - confidence: Model output confidence [0.0-1.0]
 *   - status: "NORMAL" or "ANOMALY" decision string
 *   - inferenceTime: Neural network execution time (ms)
 *   - freeRAM: Available heap memory (bytes) - for debugging memory leaks
 * 
 * Note: Called from tiny_ml_task() every 5 seconds
 */
void publishTinyMLData(float temperature, float humidity, float confidence, 
                       const char* status, unsigned long inferenceTime, 
                       unsigned long freeRAM);

/**
 * taskCoreMQTT() - FreeRTOS task managing WiFi/MQTT lifecycle
 * 
 * Responsibilities:
 *   - One-time WiFi + MQTT initialization
 *   - Periodic connection monitoring (every 10 seconds)
 *   - Automatic reconnection with exponential backoff
 *   - MQTT message loop (keepalive + incoming subscriptions)
 * 
 * Parameters: pvParameters (unused, required by FreeRTOS task signature)
 * 
 * Task Properties:
 *   - Stack Size: 4096 bytes
 *   - Priority: 2 (mid-priority)
 *   - Affinity: Auto-assigned core
 *   - Schedule: 100ms loop (FreeRTOS preemption)
 */
void taskCoreMQTT(void *pvParameters);

#endif // TASK_6_COREIOT_H