#include "task_2_ledNeoPixel.h"

// ==================== Global Variables ====================
static Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
void initNeoPixelSystem(void) {
  pixel.begin();
  pixel.setBrightness(10);  
  pixel.clear();
  pixel.show();

  if (xSemaphore_NeoPixelUpdate == NULL) {
    xSemaphore_NeoPixelUpdate = xSemaphoreCreateBinary();
    if (xSemaphore_NeoPixelUpdate != NULL) {
      Serial.println("Semaphore created successfully");
    }
  }
  
  if (xQueue_SensorData == NULL) {
    xQueue_SensorData = xQueueCreate(QUEUE_SIZE, sizeof(SensorData_t));
    if (xQueue_SensorData != NULL) {
      Serial.println("Queue created successfully");
    }
  }
}

// ==================== Function: Get Color from Temperature ====================
uint32_t getColorFromTemperature(float temp) {
  uint32_t color;
  const char* colorName;
  
  if (temp < TEMP_BLUE_MAX) {
    color = COLOR_BLUE;
    colorName = "BLUE";
  } 
  else if (temp < TEMP_GREEN_MAX) {
    color = COLOR_GREEN;
    colorName = "GREEN";
  } 
  else if (temp < TEMP_YELLOW_MAX) {
    color = COLOR_YELLOW;
    colorName = "YELLOW";
  } 
  else {
    color = COLOR_RED;
    colorName = "RED";
  }
  
  Serial.printf("Color selected: %s\n", colorName);
  return color;
}

// ==================== Function: Get Blink Pattern from Humidity ====================
void getBlinkPatternFromHumidity(float humidity, float trend, uint16_t *onTime, uint16_t *offTime) {
  if (humidity < HUMIDITY_DRY_MAX) {
    // DRY MODE: < 45%
    if (trend > TREND_RISING_FAST) {
      *onTime = 100;
      *offTime = 100;
    } else {
      *onTime = BLINK_SLOW_ON;
      *offTime = BLINK_SLOW_OFF;
    }
  } 
  else if (humidity < HUMIDITY_NORMAL_MAX) {
    // NORMAL MODE: 45-70%
    if (trend < TREND_FALLING_FAST) {
      *onTime = 300;
      *offTime = 700;
    } else {
      *onTime = 65535;           
      *offTime = BLINK_STABLE_OFF;
    }
  } 
  else {
    // HIGH MODE: > 70%
    *onTime = 100;
    *offTime = 100;
  }
}

// ==================== Function: Calculate Temperature Trend ====================
void calculateTemperatureTrend(float currentTemp, uint32_t currentTimeMs, TrendData_t *trendData) {
  if (trendData->prevTime == 0) {
    trendData->prevTemp = currentTemp;
    trendData->prevTime = currentTimeMs;
    trendData->tempTrend = 0;
    return;
  }
  
  float timeDiffMs = (float)(currentTimeMs - trendData->prevTime);
  float timeDiffSec = timeDiffMs / 1000;
  
  if (timeDiffSec > 0.1) { 
    float tempDiff = currentTemp - trendData->prevTemp;
    trendData->tempTrend = tempDiff / timeDiffSec;
    Serial.printf("Trend: %.2f°C/s\n", trendData->tempTrend);
  }
  
  trendData->prevTemp = currentTemp;
  trendData->prevTime = currentTimeMs;
}

// ==================== Function: Update NeoPixel ====================
static uint32_t lastBlinkTime = 0;
static bool ledIsOn = true;

void updateNeoPixel(uint32_t color, uint16_t onTime, uint16_t offTime) {
  uint32_t currentTime = millis();
  
  if (offTime == 0) {
    pixel.setPixelColor(0, color);
    pixel.show();
    return;
  }
  
  uint32_t elapsedTime = currentTime - lastBlinkTime;
  
  if (ledIsOn) {
    if (elapsedTime >= onTime) {
      lastBlinkTime = currentTime;
      ledIsOn = false;
      
      if (offTime > 0) {
        pixel.clear();
        pixel.show();
      }
    }
  } else {
    if (elapsedTime >= offTime) {
      lastBlinkTime = currentTime;
      ledIsOn = true;
      pixel.setPixelColor(0, color);
      pixel.show();
    }
  }
}

