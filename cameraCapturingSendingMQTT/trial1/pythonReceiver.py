import base64
import io
from PIL import Image
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad
import paho.mqtt.client as mqtt

# --- CONFIGURATION (Must match ESP32) ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_TOPIC = "esp32/camera/images"
KEY = b'mysupersecretkey' # Must be 16 bytes
IV = b'1234567890123456'  # Must be 16 bytes

def on_message(client, userdata, msg):
    try:
        print("Image received! Decrypting...")
        
        # 1. Base64 Decode
        encrypted_data = base64.b64decode(msg.payload)
        
        # 2. AES Decrypt
        cipher = AES.new(KEY, AES.MODE_CBC, IV)
        decrypted_data = unpad(cipher.decrypt(encrypted_data), AES.block_size)
        
        # 3. Convert bytes to Image
        image = Image.open(io.BytesIO(decrypted_data))
        image.show() # This opens your default photo viewer
        
    except Exception as e:
        print(f"Failed to process image: {e}")

# --- MQTT SETUP ---
client = mqtt.Client()
client.on_message = on_message

# If using HiveMQ Cloud, you need these:
client.username_pw_set("your_username", "your_password")
client.tls_set() # Enable this for port 8883

client.connect(MQTT_BROKER, 8883)
client.subscribe(MQTT_TOPIC)

print("Waiting for images...")
client.loop_forever()