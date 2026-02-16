/* ESP32-CAM Edge Impulse Classification + MQTT Only */
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include "bottle_and_clipbox_recognition_inferencing.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"

#include "base64.h"
#include "mbedtls/aes.h"

// AES KEYS
unsigned char key[16] = {'m','y','s','u','p','e','r','s','e','c','r','e','t','k','e','y'};
unsigned char iv[16]  = {'1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6'};

/* Camera Configuration */
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

/* WiFi Configuration */
const char* WIFI_SSID = "Aryahiro";      // CHANGE THIS
const char* WIFI_PASSWORD = "angga1407";  // CHANGE THIS

/* MQTT Configuration */
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* TOPIC_CLASSIFICATION = "esp32/cam/classification";
const char* TOPIC_STATUS = "esp32/cam/status";

/* Camera Settings */
#define FRAME_SIZE FRAMESIZE_QVGA  // 320x240
#define JPEG_QUALITY 10
#define INFERENCE_INTERVAL 2000    // Run inference every 2 seconds
#define PUBLISH_INTERVAL 1000      // Publish status every 1 second (changed from 100ms)

/* Global Variables */
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
uint8_t* inferenceBuffer = NULL;
unsigned long lastInferenceTime = 0;
unsigned long lastStatusTime = 0;
String clientID;

/* Function Declarations (Forward Declarations) */
String generateClientID();
bool initCamera();
void connectWiFi();
void connectMQTT();
void sendStatusMessage(String status, String ip = "");
void runInference();
void publishStatus();
void processMQTTCommands();

/* Generate unique client ID from MAC address */
String generateClientID() {
  uint64_t chipid = ESP.getEfuseMac();
  char clientID[20];
  snprintf(clientID, sizeof(clientID), "esp32-%08X", (uint32_t)chipid);
  return String(clientID);
}

// image encryption
String encryptImage(camera_fb_t * fb) {
    // 1. AES works in 16-byte blocks. We must calculate the padded size.
    // If image is 10,001 bytes, we need to make it 10,016.
    size_t original_len = fb->len;
    size_t padded_len = ((original_len / 16) + 1) * 16;
    unsigned char * padded_buf = (unsigned char *)malloc(padded_len);
    
    // Fill the buffer with 'padding' (Standard PKCS7 style)
    uint8_t padding_value = padded_len - original_len;
    memset(padded_buf, padding_value, padded_len);
    memcpy(padded_buf, fb->buf, original_len);

    // 2. Prepare the Output Buffer
    unsigned char * encrypted_output = (unsigned char *)malloc(padded_len);

    // 3. Run the AES-CBC Scrambler
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128); // 128-bit = 16 byte key
    
    // We need a local copy of IV because mbedtls modifies it during encryption
    unsigned char local_iv[16];
    memcpy(local_iv, iv, 16);

    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, local_iv, padded_buf, encrypted_output);

    // 4. Base64 encode the SCRAMBLED data
    String base64Result = base64::encode(encrypted_output, padded_len);

    // 5. Cleanup memory (Very important!)
    free(padded_buf);
    free(encrypted_output);
    mbedtls_aes_free(&aes);

    return base64Result;
}


/* Initialize Camera */
bool initCamera() {
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
  config.frame_size = FRAMESIZE_QVGA; // QVGA framesize (320 x 240) to match EI input
  
  if(psramFound()) {
    Serial.printf("PSRAM found, using higher quality settings\n");
    
    config.jpeg_quality = JPEG_QUALITY;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    Serial.printf("PSRAM not found, using lower quality settings\n");
   
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // getting the lastest picture from the camera
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err); // returns specific error, look up on esp 32 manual
    return false;
  }
  
  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  
  return true;
}

/* Connect to WiFi */
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

/* Send status message */
void sendStatusMessage(String status, String ip) {
  StaticJsonDocument<256> doc; // 256 bytes of RAM
  doc["client_id"] = clientID;
  doc["status"] = status;
  doc["ip"] = ip;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  if (mqttClient.connected()) {
    mqttClient.publish(TOPIC_STATUS, jsonString.c_str()); // publising the status 
  }
}

/* Connect to MQTT (MQTT LOGIN PROCESS) */
void connectMQTT() {
  Serial.print("Connecting to MQTT broker...");
  
  clientID = generateClientID(); // create a unique name for this specific board (e.g., esp32-D1A5)
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  
  mqttClient.setBufferSize(30000);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 10) {
    if (mqttClient.connect(clientID.c_str())) {
      Serial.println("connected!");
      
      // Send connection status
      //sendStatusMessage("connected", WiFi.localIP().toString());
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying...");
      delay(2000);
      attempts++;
    }
  }
}

/* Process MQTT commands */
void processMQTTCommands() {
  // Example: You can add command processing here
  // if you want to control the ESP32 via MQTT
}

