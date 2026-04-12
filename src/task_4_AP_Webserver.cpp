#include "task_4_AP_Webserver.h"

WiFiClient espClient;
PubSubClient client(espClient);

String mqttServer;
int mqttPort;
String mqttUser;
String mqttPass;
String mqttTopic;

String lastMsg = "-";

bool mqttConfigured = false;
unsigned long lastMQTTRetry = 0;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String msg;
  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  lastMsg = msg;

  Serial.print("MQTT msg: ");
  Serial.println(msg);
}

// ===== WiFi =====
const char *ap_ssid = "HAH-AP";
const char *ap_password = "12345678";

AsyncWebServer server(80);

void mountFlash(void *pvParameters)
{
  if (!LittleFS.begin(true))
  {
    Serial.println("[WebTask] LittleFS mount failed");
    vTaskDelete(NULL);
  }
  Serial.println("[WebTask] LittleFS mounted");
}

void settingsWifi(void *pvParameters)
{
  Serial.println("[WebTask] settingsWifi started");

  // Mode
  WiFi.mode(WIFI_AP_STA);

  // AP
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("[WebTask] AP IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("[WebTask] Connecting to STA");

  // Retry connect
  uint8_t retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20)
  {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n[WebTask] STA connected");
    Serial.print("[WebTask] STA IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\n[WebTask] STA FAILED (no Internet)");
  }
}

void webBackend(void *pvParameters)
{
  Serial.println("[WebTask] server started");

  // ===== API endpoints =====
  server.on("/api/on1", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              digitalWrite(LED_PIN, HIGH);
              if (client.connected()) {
              client.publish(mqttTopic.c_str(), "{\"led1\":\"ON\"}");
              }
              request->send(200, "text/plain", "ON"); });

  server.on("/api/off1", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              digitalWrite(LED_PIN, LOW);
              if (client.connected()) {
              client.publish(mqttTopic.c_str(), "{\"led1\":\"OFF\"}");
              }
              request->send(200, "text/plain", "OFF"); });

  server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {

    StaticJsonDocument<200> doc;
    deserializeJson(doc, data);

    String new_ssid = doc["ssid"];
    String new_pass = doc["password"];

    WiFi.disconnect(true);
    WiFi.begin(new_ssid.c_str(), new_pass.c_str());

    request->send(200, "application/json",
                  "{\"status\":\"connecting\"}"); });

  server.on("/api/wifi/status", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              StaticJsonDocument<300> doc;

              doc["connected"] = WiFi.status() == WL_CONNECTED;
              doc["ssid"] = WiFi.SSID();
              doc["ip"] = WiFi.localIP().toString();
              doc["rssi"] = WiFi.RSSI();

              // ===== MAC =====
              doc["mac"] = WiFi.macAddress();
              doc["mac_ap"] = WiFi.softAPmacAddress();

              // ===== UPTIME =====
              unsigned long ms = millis();
              unsigned long s = ms / 1000;
              unsigned long m = s / 60;
              unsigned long h = m / 60;
              unsigned long d = h / 24;

              char uptime[30];
              sprintf(uptime, "%lud %luh %lum", d, h % 24, m % 60);

              doc["uptime"] = uptime;

              String response;
              serializeJson(doc, response);

              request->send(200, "application/json", response);
            });

  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", "Rebooting...");
    delay(1000); // cho client nhận response
    ESP.restart(); });

  server.on("/api/mqtt/connect", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t)
            {

    DynamicJsonDocument doc(512);
    deserializeJson(doc, data);
    mqttServer = doc["server"].as<String>();
    mqttPort   = doc["port"].as<int>();
    mqttUser   = doc["username"].as<String>();
    mqttPass   = doc["password"].as<String>();
    mqttTopic  = doc["topic"].as<String>();

    client.disconnect();
    lastMsg = "-";
    client.setServer(mqttServer.c_str(), mqttPort);

    mqttConfigured = true; // 

    request->send(200, "application/json", "{\"ok\":true}"); });

  server.on("/api/mqtt/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    DynamicJsonDocument doc(256);

    doc["connected"] = client.connected();
    doc["lastMessage"] = lastMsg;

    String res;
    serializeJson(doc, res);

    request->send(200, "application/json", res); });

  
  // ===== Serve Web =====
  server.serveStatic("/", LittleFS, "/")
      .setDefaultFile("index.html");

  server.begin();
  Serial.println("[WebTask] Web server started");
}

void handleMQTT()
{

  if (WiFi.status() != WL_CONNECTED)
    return;
  if (!mqttConfigured)
    return;

  if (!client.connected())
  {

    if (millis() - lastMQTTRetry > 3000)
    {
      lastMQTTRetry = millis();

      Serial.println("Connecting MQTT...");

      if (client.connect("esp32-client", mqttUser.c_str(), mqttPass.c_str()))
      {
        Serial.println("MQTT Connected");

        client.subscribe(mqttTopic.c_str());
      }
      else
      {
        Serial.println("MQTT Failed");
      }
    }
  }
  else
  {
    client.loop();
  }
}
void webServerTask(void *pvParameters)
{
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  // ===== Mount LittleFS =====
  mountFlash(pvParameters);

  // ===== WiFi AP + STA =====
  settingsWifi(pvParameters);

  // ===== Web server =====
  webBackend(pvParameters);

  client.setCallback(mqttCallback);

  // ===== Task loop =====
  while (true)
  {
    handleMQTT();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
