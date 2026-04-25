#include "task_3_Lcd.h"

LiquidCrystal_I2C lcd(0x21, 16, 2);

void taskHandleLcd(void *pvParameters) {
    Wire.begin(11, 12); 
    
    if (xI2CSemaphore != NULL && xSemaphoreTake(xI2CSemaphore, portMAX_DELAY) == pdTRUE) {
        lcd.init();
        lcd.backlight();
        lcd.clear();
        xSemaphoreGive(xI2CSemaphore);
    }

    while (1) {
        State localState = BOOTING;
        if (xStateSemaphore != NULL && xSemaphoreTake(xStateSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
            localState = currentState;
            xSemaphoreGive(xStateSemaphore);
        }

        char sensorBuffer[17];
        snprintf(sensorBuffer, sizeof(sensorBuffer), "T:%.1fC H:%.1f%%  ", glob_temperature, glob_humidity);
        if (xI2CSemaphore != NULL && xSemaphoreTake(xI2CSemaphore, portMAX_DELAY) == pdTRUE) {

            lcd.setCursor(0, 0);
            switch (localState) {
                case BOOTING:         lcd.print("WiFi: BOOTING   "); break;
                case WIFI_CONNECTING: lcd.print("WiFi: CONNECTING"); break;
                case WIFI_CONNECTED:  lcd.print("WiFi: CONNECTED "); break;
                case WIFI_LOST:       lcd.print("WiFi: DISCONECT   "); break;
                default:              lcd.print("WiFi: UNKNOWN   "); break;
            }

            lcd.setCursor(0, 1);
            lcd.print(sensorBuffer);

            xSemaphoreGive(xI2CSemaphore);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}