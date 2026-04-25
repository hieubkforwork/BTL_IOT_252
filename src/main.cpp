#include <Arduino.h>
#include "task_1_ledBlink.h"
#include "task_2_ledNeoPixel.h"
#include "task_3_Lcd.h"
#include "task_4_AP_Webserver.h"
#include "task_5_TinyML.h"
#include "task_6_CoreIOT.h"
#include <LittleFS.h>

State currentState = BOOTING; 
SemaphoreHandle_t xStateSemaphore = NULL;
TaskHandle_t ledTaskHandle = NULL; 

void setup() {
    Serial.begin(115200);
    delay(1000); 
    xI2CSemaphore = xSemaphoreCreateMutex();
    xStateSemaphore = xSemaphoreCreateMutex();
    if (xStateSemaphore != NULL) {
        xTaskCreate(
            taskHandleLed, 
            "Task_1_LED", 
            4096, 
            NULL, 
            1, 
            &ledTaskHandle 
        );

        xTaskCreate(
            taskHandleLcd,     
            "LCD_Task",       
            4096,              
            NULL,              
            2,                 
            NULL               
        );

        xTaskCreate(taskReadSensor, "Sensor Reader", 3072, NULL, 3, NULL);     
        xTaskCreate(taskHandleNeoPixel, "NeoPixel Control", 4096, NULL, 2, NULL);
        xTaskCreate(tiny_ml_task, "TinyML Task", 4096, NULL, 3, NULL);
        xTaskCreatePinnedToCore(webServerTask, "WebServerTask", 8192, NULL, 1, NULL, 1);
        xTaskCreate(coreiot_task, "CoreIOT_Task", 4096, NULL, 2, NULL);
    }
}

void loop() {
    // Để trống vì đã có RTOS Tasks xử lý
}