#include "task_8_rpc.h"

#define LED 48

// WIFI
const char *ssid = "realme";
const char *password = "hieu1234";

// MQTT
const char *mqtt_server = "app.coreiot.io";
const int port = 1883;

// TOKEN
const char *access_token_rpc = "ikqYjfskbtMlsietR9lD";
const char *access_token_sensor = "EDHpzo1yGIab2tkLfuye"; // 🔥 đổi token sensor

// MQTT CLIENTS
WiFiClient espClient1;
PubSubClient clientRPC(espClient1);

WiFiClient espClient2;
PubSubClient clientSensor(espClient2);

// DATA
bool value = false;

// SENSOR
DHT20 DHT(&Wire);

// ================= CALLBACK RPC =================
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received: ");

  char msg[256];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.println(msg);

DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, msg);

  if (err) return;

  if (!doc["method"].is<const char*>()) return;

  const char *method = doc["method"];

  if (strcmp(method, "POWER") == 0)
  {
    const char *param = doc["params"];

    if (strcmp(param, "ON") == 0)
      value = true;
    else if (strcmp(param, "OFF") == 0)
      value = false;

    digitalWrite(LED, value ? HIGH : LOW);

    Serial.print("LED: ");
    Serial.println(value);

    // gửi attribute
    DynamicJsonDocument res(1024);
    res["value"] = value;

    char buffer[100];
    serializeJson(res, buffer);

    clientRPC.publish("v1/devices/me/attributes", buffer);
  }
}


// ================= RECONNECT RPC =================
void reconnectRPC()
{
  while (!clientRPC.connected())
  {
    Serial.print("Connecting RPC...");

    if (clientRPC.connect("ESP32_RPC", access_token_rpc, NULL))
    {
      Serial.println("connected");
      clientRPC.subscribe("v1/devices/me/rpc/request/+");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(clientRPC.state());
      delay(2000);
    }
  }
}


// ================= RECONNECT SENSOR =================
void reconnectSensor()
{
  while (!clientSensor.connected())
  {
    Serial.print("Connecting Sensor...");

    if (clientSensor.connect("ESP32_SENSOR", access_token_sensor, NULL))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(clientSensor.state());
      delay(2000);
    }
  }
}


// ================= SETUP =================
void setupRPC()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  Wire.begin(11,12);
  DHT.begin();

  // setup MQTT
  clientRPC.setServer(mqtt_server, port);
  clientRPC.setCallback(callback);

  clientSensor.setServer(mqtt_server, port);
}


// ================= TASK RPC =================
void taskRPC(void *pvParameters)
{
  while (1)
  {
    if (!clientRPC.connected())
    {
      reconnectRPC();
    }

    clientRPC.loop();

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


void taskSensor(void *pvParameters)
{
  while (1)
  {
    if (!clientSensor.connected())
      reconnectSensor();

    clientSensor.loop();

    if (DHT.read() != 0)   // 👈 check status
    {
      Serial.println("DHT read error!");
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }

    float temp = DHT.getTemperature();
    float hum  = DHT.getHumidity();

    if (!isnan(temp) && !isnan(hum))
    {
      DynamicJsonDocument doc(1024);
      doc["temperature"] = temp;
      doc["humidity"] = hum;

      char buffer[128];
      serializeJson(doc, buffer);

      clientSensor.publish("v1/devices/me/telemetry", buffer);

      Serial.printf("Temp: %.2f | Hum: %.2f\n", temp, hum);
    }
    else
    {
      Serial.println("NaN data!");
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}