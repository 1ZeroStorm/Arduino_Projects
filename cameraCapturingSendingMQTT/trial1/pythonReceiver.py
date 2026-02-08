# run using python f:/github/Arduino_Projects/cameraCapturingSendingMQTT/trial1/pythonReceiver.py

import base64
import io
import os
import shutil
from PIL import Image
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad
import paho.mqtt.client as mqtt

# --- CONFIGURATION ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_TOPIC = "esp32/camera/images"
KEY = b'mysupersecretkey' 
IV = b'1234567890123456'  
SAVE_FOLDER = "trial1/received_images"

# --- 1. CLEANUP/FOLDER SETUP ---
if os.path.exists(SAVE_FOLDER):
    print(f"Cleaning up old folder: {SAVE_FOLDER}")
    shutil.rmtree(SAVE_FOLDER)  # Deletes the folder and everything inside

os.makedirs(SAVE_FOLDER) # Creates a fresh, empty folder
print(f"Folder '{SAVE_FOLDER}' is ready.")

image_count = 0

def on_message(client, userdata, msg):
    global image_count
    try:
        # 1. Base64 Decode
        encrypted_data = base64.b64decode(msg.payload)
        
        # 2. AES Decrypt
        cipher = AES.new(KEY, AES.MODE_CBC, IV)
        decrypted_data = unpad(cipher.decrypt(encrypted_data), AES.block_size)
        
        # 3. Save to Folder
        image_count += 1
        filename = f"{SAVE_FOLDER}/capture_{image_count}.jpg"
        
        with open(filename, "wb") as f:
            f.write(decrypted_data)
            
        print(f"Success! Saved: {filename}")
        
    except Exception as e:
        print(f"Failed to process image: {e}")

# --- MQTT SETUP ---
client = mqtt.Client()
client.on_message = on_message
client.username_pw_set("Aryahiro", "angga1407")
client.tls_set() 

print("Connecting to HiveMQ...")
client.connect(MQTT_BROKER, 8883)
client.subscribe(MQTT_TOPIC)

client.loop_forever()