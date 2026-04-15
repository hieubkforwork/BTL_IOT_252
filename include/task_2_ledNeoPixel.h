#ifndef TASK_2_LEDNEOPIXEL_H
#define TASK_2_LEDNEOPIXEL_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "global.h"

#define DHT20_I2C_SDA GPIO_NUM_11  // SDA pin cho DHT20
#define DHT20_I2C_SCL GPIO_NUM_12  // SCL pin cho DHT20
#define DHT20_I2C_FREQ 100000     

#define SENSOR_READ_INTERVAL_MS 2000  // Đọc cảm biến mỗi 2 giây
#define SENSOR_MAX_RETRY 3             // Retry 3 lần nếu đọc thất bại
#define QUEUE_SIZE 1                   // RTOS Queue size = 1 (for xQueueOverwrite - keeps latest data only)

#define NEOPIXEL_PIN GPIO_NUM_45  // Pin GPIO nối với NeoPixel (Yolo UNO board)
#define NEOPIXEL_COUNT 1        

#define TEMP_BLUE_MAX 20        // < 20°C: Blue
#define TEMP_GREEN_MAX 28       // 20-28°C: Green
#define TEMP_YELLOW_MAX 31      // 28-31°C: Yellow (changed from 32)
#define TEMP_RED_MAX 100        // > 31°C: Red

#define HUMIDITY_DRY_MAX 45
#define HUMIDITY_NORMAL_MAX 70 

#define BLINK_SLOW_ON 1000     
#define BLINK_SLOW_OFF 1000    

#define BLINK_FAST_ON 300      
#define BLINK_FAST_OFF 300     

#define BLINK_STABLE_OFF 0      

#define COLOR_BLUE    0x0000FF  // RGB (0, 0, 255)
#define COLOR_GREEN   0x00FF00  // RGB (0, 255, 0)
#define COLOR_YELLOW  0xFFFF00  // RGB (255, 255, 0)
#define COLOR_RED     0xFF0000  // RGB (255, 0, 0)
#define COLOR_OFF     0x000000  // RGB (0, 0, 0)

#define TREND_RISING_FAST 2        // °C/giây - nhiệt độ tăng nhanh
#define TREND_FALLING_FAST -2       // °C/giây - nhiệt độ giảm nhanh

typedef struct {
  float prevTemp;       // Nhiệt độ lần trước (°C)
  uint32_t prevTime;    // Thời gian lần trước (ms)
  float tempTrend;      // Xu hướng (°C/giây) = ΔT/Δt
} TrendData_t;

extern SemaphoreHandle_t xSemaphore_NeoPixelUpdate;  
extern QueueHandle_t xQueue_SensorData;  

void initNeoPixelSystem(void);

void taskHandleNeoPixel(void *pvParameters);

uint32_t getColorFromTemperature(float temp);

void getBlinkPatternFromHumidity(float humidity, float trend, uint16_t *onTime, uint16_t *offTime);

void updateNeoPixel(uint32_t color, uint16_t onTime, uint16_t offTime);

BaseType_t sendSensorDataToNeoPixel(const SensorData_t *sensorData);

void calculateTemperatureTrend(float currentTemp, uint32_t currentTimeMs, TrendData_t *trendData);

void initDHT20Sensor(void);

bool readDHT20Data(float *temperature, float *humidity);

void taskReadSensor(void *pvParameters);

#endif // TASK_2_LEDNEOPIXEL_H