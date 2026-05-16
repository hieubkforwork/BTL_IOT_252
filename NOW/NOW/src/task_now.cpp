#include "task_now.h"

static const String msg = "Hello ESP-NOW!";
static SemaphoreHandle_t send_sem;

// ===== CALLBACK =====
void dataReceived(uint8_t *address, uint8_t *data, uint8_t len, signed int rssi, bool broadcast)
{
  Serial.print("Received: ");
  Serial.printf("%.*s\n", len, data);
  Serial.printf("RSSI: %d dBm\n", rssi);
  Serial.printf("From: " MACSTR "\n", MAC2STR(address));
  Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");
}

// ===== LOGIC =====
static void taskNow()
{
  static uint32_t lastSend = 0;
  static unsigned int counter = 0;

  if (quickEspNow.readyToSendData() &&
      ((millis() - lastSend) > SEND_MSG_MSEC))
  {
    if (xSemaphoreTake(send_sem, 0) == pdTRUE)
    {
      lastSend = millis();

      char message[64];
      snprintf(message, sizeof(message), "%s %u", msg.c_str(), counter++);

      bool ok = quickEspNow.send(DEST_ADDR, (uint8_t *)message, strlen(message));

      if (ok)
      {
        Serial.println(">>>>>>>>>> Message sent");
      }
      else
      {
        xSemaphoreGive(send_sem);
      }
    }
  }
}

// ===== TASK =====
void taskHandleNow2(void *pvParameters)
{
  send_sem = xSemaphoreCreateBinary();
  xSemaphoreGive(send_sem);

  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin("realme", "hieu1234");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi...");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  int ch = WiFi.channel(); // 🔥 lấy channel thật

  quickEspNow.begin(ch, 0, false);
  Serial.printf("Connected to %s in channel %d\n", WiFi.SSID().c_str(), WiFi.channel());
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());

  quickEspNow.onDataRcvd(dataReceived);

  while (1)
  {
    //    taskNow();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}