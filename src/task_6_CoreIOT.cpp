#include "task_6_CoreIOT.h"

// ========== Connection State Machine ==========
// Tracks current connection status for robust lifecycle management
// (Enum definition is in task_6_CoreIOT.h as ConnectionState_t)

// Global state tracking
ConnectionState_t wifiState = STATE_INIT;
ConnectionState_t mqttState = STATE_INIT;
unsigned long lastWiFiReconnectAttempt = 0;  // Throttle WiFi reconnection attempts
unsigned long lastMQTTReconnectAttempt = 0;  // Throttle MQTT reconnection attempts
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;  // Min 5s between MQTT reconnect attempts
const unsigned long WIFI_RECONNECT_INTERVAL = 10000; // Min 10s between WiFi reconnect attempts

// ========== MQTT Client & Buffer ==========
WiFiClient wifiClient;           // WiFi transport layer for MQTT
PubSubClient mqttClient(wifiClient);  // MQTT client instance
char jsonBuffer[512];            // Serialization buffer for JSON payloads

bool initWiFi() {
    Serial.println("\n========== WiFi Initialization ==========");
    Serial.printf("Target SSID: %s\n", WIFI_SSID);
    
    // ========== Smart WiFi Mode Handling ==========
    // Check if Task 4 already set up dual mode (AP+STA)
    wifi_mode_t current_mode = WiFi.getMode();
    
    if (current_mode == WIFI_AP_STA) {
        // Task 4 (WebServer) already setup dual mode - keep it!
        Serial.println("Dual mode (AP+STA) already active from Task 4 WebServer");
        Serial.println("Preserving AP mode for web interface...");
    } else if (current_mode != WIFI_STA) {
        // Not in STA mode yet - set it (no Task 4 running or already checked)
        WiFi.mode(WIFI_STA);
        Serial.println("WiFi mode set to STA (Station)");
    } else {
        Serial.println("WiFi already in STA mode");
    }
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("WiFi STA connection initiated...");
    
    // ========== Exponential Backoff Retry Strategy ==========
    unsigned long retryDelay = 500;      // Start with 500ms
    int maxAttempts = 6;                 // 6 attempts = ~30 seconds total
    int attemptCount = 0;
    
    while (WiFi.status() != WL_CONNECTED && attemptCount < maxAttempts) {
        delay(retryDelay);  
        Serial.print("."); 
        
        retryDelay = (retryDelay * 2 > 8000) ? 8000 : retryDelay * 2;
        attemptCount++;
    }
    
    Serial.println(); 
    
    // ========== Connection Result ==========
    if (WiFi.status() == WL_CONNECTED) {
        wifiState = STATE_WIFI_CONNECTED;
        Serial.print("WiFi Connected! IP: ");
        Serial.println(WiFi.localIP());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        wifiState = STATE_WIFI_FAILED;
        Serial.printf("WiFi Connection Failed! Status: %d\n", WiFi.status());
        // Status codes: 0=IDLE, 1=NO_SSID_AVAIL, 2=SCAN_COMPLETED, 3=CONNECTED, 4=CONNECT_FAILED, 5=CONNECTION_LOST, 6=DISCONNECTED
        Serial.println("Will retry connection from wireless monitoring task...");
        return false;
    }
}
bool initMQTT() {
    Serial.println("\n========== MQTT Client Setup ==========");
    Serial.printf("Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    Serial.printf("Topic: %s\n", MQTT_TOPIC);
    
    // Configure MQTT broker address and port
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    
    // Set connection timeout (ms): prevents indefinite blocking
    mqttClient.setSocketTimeout(5000);  // 5 second socket timeout
    
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect with client ID
    if (mqttClient.connect("ESP32_TinyML_Client")) {
        mqttState = STATE_MQTT_CONNECTED;
        Serial.println(" SUCCESS!");
        Serial.printf("MQTT State: %d (0=MQTT_CONNECTED)\n", mqttClient.state());
        return true;
    } else {
        mqttState = STATE_MQTT_ERROR;
        Serial.println(" FAILED!");
        // Decode error state for debugging
        int state = mqttClient.state();
        Serial.print("Error Code: ");
        Serial.print(state);
        Serial.print(" - ");
        
        // MQTT error code descriptions
        switch (state) {
            case -4: Serial.println("CONNECT_TIMEOUT"); break;           // Server unreachable
            case -3: Serial.println("CONNECT_FAILED"); break;            // Network failure
            case -2: Serial.println("CONNECT_LOST"); break;              // Connection lost
            case -1: Serial.println("DISCONNECTED"); break;              // Disconnected
            case 0:  Serial.println("CONNECTED"); break;                 // OK
            case 1:  Serial.println("CONNECT_BAD_PROTOCOL"); break;      // Invalid protocol version
            case 2:  Serial.println("CONNECT_BAD_CLIENT_ID"); break;     // Client ID rejected
            case 3:  Serial.println("CONNECT_UNAVAILABLE"); break;       // MQTT server unavailable
            case 4:  Serial.println("CONNECT_BAD_CREDENTIALS"); break;   // Bad username/password
            case 5:  Serial.println("CONNECT_UNAUTHORIZED"); break;      // Not authorized
            default: Serial.println("UNKNOWN"); break;
        }
        Serial.println("Will retry from connection manager...");
        return false;
    }
}

void reconnectMQTT() {
    // Skip if already connected
    if (mqttClient.connected()) {
        return;
    }
    
    // Check if enough time has elapsed since last attempt (throttle)
    unsigned long now = millis();
    if (now - lastMQTTReconnectAttempt < MQTT_RECONNECT_INTERVAL) {
        return;  // Not ready to retry yet
    }
    
    // Attempt reconnection
    lastMQTTReconnectAttempt = now;
    Serial.print("[MQTT] Reconnecting...");
    
    if (mqttClient.connect("ESP32_TinyML_Client")) {
        mqttState = STATE_MQTT_CONNECTED;
        Serial.println(" SUCCESS!");
        Serial.print("[MQTT] Connected! Subscribing to ");
        Serial.println(MQTT_TOPIC);
    } else {
        mqttState = STATE_MQTT_ERROR;
        int errorCode = mqttClient.state();
        Serial.print(" FAILED (State: ");
        Serial.print(errorCode);
        Serial.println(")");
        
        // Log error details for troubleshooting
        if (errorCode == -4) Serial.println("[MQTT] Error: Connection timeout - broker unreachable");
        else if (errorCode == -3) Serial.println("[MQTT] Error: Connection failed - network error");
        else if (errorCode == 1) Serial.println("[MQTT] Error: Bad protocol version");
        else if (errorCode == 4) Serial.println("[MQTT] Error: Bad credentials");
        
        Serial.printf("[MQTT] Next retry in %ds...\n", MQTT_RECONNECT_INTERVAL / 1000);
    }
}

void publishTinyMLData(float temperature, float humidity, float confidence, 
                       const char* status, unsigned long inferenceTime, 
                       unsigned long freeRAM) {
    // ========== Connection Check & Reconnect ==========
    // Attempt reconnection if disconnected (uses throttling)
    if (!mqttClient.connected()) {
        reconnectMQTT();
        
        // If still not connected after reconnect attempt, abort publish
        if (!mqttClient.connected()) {
            Serial.println("[PUBLISH] MQTT not connected, message discarded");
            return;
        }
    }
    
    // ========== JSON Serialization ==========
    // Create JSON document with sensor readings and model output
    JsonDocument doc;  // Modern ArduinoJson v7+ - heap-allocated with auto-sizing
    doc["device_id"] = "ESP32_001";
    doc["temperature"] = temperature;    // From DHT20 sensor
    doc["humidity"] = humidity;          // From DHT20 sensor
    doc["confidence"] = confidence;      // Model output probability
    doc["status"] = status;              // Anomaly decision string
    doc["inference_time"] = inferenceTime;  // Neural network execution time (ms)
    doc["free_ram"] = freeRAM;           // For memory leak detection
    
    // Serialize to string buffer
    size_t bytes_written = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    if (bytes_written == 0) {
        Serial.println("[PUBLISH] JSON serialization failed (buffer too small)");
        return;
    }
    
    // ========== MQTT Publish ==========
    // Topic: esp32/telemetry
    // QoS: 1 (at-least-once delivery)
    bool publishSuccess = mqttClient.publish(MQTT_TOPIC, jsonBuffer, false);
    
    if (publishSuccess) {
        Serial.print("[PUBLISH] SUCCESS: ");
        Serial.println(jsonBuffer);
    } else {
        Serial.print("[PUBLISH] FAILED: Code=");
        Serial.println(mqttClient.state());
        Serial.print("[PUBLISH] Payload: ");
        Serial.println(jsonBuffer);
    }
}

void taskCoreMQTT(void *pvParameters) {
    // ========== Initialization Phase ==========
    Serial.println("\n========== CoreIOT MQTT Task Starting ==========");
    
    // Initialize WiFi connection
    bool wifiReady = initWiFi();
    if (wifiReady) {
        Serial.println("[INIT] WiFi initialization: SUCCESS");
    } else {
        Serial.println("[INIT] WiFi initialization: FAILED - will retry in main loop");
    }
    
    // Small delay to allow WiFi driver to stabilize
    vTaskDelay(2000);
    
    // Initialize MQTT client
    bool mqttReady = initMQTT();
    if (mqttReady) {
        Serial.println("[INIT] MQTT initialization: SUCCESS");
    } else {
        Serial.println("[INIT] MQTT initialization: FAILED - will retry in main loop");
    }
    
    Serial.println("[INIT] Starting connection management loop...");
    
    // ========== Main Connection Management Loop ==========
    unsigned long lastWiFiCheck = millis();
    unsigned long wifiCheckInterval = 10000;  // Check WiFi every 10 seconds
    
    while (1) {
        // ===== WiFi Connection Monitoring =====
        // Periodically verify WiFi connection and reconnect if needed
        unsigned long now = millis();
        if (now - lastWiFiCheck > wifiCheckInterval) {
            lastWiFiCheck = now;
            
            if (WiFi.status() != WL_CONNECTED) {
                Serial.print("[WiFi] Connection lost! Status: ");
                Serial.println(WiFi.status());
                
                // Attempt WiFi reconnection (uses throttling internally)
                if (initWiFi()) {
                    Serial.println("[WiFi] Reconnection: SUCCESS");
                    wifiState = STATE_WIFI_CONNECTED;
                    
                    // After WiFi restores, need to reconnect MQTT too
                    Serial.println("[MQTT] WiFi restored, attempting MQTT reconnect...");
                    reconnectMQTT();
                } else {
                    Serial.println("[WiFi] Reconnection: FAILED - will retry");
                    wifiState = STATE_WIFI_FAILED;
                }
            }
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            if (!mqttClient.connected()) {
                reconnectMQTT();
            } else {
                mqttClient.loop();
            }
        } else {
            Serial.print(".");  // Progress indicator while waiting for WiFi
        }
        
        // ===== Status Reporting =====
        // Log state for diagnostics (every ~5-10 seconds)
        static unsigned long lastStatusReport = 0;
        if (now - lastStatusReport > 10000) {
            lastStatusReport = now;
            Serial.printf("\n[STATUS] WiFi: %s | MQTT: %s | Heap: %u bytes\n",
                         WiFi.status() == WL_CONNECTED ? "OK" : "FAIL",
                         mqttClient.connected() ? "OK" : "FAIL",
                         xPortGetFreeHeapSize());
        }
        
        vTaskDelay(100);
    }
}
