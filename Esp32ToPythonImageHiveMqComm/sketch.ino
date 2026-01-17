#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi Settings
const char* ssid = "Aryahiro";
const char* password = "angga1407";

// MQTT Settings
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Topics
const char* topic_esp32_sends = "test/esp32_to_python";
const char* topic_python_sends = "test/python_to_esp32";

// Camera pins for ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_PIN            4  // Flash LED on GPIO4

WiFiClient espClient;
PubSubClient client(espClient);

// Base64 encoding function
String base64_encode(uint8_t* data, size_t length) {
  const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String encoded_string = "";
  int i = 0;
  int j = 0;
  uint8_t char_array_3[3];
  uint8_t char_array_4[4];
  
  while (length--) {
    char_array_3[i++] = *(data++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
      
      for(i = 0; i < 4; i++)
        encoded_string += base64_chars[char_array_4[i]];
      i = 0;
    }
  }
  
  if (i) {
    for(j = i; j < 3; j++)
      char_array_3[j] = 0;
    
    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;
    
    for(j = 0; j < i + 1; j++)
      encoded_string += base64_chars[char_array_4[j]];
    
    while(i++ < 3)
      encoded_string += '=';
  }
  
  return encoded_string;
}

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    ESP.restart();
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  
  Serial.print("📨 FROM PYTHON [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(messageTemp);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to HiveMQ... ");
    
    String clientId = "ESP32-CAM-" + String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("✅ Connected to HiveMQ!");
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
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n\nESP32-CAM MQTT Image Sender");
  
  // Set LED pin high right from the start
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON (GPIO4)");
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Initialize camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Use smaller image size for faster transmission
  config.frame_size = FRAMESIZE_QQVGA;  // 160x120 - smaller for testing
  config.jpeg_quality = 10;            // Lower = better quality (1-63)
  config.fb_count = 1;                 // Single buffer
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
    return;
  }
  
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    Serial.println("Camera configured with 180° rotation");
  }
  
  Serial.println("Setup complete!");
  delay(2000);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  static unsigned long lastCaptureTime = 0;
  unsigned long currentTime = millis();
  
  // Capture image every 5 seconds
  if (currentTime - lastCaptureTime > 5000) {
    lastCaptureTime = currentTime;
    
    Serial.println("\n📸 Capturing image...");
    
    // Capture image
    camera_fb_t *fb = esp_camera_fb_get();
    
    if (!fb) {
      Serial.println("❌ Camera capture failed");
      return;
    }
    
    Serial.printf("✅ Captured %d bytes JPEG image\n", fb->len);
    
    if (client.connected()) {
      Serial.println("📤 Encoding image to base64...");
      
      // Encode to base64
      String base64Image = base64_encode(fb->buf, fb->len);
      
      // Calculate chunks (max 1000 chars per chunk for MQTT safety)
      int chunkSize = 1000;
      int totalChunks = (base64Image.length() + chunkSize - 1) / chunkSize;
      
      Serial.printf("Base64 length: %d, Chunks: %d\n", base64Image.length(), totalChunks);
      
      // Send start marker
      String startMsg = "IMG_START:" + String(fb->len) + ":" + String(totalChunks);
      client.publish(topic_esp32_sends, startMsg.c_str());
      delay(10);
      
      // Send chunks
      for (int i = 0; i < totalChunks; i++) {
        int startIdx = i * chunkSize;
        int endIdx = min((i + 1) * chunkSize, base64Image.length());
        String chunk = base64Image.substring(startIdx, endIdx);
        String chunkMsg = "IMG_CHUNK:" + String(i) + ":" + chunk;
        
        if (client.publish(topic_esp32_sends, chunkMsg.c_str())) {
          Serial.printf("Sent chunk %d/%d (%d bytes)\n", i + 1, totalChunks, chunk.length());
        } else {
          Serial.println("Failed to send chunk!");
        }
        
        client.loop();
        delay(5);  // Small delay between chunks
      }
      
      // Send end marker
      client.publish(topic_esp32_sends, "IMG_END");
      Serial.println("✅ Image sent successfully!");
      
    } else {
      Serial.println("❌ MQTT not connected");
    }
    
    // Return the frame buffer
    esp_camera_fb_return(fb);
    
    Serial.println("⏳ Waiting for next capture...");
  }
  
  delay(100);
}