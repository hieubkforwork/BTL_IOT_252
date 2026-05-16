#include "global.h"

SemaphoreHandle_t xSemaphore_NeoPixelUpdate = NULL;
QueueHandle_t xQueue_SensorData = NULL;
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;

float glob_temperature = 25.0;
float glob_humidity = 55.0;

WiFiClient espClient;
PubSubClient client(espClient);