/* Run Edge Impulse Inference */
void runInference() {

  // blocks the whole code if the pause is not reached (2 seconds)
  unsigned long now = millis();
  if (now - lastInferenceTime < INFERENCE_INTERVAL) {
    return;
  }
  lastInferenceTime = now;
  
  // Allocate buffer if needed
  if (!inferenceBuffer) {
    inferenceBuffer = (uint8_t*)malloc(320 * 240 * 3);  // QVGA RGB - matching the frame size in camera config, the '3' is like [255, 0, 0]
    if (!inferenceBuffer) {
      Serial.println("Failed to allocate inference buffer");
      return;
    }
  }
  
  // Capture frame for inference
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  String encryptedString = encryptImage(fb);

  // Convert JPEG to RGB888
  if (!fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, inferenceBuffer)) {
    Serial.println("JPEG to RGB conversion failed");
    esp_camera_fb_return(fb);
    return;
  }
  esp_camera_fb_return(fb);
  
  // Prepare signal for Edge Impulse
  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = [](size_t offset, size_t length, float* out_ptr) -> int {
    size_t pixel_ix = offset * 3;
    size_t out_ptr_ix = 0;
    
    while (length--) {
      uint8_t r = inferenceBuffer[pixel_ix];
      uint8_t g = inferenceBuffer[pixel_ix + 1];
      uint8_t b = inferenceBuffer[pixel_ix + 2];
      
      out_ptr[out_ptr_ix++] = (r << 16) + (g << 8) + b;
      pixel_ix += 3;
    }
    return 0;
  };
  
  // Run inference
  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  
  if (err != EI_IMPULSE_OK) {
    Serial.printf("Inference failed: %d\n", err);
    return;
  }
  
  // Find best classification
  float bestConfidence = 0;
  int bestIndex = 0;
  String bestLabel = "unknown";
  
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) { // EI_CLASSIFIER_LABEL_COUNT: (ex) i=0 represents clipbox, i=1 represents waterbottle 
    if (result.classification[i].value > bestConfidence) {
      bestConfidence = result.classification[i].value;
      bestIndex = i;
      bestLabel = String(ei_classifier_inferencing_categories[i]);
    }
  }
  
  // Create JSON message
  DynamicJsonDocument doc(25000); 
  doc["client_id"] = clientID;
  doc["timestamp"] = now;
  doc["label"] = bestLabel;
  doc["confidence"] = bestConfidence;
  doc["image_aes"] = encryptedString;

  JsonArray probabilities = doc.createNestedArray("probabilities");
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    JsonObject prob = probabilities.createNestedObject();
    prob["label"] = String(ei_classifier_inferencing_categories[i]);
    prob["value"] = result.classification[i].value;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send via MQTT
  if (mqttClient.connected()) {
    if (mqttClient.publish(TOPIC_CLASSIFICATION, jsonString.c_str())) {
      Serial.printf("Published: %s (%.1f%%)\n", bestLabel.c_str(), bestConfidence * 100);
    } else {
      Serial.println("MQTT publish failed, try maybe the camera resolution VGA is too big"); 
    }
  }
  
  // Also print to Serial for debugging
  Serial.print("RESULT:");
  Serial.println(jsonString);
}

/* Publish periodic status */
void publishStatus() {
  unsigned long now = millis();
  
  if (now - lastStatusTime < PUBLISH_INTERVAL) {
    return;
  }
  lastStatusTime = now;
  
  if (mqttClient.connected()) {
    // Send heartbeat status
    sendStatusMessage("online", WiFi.localIP().toString());
  }
}

/* Setup */
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("ESP32-CAM Classification System");
  Serial.println("=================================");
  
  // Initialize camera
  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    while(1);
  }
  Serial.println("Camera initialized");
  
  // Connect to WiFi
  connectWiFi();
  
  // Connect to MQTT
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
  }
  
  // Setup LED flash
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);  // Turn off flash initially
  
  // Blink LED to indicate ready
  for(int i = 0; i < 3; i++) {
    digitalWrite(4, HIGH);
    delay(200);
    digitalWrite(4, LOW);
    delay(200);
  }
  digitalWrite(4, HIGH);
  
  Serial.println("\nSystem ready!");
  Serial.println("Starting classification loop...");
  Serial.println("=================================");
}

/* Main Loop */
void loop() {
  // Maintain MQTT connection
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
    
    // Process any incoming MQTT commands
    //processMQTTCommands();
    
    // Publish periodic status
    //publishStatus();
    
  } else {
    // Try to reconnect WiFi
    static unsigned long lastWiFiReconnect = 0;
    if (millis() - lastWiFiReconnect > 10000) {
      lastWiFiReconnect = millis();
      Serial.println("Reconnecting WiFi...");
      connectWiFi();
    }
  }
  
  // Run inference
  runInference();
  
  // Small delay to prevent watchdog issues
  delay(10);
}