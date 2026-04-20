#ifndef BLINKTEST_H
#define BLINKTEST_H
#include <Arduino.h>

#define LED_GPIO 2

// ========== Global Task Handle ==========
// Used by Task 4 (Web Server) to notify LED task of WiFi events
extern TaskHandle_t ledTaskHandle;

void blinkTest(void *pvParameters);
#endif // BLINKTEST_H