// ==================== Function: Send Sensor Data to NeoPixel Task ====================
BaseType_t sendSensorDataToNeoPixel(const SensorData_t *sensorData) {
  if (xQueue_SensorData == NULL || sensorData == NULL) {
    return pdFALSE;
  }
  
  BaseType_t result = xQueueOverwrite(xQueue_SensorData, sensorData);
  
  if (xSemaphore_NeoPixelUpdate != NULL) {
    xSemaphoreGive(xSemaphore_NeoPixelUpdate);
  }
  
  return result;
}

// ==================== Main Task: Handle NeoPixel ====================
void taskHandleNeoPixel(void *pvParameters) {
  SensorData_t sensorData;
  uint32_t currentColor = COLOR_BLUE;
  uint16_t blinkOnTime = BLINK_SLOW_ON;
  uint16_t blinkOffTime = BLINK_SLOW_OFF;
  uint16_t prevBlinkOnTime = BLINK_SLOW_ON;   // Track previous pattern
  uint16_t prevBlinkOffTime = BLINK_SLOW_OFF;
  
  TrendData_t trendData = {0, 0, 0};
  uint32_t currentTimeMs = 0;
  
  Serial.println("NeoPixel task started");
  initNeoPixelSystem();
  Serial.println("Initial color: BLUE, blink pattern: SLOW");
  
  while (1) {
    if (xSemaphoreTake(xSemaphore_NeoPixelUpdate, 0) == pdTRUE) {
      if (xQueueReceive(xQueue_SensorData, &sensorData, 0) == pdPASS) {
        Serial.printf("Received - Temp: %.1f°C, Humidity: %.1f%%\n", 
                      sensorData.temperature, sensorData.humidity);
        
        currentTimeMs = xTaskGetTickCount();
        calculateTemperatureTrend(sensorData.temperature, currentTimeMs, &trendData);
        
        uint32_t newColor = getColorFromTemperature(sensorData.temperature);
        getBlinkPatternFromHumidity(sensorData.humidity, trendData.tempTrend, &blinkOnTime, &blinkOffTime);
        
        if (newColor != currentColor || blinkOnTime != prevBlinkOnTime || blinkOffTime != prevBlinkOffTime) {
          currentColor = newColor;
          prevBlinkOnTime = blinkOnTime;
          prevBlinkOffTime = blinkOffTime;
          lastBlinkTime = millis();
          ledIsOn = true;
        }
      }
    }
    
    updateNeoPixel(currentColor, blinkOnTime, blinkOffTime);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ==================== DHT20 Sensor Implementation ====================
DHT20 dht20;

void initDHT20Sensor(void) {
  Serial.println("\n[Sensor] Initializing DHT20...");
  Wire.begin(DHT20_I2C_SDA, DHT20_I2C_SCL);
  Wire.setClock(DHT20_I2C_FREQ);
  
  if (dht20.begin()) {
    Serial.println("DHT20 initialized successfully!");
  } else {
    Serial.println("ERROR: DHT20 initialization failed!");
  }
  delay(500);
}

bool readDHT20Data(float *temperature, float *humidity) {
  int status = dht20.read();
  
  if (status != 0) {
    Serial.printf("Read failed (Status: %d)\n", status);
    return false;
  }
  
  *temperature = dht20.getTemperature();
  *humidity = dht20.getHumidity();
  
  if (isnan(*temperature) || isnan(*humidity)) {
    Serial.println("Invalid data from DHT20 (NaN)");
    return false;
  }
  
  Serial.printf("Temp: %.1f°C, Humidity: %.1f%%\n", *temperature, *humidity);
  return true;
}

void taskReadSensor(void *pvParameters) {
  float temperature = 0;
  float humidity = 0;
  SensorData_t sensorData;
  uint8_t retryCount = 0;
  
  initDHT20Sensor();
  
  while (1) {
    retryCount = 0;
    while (retryCount < SENSOR_MAX_RETRY) {
      if (readDHT20Data(&temperature, &humidity)) {
        sensorData.temperature = temperature;
        sensorData.humidity = humidity;
        
        // Update global variables for TinyML task
        glob_temperature = temperature;
        glob_humidity = humidity;
        
        if (sendSensorDataToNeoPixel(&sensorData) == pdPASS) {
          Serial.println("Data sent to queue");
        } else {
          Serial.println("Queue full, data not sent");
        }
        
        retryCount = SENSOR_MAX_RETRY;
      } else {
        retryCount++;
        if (retryCount < SENSOR_MAX_RETRY) {
          Serial.printf("Retry %d/%d...\n", retryCount, SENSOR_MAX_RETRY);
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
  }
}

