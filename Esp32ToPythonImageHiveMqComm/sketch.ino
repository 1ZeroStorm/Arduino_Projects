#include "esp_camera.h" // esp 32 cam module
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi Settings - CHANGE THESE!
const char* ssid = "Aryahiro";
const char* password = "angga1407";

// MQTT Settings - USE HIVEMQ
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Topics
const char* topic_esp32_sends = "test/esp32_to_python";
const char* topic_python_sends = "test/python_to_esp32";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  
  // wifi connection
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

}

void callback(char* topic, byte* message, unsigned int length) {
  // Convert message to string
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  
  Serial.print("📨 FROM PYTHON: ");
  Serial.println(messageTemp);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to HiveMQ... ");
    
    // Create unique client ID
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("✅ Connected to HiveMQ!");
      
      // Subscribe to Python's messages
      client.subscribe(topic_python_sends);
      Serial.print("Subscribed to: ");
      Serial.println(topic_python_sends);
      
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 5s...");
      delay(5000);
    }
  }
}

void setup() {
  // set wifi and hivemq server
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Initialize camera (same config as before)
  camera_config_t config;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  if (esp_camera_init(&config) != ESP_OK) { 
    Serial.println("Camera init failed"); 
    return; 
  }

  // catching pythons message starts here
  client.setCallback(callback); 
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Capture frame 
  camera_fb_t * fb = esp_camera_fb_get(); 
  if (!fb) { 
    Serial.println("Camera capture failed"); 
    return; 
  } 
  
  // Publish image buffer to MQTT 
  if (client.connected()) { 
    Serial.println("Publishing image to MQTT..."); 
    client.publish(mqtt_topic, fb->buf, fb->len); // raw JPEG bytes 
  } else {
    Serial.println("MQTT not connected"); 
  }
}