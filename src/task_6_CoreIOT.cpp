#include "task_6_CoreIOT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "global.h"

// ----------- CẤU HÌNH TỪ VÍ DỤ CỦA THẦY -----------
const char* coreIOT_Server = "app.coreiot.io";  
const char* coreIOT_Token = "UeF27R1NHbQu2uQ9kOqh";   // Device Access Token
const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("[Task6] Attempting MQTT connection...");
    
    // Attempt to connect (username=token, password=empty)
    if (client.connect("ESP32_BTL_IOT", coreIOT_Token, NULL)) {
      Serial.println("connected to CoreIOT Server!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("[Task6] Subscribed to RPC topics");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(5000)); // Dùng vTaskDelay của RTOS
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.println("deserializeJson() failed");
    return;
  }

  const char* method = doc["method"];
  if (strcmp(method, "setStateLED") == 0) {
    const char* params = doc["params"];
    if (strcmp(params, "ON") == 0) {
      Serial.println("Device turned ON.");
      // TODO: Thêm lệnh bật đèn LED (Ví dụ: digitalWrite(LED_PIN, HIGH); )
    } else {   
      Serial.println("Device turned OFF.");
      // TODO: Thêm lệnh tắt đèn LED
    }
  } else {
    Serial.print("Unknown method: ");
    Serial.println(method);
  }
}

void setup_coreiot() {
  Serial.println("[Task6] Đang chờ WiFi kết nối...");
  
  // Kiểm tra trạng thái WiFi thay vì dùng Semaphore để đơn giản hóa
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }

  Serial.println("\n[Task6] WiFi Ready! Bắt đầu cấu hình MQTT.");
  client.setServer(coreIOT_Server, mqttPort);
  client.setCallback(callback);
}

void coreiot_task(void *pvParameters) {
    setup_coreiot();

    while(1) {
        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        // Gửi dữ liệu Cảm biến & TinyML lên bảng điều khiển CoreIOT
        StaticJsonDocument<256> doc;
        doc["temperature"] = glob_temperature;
        doc["humidity"] = glob_humidity;

        char buffer[256];
        serializeJson(doc, buffer);
        
        client.publish("v1/devices/me/telemetry", buffer);
        Serial.println("[Task6] Đã đẩy dữ liệu: " + String(buffer));

        // Publish mỗi 10 giây theo yêu cầu của thầy
        vTaskDelay(pdMS_TO_TICKS(2000));  
    }
}