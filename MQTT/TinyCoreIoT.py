import paho.mqtt.client as mqtt
import json
import time
import threading

# CoreIOT Config
COREIOT_HOST = 'app.coreiot.io'  
COREIOT_PORT = 1883  
ACCESS_TOKEN = 'rHa5SAQEanvnWzXnqDNi' 

# Local MQTT Broker (ESP32 data)
LOCAL_BROKER = '127.0.0.1'
LOCAL_PORT = 1883

# Clients
coreiot_client = None
local_client = None

def on_connect_coreiot(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to CoreIOT!")
    else:
        print("Failed to connect CoreIOT, code:", rc)

def on_connect_local(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to Local MQTT Broker!")
        client.subscribe("esp32/telemetry", qos=0)  # Subscribe ESP32 topic
    else:
        print("Failed to connect Local Broker, code:", rc)

def on_message_local(client, userdata, msg):
    """Terima data từ ESP32, relay lên CoreIOT"""
    try:
        payload = json.loads(msg.payload.decode('utf-8'))
        print(f"Received from ESP32: {payload}")
        
        # Format CoreIOT gateway telemetry
        device_id = payload.get('device_id', 'ESP32_001')
        telemetry = {
            device_id: [
                {
                    "ts": int(time.time() * 1000),
                    "values": {
                        "temperature": payload.get('temperature', 0),
                        "humidity": payload.get('humidity', 0),
                        "confidence": payload.get('confidence', 0),
                        "status": payload.get('status', 'UNKNOWN'),
                        "inference_time": payload.get('inference_time', 0),
                        "free_ram": payload.get('free_ram', 0)
                    }
                }
            ]
        }
        
        payload_str = json.dumps(telemetry)
        result = coreiot_client.publish('v1/gateway/telemetry', payload_str)
        print(f"Published to CoreIOT: {payload_str}")
        
    except Exception as e:
        print(f"Error processing message: {e}")

def on_publish(client, userdata, mid):
    print(f"Message {mid} published to CoreIOT")

def start_coreiot_client():
    global coreiot_client
    coreiot_client = mqtt.Client()
    coreiot_client.username_pw_set(ACCESS_TOKEN)
    coreiot_client.on_connect = on_connect_coreiot
    coreiot_client.on_publish = on_publish
    coreiot_client.connect(COREIOT_HOST, COREIOT_PORT, 60)
    coreiot_client.loop_start()

def start_local_client():
    global local_client
    local_client = mqtt.Client("LocalSubscriber")
    local_client.on_connect = on_connect_local
    local_client.on_message = on_message_local
    local_client.connect(LOCAL_BROKER, LOCAL_PORT, 60)
    local_client.loop_forever()

if __name__ == "__main__":
    print("Starting MQTT Relay: Local Broker → CoreIOT Cloud...")
    
    # Start CoreIOT connection
    start_coreiot_client()
    
    # Start Local Broker subscription (blocks)
    time.sleep(1)
    local_thread = threading.Thread(target=start_local_client, daemon=False)
    local_thread.start()
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nShutting down...")
        if local_client:
            local_client.loop_stop()
            local_client.disconnect()
        if coreiot_client:
            coreiot_client.loop_stop()
            coreiot_client.disconnect()