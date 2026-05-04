import paho.mqtt.client as mqtt
from paho.mqtt.client import CallbackAPIVersion
import json
import time

# ================= CONFIG =================
LOCAL_BROKER = "10.229.165.178"
LOCAL_PORT = 1883

THINGSBOARD_HOST = "app.coreiot.io"
THINGSBOARD_PORT = 1883
ACCESS_TOKEN = "5fBB3h4jv2s2k16jVcyR"

# ================= STATE =================
device_data = {}

coreiot_connected = False
last_send = 0
SEND_INTERVAL = 5

USE_SAMPLE = True   # fake ESP32 mode

# ================= SAMPLE DATA =================
def get_sample_data():
    now = int(time.time() * 1000)
    return {
        "ESP32_001": [
            {"ts": now, "values": {"temperature": 22.5, "humidity": 55}}
        ],
        "ESP32_002": [
            {"ts": now, "values": {"temperature": 30.5, "humidity": 80}}
        ],
        "ESP32_003": [
            {"ts": now, "values": {"temperature": 10.5, "humidity": 20}}
        ]
    }

# ================= LOCAL MQTT =================
def on_local_connect(client, userdata, flags, reason_code, properties=None):
    print("[LOCAL] Connected:", reason_code)

    # subscribe custom ESP32 topics
    client.subscribe("iot/+/telemetry")

def on_local_message(client, userdata, msg):
    try:
        topic = msg.topic
        if not topic.startswith("iot/"):
            return

        parts = topic.split("/")
        if len(parts) < 3:
            return

        device_id = parts[1]
        payload = json.loads(msg.payload.decode())

        device_data[device_id] = {
            "ts": int(time.time() * 1000),
            "values": payload
        }

        print("[LOCAL RX]", device_id, payload)

    except Exception as e:
        print("parse error:", e)

# ================= COREIOT MQTT =================
def on_coreiot_connect(client, userdata, flags, reason_code, properties=None):
    global coreiot_connected

    if reason_code == 0:
        print("[COREIOT] Connected!")
        coreiot_connected = True

        client.subscribe("v1/devices/me/rpc/request/+")
        print("[COREIOT] RPC subscribed")

    else:
        print("[COREIOT] Failed:", reason_code)

def on_coreiot_publish(client, userdata, mid, reason_code=None, properties=None):
    print("[COREIOT] Published mid =", mid)

def on_coreiot_message(client, userdata, msg):
    print("[RPC]", msg.topic, msg.payload.decode())

# ================= CLIENT =================
local_client = mqtt.Client(
    client_id="local_sub",
    callback_api_version=CallbackAPIVersion.VERSION2
)

local_client.on_connect = on_local_connect
local_client.on_message = on_local_message
local_client.connect(LOCAL_BROKER, LOCAL_PORT)
local_client.loop_start()

coreiot_client = mqtt.Client(
    client_id="coreiot_gateway",
    callback_api_version=CallbackAPIVersion.VERSION2
)

coreiot_client.username_pw_set(ACCESS_TOKEN)
coreiot_client.on_connect = on_coreiot_connect
coreiot_client.on_publish = on_coreiot_publish
coreiot_client.on_message = on_coreiot_message

coreiot_client.connect(THINGSBOARD_HOST, THINGSBOARD_PORT, 60)
coreiot_client.loop_start()

# ================= MAIN LOOP =================
try:
    while True:
        now = time.time()

        telemetry = {}

        # ===== 1. REAL ESP32 DATA =====
        for device_id, data in device_data.items():
            telemetry[device_id] = [
                {
                    "ts": data["ts"],
                    "values": data["values"]
                }
            ]

        # ===== 2. FAKE DATA =====
        if USE_SAMPLE:
            sample = get_sample_data()
            telemetry.update(sample)

        # ===== 3. SEND GATEWAY =====
        if coreiot_connected and (now - last_send >= SEND_INTERVAL):
            last_send = now

            payload = json.dumps(telemetry)

            print("\n[SEND TO COREIOT - GATEWAY]")
            print(payload)

            coreiot_client.publish(
                "v1/gateway/telemetry",
                payload,
                qos=1
            )

        time.sleep(0.2)

except KeyboardInterrupt:
    print("Stopped")

finally:
    local_client.loop_stop()
    coreiot_client.loop_stop()
    local_client.disconnect()
    coreiot_client.disconnect()