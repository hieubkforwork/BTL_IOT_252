#include "task_1_ledBlink.h"

/**
 * @brief taskHandleLed
 * 
 * Redefine the LED blinking behavior to respond to different temperature conditions (at least 3 different behaviors).
 * Use semaphores to manage task synchronization in your implementation.
 * Ensure that the condition handling and semaphore logic are clearly explained in your code and report.
 * 
 * @param pvParameters 
 */

#define LED_GPIO GPIO_NUM_48 
#define WIFI_CONNECTED_NOTIFY_BIT (1 << 0)
#define WIFI_CONNECTING_NOTIFY_BIT (1 << 1) // Thêm cờ Connecting
#define WIFI_LOST_NOTIFY_BIT       (1 << 2) // Thêm cờ Lost

State lastState = (State)-1;

void taskHandleLed(void *pvParameters) {
    pinMode(LED_GPIO, OUTPUT);
    bool ledState = false;

    while (1) {
        uint32_t notifiedValue = 0;

        if (xTaskNotifyWait(0x00, ULONG_MAX, &notifiedValue, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            if (xSemaphoreTake(xStateSemaphore, portMAX_DELAY)) {
                if (notifiedValue & WIFI_CONNECTED_NOTIFY_BIT) {
                    currentState = WIFI_CONNECTED;
                }
                else if (notifiedValue & WIFI_CONNECTING_NOTIFY_BIT) {
                    currentState = WIFI_CONNECTING;
                }
                else if (notifiedValue & WIFI_LOST_NOTIFY_BIT) {
                    currentState = WIFI_LOST;
                }
                xSemaphoreGive(xStateSemaphore);
            }
        }

        State localState = BOOTING;
        if (xSemaphoreTake(xStateSemaphore, portMAX_DELAY)) {
            localState = currentState;
            xSemaphoreGive(xStateSemaphore);
        }

        if (localState != lastState) {
            switch (localState) {
                case BOOTING:         Serial.println("[LED] Trạng thái: BOOTING (Nháy cực nhanh 100ms)"); break;
                case WIFI_CONNECTING: Serial.println("[LED] Trạng thái: CONNECTING (Nháy đều 500ms)"); break;
                case WIFI_CONNECTED:  Serial.println("[LED] Trạng thái: CONNECTED (Sáng liên tục)"); break;
                case WIFI_LOST:       Serial.println("[LED] Trạng thái: LOST (Nháy chậm 1500ms)"); break;
            }
            lastState = localState;
        }

        switch (localState) {
            case BOOTING:
                ledState = !ledState;
                digitalWrite(LED_GPIO, ledState);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case WIFI_CONNECTING:
                ledState = !ledState;
                digitalWrite(LED_GPIO, ledState);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case WIFI_CONNECTED:
                digitalWrite(LED_GPIO, HIGH);
                vTaskDelay(pdMS_TO_TICKS(200)); 
                break;

            case WIFI_LOST:
                ledState = !ledState;
                digitalWrite(LED_GPIO, ledState);
                vTaskDelay(pdMS_TO_TICKS(1500)); 
                break;

            default:
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }
}