
# use python f:/github/Arduino_Projects/cameraCapturingSendingMQTT/trial2/pythonReceiverTkinter.py

import base64
import json
import os
import shutil
import paho.mqtt.client as mqtt
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad

import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import threading
import io

# Global variable for the UI label
panel = None
info_label = None


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
    global image_count, panel, info_label
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
        
        # UPDATE UI: Process Image for Tkinter
        img = Image.open(io.BytesIO(decrypted_data))
        img = img.resize((320, 240)) # Resize for window
        img_tk = ImageTk.PhotoImage(img)

        panel.config(image=img_tk)
        panel.image = img_tk # Keep a reference!
        
    except json.JSONDecodeError:
        print("Error: Received message was not valid JSON.")
    except ValueError as ve:
        print(f"Decryption/Padding Error: {ve}. Check if KEY and IV match ESP32.")
    except Exception as e:
        print(f"Unexpected error: {e}")

#tkinter GUI
def setup_gui():
    global panel, info_label, root
    root = tk.Tk()
    root.title("ESP32-CAM AI Dashboard")
    root.geometry("400x500")

    # Label for AI Results
    info_label = tk.Label(root, text="Waiting for data...", font=("Arial", 14), fg="blue")
    info_label.pack(pady=10)

    # Placeholder for the Image
    panel = tk.Label(root, text="Image will appear here")
    panel.pack(pady=10)

    return root

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
    # Create the GUI
    root = setup_gui()

    # Start MQTT in a background thread
    mqtt_thread = threading.Thread(target=client.loop_forever)
    mqtt_thread.daemon = True # Closes thread when window is closed
    mqtt_thread.start()

    # Run the Tkinter main loop (this blocks the main script)
    root.mainloop()
except KeyboardInterrupt:
    print("Receiver stopped.")