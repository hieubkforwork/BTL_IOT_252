#include "task_7_NOW.h"

static const String msg = "Hello ESP-NOW!";
static SemaphoreHandle_t send_sem;

// ===== CALLBACK =====
void dataSent(uint8_t *address, uint8_t status)
{
  if (send_sem != NULL)
  {
    xSemaphoreGive(send_sem);
  }
  Serial.printf("Message sent to " MACSTR ", status: %d\n", MAC2STR(address), status);
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
void taskHandleNow(void *pvParameters)
{
  send_sem = xSemaphoreCreateBinary();
  xSemaphoreGive(send_sem);
  wifi_info_t info;

  // ===== CHỜ WIFI CONNECT =====
  if (xQueueReceive(wifiQueue, &info, portMAX_DELAY))
  {
      Serial.printf("[NOW] Got MAC: %s, CH: %d\n", info.mac, info.channel);

      // ===== INIT ESPNOW =====
      if (!quickEspNow.begin(info.channel, 0, false))
      {
          Serial.println("[NOW] ESP-NOW init failed");
          vTaskDelete(NULL);
      }
  }

  quickEspNow.onDataSent(dataSent);

  while (1)
  {
    taskNow();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}