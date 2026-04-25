#ifndef TASK_3_LCD_H
#define TASK_3_LCD_H
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "global.h"
#include "task_1_ledBlink.h"

// Khai báo hàm task
void taskHandleLcd(void *pvParameters);
#endif // TASK_3_LCD_H