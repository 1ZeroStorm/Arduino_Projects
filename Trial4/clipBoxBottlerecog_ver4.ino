/* ESP32-CAM Edge Impulse Classification + Video Streaming + MQTT */
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include "bottle_and_clipbox_recognition_inferencing.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"

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
#define INFERENCE_INTERVAL 2000  // Run inference every 2 seconds

/* Global Variables */
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
WebServer server(80);
uint8_t* inferenceBuffer = NULL;
unsigned long lastInferenceTime = 0;
String clientID;
bool streamEnabled = true;
uint32_t streamDelay = 100; // ms between frames

/* Generate unique client ID from MAC address */
String generateClientID() {
  uint64_t chipid = ESP.getEfuseMac();
  char clientID[20];
  snprintf(clientID, sizeof(clientID), "esp32-%08X", (uint32_t)chipid);
  return String(clientID);
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
  
  if(psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = JPEG_QUALITY;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
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
    Serial.print("Stream URL: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/stream");
    Serial.print("Single JPEG: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/jpg");
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

/* Connect to MQTT */
void connectMQTT() {
  Serial.print("Connecting to MQTT broker...");
  
  clientID = generateClientID();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 10) {
    if (mqttClient.connect(clientID.c_str())) {
      Serial.println("connected!");
      
      // Send connection status
      String statusMsg = "{\"status\":\"connected\",\"client_id\":\"" + clientID + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
      mqttClient.publish(TOPIC_STATUS, statusMsg.c_str());
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying...");
      delay(2000);
      attempts++;
    }
  }
}

/* Handle root page */
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Stream</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            text-align: center; 
            background: #f0f0f0;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 { color: #333; }
        .video-container {
            margin: 20px 0;
            border: 2px solid #ddd;
            border-radius: 5px;
            overflow: hidden;
        }
        img { 
            width: 100%; 
            max-width: 640px;
            height: auto;
        }
        .controls {
            margin: 20px 0;
        }
        button {
            background: #4CAF50;
            color: white;
            border: none;
            padding: 10px 20px;
            margin: 5px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover { background: #45a049; }
        #status {
            margin: 20px 0;
            padding: 10px;
            border-radius: 5px;
            background: #e7f3fe;
        }
        .classification {
            background: #f8f9fa;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
            text-align: left;
        }
        .classification h3 { margin-top: 0; }
        .result { font-size: 18px; font-weight: bold; }
        .confidence { color: #4CAF50; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-CAM Live Stream with AI Classification</h1>
        
        <div id="status">
            <p>Streaming from ESP32-CAM</p>
            <p>IP: )rawliteral" + WiFi.localIP().toString() + R"rawliteral(</p>
            <p>Client ID: )rawliteral" + clientID + R"rawliteral(</p>
        </div>
        
        <div class="video-container">
            <img src="/stream" id="stream" alt="Video Stream">
        </div>
        
        <div class="classification" id="classification">
            <h3>AI Classification Results</h3>
            <p id="result">Waiting for classification...</p>
            <p id="confidence">Confidence: 0%</p>
            <p id="timestamp">Last update: Never</p>
        </div>
        
        <div class="controls">
            <button onclick="location.reload()">Refresh Page</button>
            <button onclick="window.open('/jpg', '_blank')">Capture Still Image</button>
            <button onclick="toggleStream()">Toggle Stream</button>
        </div>
        
        <div>
            <p>Connection: <span id="connStatus">Connected</span></p>
            <p>Frame Rate: <input type="range" id="fps" min="1" max="30" value="10" onchange="changeFPS()"> <span id="fpsValue">10</span> FPS</p>
        </div>
    </div>

    <script>
        let streamEnabled = true;
        let currentFPS = 10;
        
        function toggleStream() {
            streamEnabled = !streamEnabled;
            const img = document.getElementById('stream');
            const btn = event.target;
            
            if (streamEnabled) {
                img.src = "/stream?fps=" + currentFPS;
                btn.textContent = "Stop Stream";
                document.getElementById('connStatus').textContent = "Streaming";
            } else {
                img.src = "";
                btn.textContent = "Start Stream";
                document.getElementById('connStatus').textContent = "Paused";
            }
        }
        
        function changeFPS() {
            const slider = document.getElementById('fps');
            const fpsValue = document.getElementById('fpsValue');
            currentFPS = slider.value;
            fpsValue.textContent = currentFPS;
            
            if (streamEnabled) {
                document.getElementById('stream').src = "/stream?fps=" + currentFPS;
            }
        }
        
        // MQTT over WebSocket for classification updates
        const mqttClient = new Paho.MQTT.Client("broker.hivemq.com", 8000, "webclient_" + Math.random().toString(36).substr(2, 9));
        
        mqttClient.onConnectionLost = (responseObject) => {
            console.log("Connection lost: " + responseObject.errorMessage);
        };
        
        mqttClient.onMessageArrived = (message) => {
            try {
                const data = JSON.parse(message.payloadString);
                if (message.destinationName === "esp32/cam/classification") {
                    updateClassification(data);
                }
            } catch (e) {
                console.error("Error parsing MQTT message:", e);
            }
        };
        
        function updateClassification(data) {
            document.getElementById('result').textContent = "Class: " + data.label;
            document.getElementById('confidence').textContent = 
                "Confidence: " + (data.confidence * 100).toFixed(1) + "%";
            
            const timestamp = new Date(data.timestamp);
            document.getElementById('timestamp').textContent = 
                "Last update: " + timestamp.toLocaleTimeString();
        }
        
        // Connect to MQTT
        mqttClient.connect({
            onSuccess: () => {
                console.log("Connected to MQTT");
                mqttClient.subscribe("esp32/cam/classification");
            },
            onFailure: (error) => {
                console.log("MQTT connection failed:", error.errorMessage);
            }
        });
        
        // Auto-refresh stream every 30 seconds to prevent timeout
        setInterval(() => {
            if (streamEnabled) {
                const img = document.getElementById('stream');
                img.src = img.src.split('?')[0] + "?t=" + new Date().getTime();
            }
        }, 30000);
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

/* Handle MJPEG stream */
void handleStream() {
  WiFiClient client = server.client();
  
  // Get requested FPS
  String fpsParam = server.arg("fps");
  if (fpsParam != "") {
    streamDelay = 1000 / fpsParam.toInt();
    if (streamDelay < 33) streamDelay = 33; // Max ~30 FPS
  }
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n";
  response += "Access-Control-Allow-Origin: *\r\n";
  response += "\r\n";
  
  client.print(response);
  
  unsigned long lastFrame = 0;
  
  while (client.connected()) {
    unsigned long now = millis();
    if (now - lastFrame >= streamDelay) {
      lastFrame = now;
      
      camera_fb_t* fb = esp_camera_fb_get();
      if (!fb) {
        continue;
      }
      
      client.print("--frame\r\n");
      client.print("Content-Type: image/jpeg\r\n");
      client.print("Content-Length: " + String(fb->len) + "\r\n");
      client.print("\r\n");
      client.write((char*)fb->buf, fb->len);
      client.print("\r\n");
      
      esp_camera_fb_return(fb);
    }
    
    // Small delay to prevent CPU hogging
    delay(1);
    
    // Check if client is still connected
    if (!client.connected()) {
      break;
    }
  }
}

/* Handle single JPEG capture */
void handleJPG() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

/* Run Edge Impulse Inference */
void runInference() {
  unsigned long now = millis();
  
  if (now - lastInferenceTime < INFERENCE_INTERVAL) {
    return;
  }
  lastInferenceTime = now;
  
  // Allocate buffer if needed
  if (!inferenceBuffer) {
    inferenceBuffer = (uint8_t*)malloc(320 * 240 * 3);  // QVGA RGB
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
  
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > bestConfidence) {
      bestConfidence = result.classification[i].value;
      bestIndex = i;
      bestLabel = String(ei_classifier_inferencing_categories[i]);
    }
  }
  
  // Create JSON message
  StaticJsonDocument<512> doc;
  doc["client_id"] = clientID;
  doc["timestamp"] = now;
  doc["label"] = bestLabel;
  doc["confidence"] = bestConfidence;
  doc["inference_time"] = result.timing.classification;
  
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
      Serial.println("MQTT publish failed");
    }
  }
  
  // Also print to Serial for debugging
  Serial.print("RESULT:");
  Serial.println(jsonString);
}

/* Setup */
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("ESP32-CAM Streaming & Classification");
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
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.on("/jpg", handleJPG);
  server.begin();
  
  Serial.println("HTTP server started");
  
  // Flash LED to indicate ready
  pinMode(4, OUTPUT);
  for(int i = 0; i < 3; i++) {
    digitalWrite(4, HIGH);
    delay(200);
    digitalWrite(4, LOW);
    delay(200);
  }
  
  Serial.println("\nSystem ready!");
  Serial.println("Access the stream at:");
  Serial.print("  http://");
  Serial.println(WiFi.localIP());
  Serial.println("=================================");
}

/* Main Loop */
void loop() {
  // Handle web clients
  server.handleClient();
  
  // Maintain MQTT connection
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
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
  
  // Small delay
  delay(10);
}