import paho.mqtt.client as mqtt
import time

# HiveMQ Broker Settings
BROKER = "broker.hivemq.com"
PORT = 1883

# Topics - MUST MATCH ESP32 CODE!
TOPIC_ESP32_SENDS = "test/esp32_to_python"
TOPIC_PYTHON_SENDS = "test/python_to_esp32"

def on_connect(client, userdata, flags, rc):
    """Called when connected to broker"""
    if rc == 0:
        print("✅ Connected to HiveMQ broker!")
        
        # Subscribe to ESP32's messages
        client.subscribe(TOPIC_ESP32_SENDS)
        print(f"Subscribed to: {TOPIC_ESP32_SENDS}")
        
        # Send welcome message
        client.publish(TOPIC_PYTHON_SENDS, "Python is ready!")
        print("📤 Sent: Python is ready!")
        
    else:
        print(f"❌ Connection failed with code {rc}")

def on_message(client, userdata, msg):
    """Called when a message is received"""
    message = msg.payload.decode()
    print(f"📨 FROM ESP32: {message}")
    
    # Respond to ESP32
    response = "Hello again ESP32!"
    client.publish(TOPIC_PYTHON_SENDS, response)
    print(f"📤 TO ESP32: {response}")

# Create MQTT client
print("Starting Python MQTT Client...")
client = mqtt.Client()

# Set up callbacks
client.on_connect = on_connect
client.on_message = on_message

# Connect to HiveMQ
print(f"Connecting to {BROKER}:{PORT}...")
try:
    client.connect(BROKER, PORT, 60)
except Exception as e:
    print(f"❌ Connection error: {e}")
    print("Make sure you're connected to the internet!")
    exit(1)

# Start the loop
client.loop_start()

print("\n" + "="*50)
print("ESP32 ↔ Python Communication via HiveMQ")
print("="*50)
print("ESP32 sends: 'Hello World from ESP32!'")
print("Python responds: 'Hello again ESP32!'")
print("\n👂 Waiting for ESP32 messages...")
print("Press Ctrl+C to exit\n")

# Keep the program running
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\n\nDisconnecting...")
    client.loop_stop()
    client.disconnect()
    print("Goodbye!")
