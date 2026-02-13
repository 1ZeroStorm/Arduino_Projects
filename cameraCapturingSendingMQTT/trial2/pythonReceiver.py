import base64
import json
import os
import shutil
import paho.mqtt.client as mqtt
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad

# --- CONFIGURATION ---
MQTT_BROKER = "broker.hivemq.com"
# Make sure this matches TOPIC_CLASSIFICATION in your Arduino code!
MQTT_TOPIC = "esp32/cam/classification" 
KEY = b'mysupersecretkey' 
IV = b'1234567890123456'  
SAVE_FOLDER = "received_images"

# --- FOLDER SETUP ---
if os.path.exists(SAVE_FOLDER):
    shutil.rmtree(SAVE_FOLDER)
os.makedirs(SAVE_FOLDER)
print(f"Directory '{SAVE_FOLDER}' is ready.")

image_count = 0

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Connection failed with code {rc}")

def on_message(client, userdata, msg):
    global image_count
    try:
        # 1. Parse the JSON wrapper
        payload = json.loads(msg.payload.decode('utf-8'))
        
        # Get data from JSON
        label = payload.get("label", "unknown")
        confidence = payload.get("confidence", 0)
        base64_str = payload.get("image_aes", "")

        if not base64_str:
            print("Received JSON but 'image_aes' field is empty.")
            return

        # 2. Base64 Decode
        encrypted_data = base64.b64decode(base64_str)
        
        # 3. AES Decrypt (CBC Mode)
        cipher = AES.new(KEY, AES.MODE_CBC, IV)
        # Decrypt and remove PKCS7 padding
        decrypted_data = unpad(cipher.decrypt(encrypted_data), AES.block_size)
        
        # 4. Save to Folder
        image_count += 1
        filename = f"{SAVE_FOLDER}/img_{image_count}_{label}.jpg"
        
        with open(filename, "wb") as f:
            f.write(decrypted_data)
            
        print(f"[{image_count}] Detected: {label} ({confidence*100:.1f}%) -> Saved to {filename}")
        
    except json.JSONDecodeError:
        print("Error: Received message was not valid JSON.")
    except ValueError as ve:
        print(f"Decryption/Padding Error: {ve}. Check if KEY and IV match ESP32.")
    except Exception as e:
        print(f"Unexpected error: {e}")

# --- MQTT SETUP ---
# Callback for newer paho-mqtt versions might need CallbackAPIVersion
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Note: HiveMQ Public doesn't strictly require username/pw on 1883
# If you didn't set them up in Arduino, you don't need them here.
# client.username_pw_set("Aryahiro", "angga1407")

print(f"Connecting to {MQTT_BROKER}...")
client.connect(MQTT_BROKER, 1883, 60)

# Start the loop
try:
    client.loop_forever()
except KeyboardInterrupt:
    print("Receiver stopped.")