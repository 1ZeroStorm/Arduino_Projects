/* ESP32-CAM Edge Impulse Classification + Video Streaming */
#include <WiFi.h>
#include <WebServer.h>
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

/* Camera Settings */
#define FRAME_SIZE FRAMESIZE_QVGA  // 320x240
#define JPEG_QUALITY 10
#define INFERENCE_INTERVAL 2000  // Run inference every 2 seconds

/* Global Variables */
WebServer server(80);
uint8_t* inferenceBuffer = NULL;
unsigned long lastInferenceTime = 0;
String clientID;
bool streamEnabled = true;
uint32_t streamDelay = 100; // ms between frames

// Variables to store classification results
String lastLabel = "none";
float lastConfidence = 0.0;
unsigned long lastInferenceTimestamp = 0;
bool inferenceRunning = false;

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
    Serial.print("Classification data: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/classification");
  } else {
    Serial.println("\nWiFi connection failed!");
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
    <meta charset="UTF-8">
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
        button.stop { background: #f44336; }
        button.stop:hover { background: #d32f2f; }
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
            border-left: 5px solid #2196F3;
        }
        .classification h3 { margin-top: 0; }
        .result { 
            font-size: 24px; 
            font-weight: bold;
            color: #2196F3;
            margin: 10px 0;
            padding: 10px;
            background: white;
            border-radius: 5px;
        }
        .confidence { 
            font-size: 18px; 
            color: #4CAF50;
            margin: 10px 0;
        }
        .timestamp {
            font-size: 14px;
            color: #666;
            font-style: italic;
        }
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }
        .status-connected {
            background-color: #4CAF50;
            animation: pulse 2s infinite;
        }
        .status-disconnected {
            background-color: #f44336;
        }
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
        .stats {
            display: flex;
            justify-content: space-around;
            margin: 20px 0;
            padding: 15px;
            background: #f0f7ff;
            border-radius: 10px;
        }
        .stat-item {
            text-align: center;
        }
        .stat-value {
            font-size: 24px;
            font-weight: bold;
            color: #2196F3;
        }
        .stat-label {
            font-size: 14px;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-CAM Live Stream with AI Classification</h1>
        
        <div id="status">
            <p><span class="status-indicator status-connected"></span>Streaming from ESP32-CAM</p>
            <p>IP: )rawliteral" + WiFi.localIP().toString() + R"rawliteral(</p>
            <p>Client ID: )rawliteral" + clientID + R"rawliteral(</p>
            <p>Inference runs every 2 seconds</p>
        </div>
        
        <div class="stats">
            <div class="stat-item">
                <div class="stat-value" id="currentFPS">10</div>
                <div class="stat-label">FPS</div>
            </div>
            <div class="stat-item">
                <div class="stat-value" id="inferenceTime">0</div>
                <div class="stat-label">ms</div>
            </div>
            <div class="stat-item">
                <div class="stat-value" id="confidenceValue">0%</div>
                <div class="stat-label">Confidence</div>
            </div>
        </div>
        
        <div class="video-container">
            <img src="/stream" id="stream" alt="Video Stream">
        </div>
        
        <div class="classification" id="classification">
            <h3>AI Classification Results</h3>
            <div class="result" id="result">Waiting for classification...</div>
            <div class="confidence" id="confidence">Confidence: 0%</div>
            <div class="timestamp" id="timestamp">Last update: Never</div>
        </div>
        
        <div class="controls">
            <button onclick="location.reload()">Refresh Page</button>
            <button onclick="window.open('/jpg', '_blank')">Capture Still Image</button>
            <button onclick="toggleStream()" id="streamBtn">Stop Stream</button>
            <button onclick="forceInference()" id="inferenceBtn">Run Inference Now</button>
        </div>
        
        <div>
            <p>Connection: <span id="connStatus">Streaming</span></p>
            <p>Frame Rate: <input type="range" id="fps" min="1" max="30" value="10" onchange="changeFPS()"> 
               <span id="fpsValue">10</span> FPS</p>
        </div>
    </div>

    <script>
        let streamEnabled = true;
        let currentFPS = 10;
        let lastClassificationTime = 0;
        
        function toggleStream() {
            streamEnabled = !streamEnabled;
            const img = document.getElementById('stream');
            const btn = document.getElementById('streamBtn');
            const connStatus = document.getElementById('connStatus');
            
            if (streamEnabled) {
                img.src = "/stream?fps=" + currentFPS;
                btn.textContent = "Stop Stream";
                btn.classList.remove('stop');
                connStatus.textContent = "Streaming";
                connStatus.style.color = "#4CAF50";
            } else {
                img.src = "";
                btn.textContent = "Start Stream";
                btn.classList.add('stop');
                connStatus.textContent = "Paused";
                connStatus.style.color = "#f44336";
            }
        }
        
        function changeFPS() {
            const slider = document.getElementById('fps');
            const fpsValue = document.getElementById('fpsValue');
            const currentFPSValue = document.getElementById('currentFPS');
            currentFPS = parseInt(slider.value);
            fpsValue.textContent = currentFPS;
            currentFPSValue.textContent = currentFPS;
            
            if (streamEnabled) {
                document.getElementById('stream').src = "/stream?fps=" + currentFPS;
            }
        }
        
        function formatTime(timestamp) {
            const date = new Date(timestamp);
            return date.toLocaleTimeString();
        }
        
        function formatTimeAgo(timestamp) {
            const now = Date.now();
            const diff = now - timestamp;
            
            if (diff < 60000) {
                return "Just now";
            } else if (diff < 3600000) {
                return Math.floor(diff / 60000) + " minutes ago";
            } else if (diff < 86400000) {
                return Math.floor(diff / 3600000) + " hours ago";
            } else {
                return Math.floor(diff / 86400000) + " days ago";
            }
        }
        
        function updateClassificationUI(data) {
            if (data && data.label && data.label !== "none") {
                const resultElem = document.getElementById('result');
                const confidenceElem = document.getElementById('confidence');
                const timestampElem = document.getElementById('timestamp');
                const confidenceValueElem = document.getElementById('confidenceValue');
                
                const confidencePercent = (data.confidence * 100).toFixed(1);
                const timeAgo = formatTimeAgo(data.timestamp);
                
                resultElem.textContent = "🎯 Detected: " + data.label.toUpperCase();
                confidenceElem.textContent = "Confidence: " + confidencePercent + "%";
                timestampElem.textContent = "Last update: " + timeAgo + " (" + formatTime(data.timestamp) + ")";
                confidenceValueElem.textContent = confidencePercent + "%";
                
                // Visual feedback based on confidence
                if (data.confidence > 0.8) {
                    resultElem.style.color = "#4CAF50";
                    resultElem.style.backgroundColor = "#e8f5e9";
                } else if (data.confidence > 0.6) {
                    resultElem.style.color = "#FF9800";
                    resultElem.style.backgroundColor = "#fff3e0";
                } else {
                    resultElem.style.color = "#f44336";
                    resultElem.style.backgroundColor = "#ffebee";
                }
                
                lastClassificationTime = data.timestamp;
                
                // Update inference time if available
                if (data.inference_time) {
                    document.getElementById('inferenceTime').textContent = data.inference_time;
                }
            } else {
                document.getElementById('result').textContent = "⏳ Waiting for classification...";
                document.getElementById('result').style.color = "#666";
                document.getElementById('result').style.backgroundColor = "#f5f5f5";
            }
        }
        
        function fetchClassification() {
            fetch('/classification')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.json();
                })
                .then(data => {
                    console.log('Received classification:', data);
                    updateClassificationUI(data);
                })
                .catch(error => {
                    console.error('Error fetching classification:', error);
                    document.getElementById('result').textContent = "⚠️ Error fetching classification data";
                    document.getElementById('result').style.color = "#f44336";
                });
        }
        
        function forceInference() {
            fetch('/force-inference')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Force inference failed');
                    }
                    return response.text();
                })
                .then(data => {
                    console.log('Force inference response:', data);
                    document.getElementById('result').textContent = "🔄 Running inference...";
                    setTimeout(fetchClassification, 1000);
                })
                .catch(error => {
                    console.error('Error forcing inference:', error);
                });
        }
        
        // Initial fetch
        fetchClassification();
        
        // Poll for updates every 1 second
        setInterval(fetchClassification, 1000);
        
        // Auto-refresh stream every 30 seconds to prevent timeout
        setInterval(() => {
            if (streamEnabled) {
                const img = document.getElementById('stream');
                // Add timestamp to prevent caching
                const currentSrc = img.src.split('?')[0];
                img.src = currentSrc + "?t=" + new Date().getTime() + "&fps=" + currentFPS;
            }
        }, 30000);
        
        // Update FPS slider display
        document.getElementById('fps').addEventListener('input', function() {
            document.getElementById('fpsValue').textContent = this.value;
            document.getElementById('currentFPS').textContent = this.value;
        });
        
        // Initialize display
        document.getElementById('currentFPS').textContent = currentFPS;
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

/* Handle classification data request */
void handleClassification() {
  String jsonResponse = "{";
  jsonResponse += "\"client_id\":\"" + clientID + "\",";
  jsonResponse += "\"label\":\"" + lastLabel + "\",";
  jsonResponse += "\"confidence\":" + String(lastConfidence, 4) + ",";
  jsonResponse += "\"timestamp\":" + String(lastInferenceTimestamp);
  
  // Add inference time if available
  if (lastInferenceTimestamp > 0) {
    jsonResponse += ",\"inference_time\":100"; // Default value
  }
  
  jsonResponse += "}";
  
  server.send(200, "application/json", jsonResponse);
  
  // Debug output
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 5000) { // Only print every 5 seconds
    lastDebugTime = millis();
    Serial.print("Served classification data: ");
    Serial.println(jsonResponse);
  }
}

/* Handle force inference request */
void handleForceInference() {
  lastInferenceTime = 0; // Reset timer to force immediate inference
  server.send(200, "text/plain", "Inference will run on next loop");
  Serial.println("Force inference requested");
}

/* Run Edge Impulse Inference - Fixed version */
void runInference() {
  unsigned long now = millis();
  
  // Check if it's time to run inference
  if (now - lastInferenceTime < INFERENCE_INTERVAL) {
    return;
  }
  
  // Prevent multiple simultaneous inferences
  if (inferenceRunning) {
    return;
  }
  
  inferenceRunning = true;
  
  Serial.println("\n=== Starting Inference ===");
  
  // Allocate buffer if needed
  if (!inferenceBuffer) {
    Serial.println("Allocating inference buffer...");
    inferenceBuffer = (uint8_t*)malloc(320 * 240 * 3);  // QVGA RGB
    if (!inferenceBuffer) {
      Serial.println("ERROR: Failed to allocate inference buffer!");
      inferenceRunning = false;
      return;
    }
    Serial.println("Inference buffer allocated");
  }
  
  // Capture frame for inference
  Serial.println("Capturing frame...");
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("ERROR: Camera capture failed for inference");
    inferenceRunning = false;
    return;
  }
  
  Serial.printf("Frame captured: %d bytes\n", fb->len);
  
  // Convert JPEG to RGB888
  Serial.println("Converting JPEG to RGB...");
  if (!fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, inferenceBuffer)) {
    Serial.println("ERROR: JPEG to RGB conversion failed");
    esp_camera_fb_return(fb);
    inferenceRunning = false;
    return;
  }
  esp_camera_fb_return(fb);
  Serial.println("Conversion successful");
  
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
  Serial.println("Running classifier...");
  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  
  if (err != EI_IMPULSE_OK) {
    Serial.printf("ERROR: Inference failed with code: %d\n", err);
    inferenceRunning = false;
    return;
  }
  
  // Find best classification
  float bestConfidence = 0;
  String bestLabel = "unknown";
  
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > bestConfidence) {
      bestConfidence = result.classification[i].value;
      bestLabel = String(ei_classifier_inferencing_categories[i]);
    }
  }
  
  // Update global variables
  lastLabel = bestLabel;
  lastConfidence = bestConfidence;
  lastInferenceTimestamp = millis();
  lastInferenceTime = now;
  
  // Print detailed results to Serial
  Serial.println("\n=== INFERENCE RESULTS ===");
  Serial.print("Label: ");
  Serial.println(bestLabel);
  Serial.print("Confidence: ");
  Serial.print(bestConfidence * 100, 1);
  Serial.println("%");
  Serial.print("Inference time: ");
  Serial.print(result.timing.classification);
  Serial.println(" ms");
  Serial.print("Timestamp: ");
  Serial.println(lastInferenceTimestamp);
  Serial.println("=========================\n");
  
  
  inferenceRunning = false;
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
  
  // Generate client ID
  clientID = generateClientID();
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.on("/jpg", handleJPG);
  server.on("/classification", handleClassification);
  server.on("/force-inference", handleForceInference);
  server.begin();
  
  Serial.println("HTTP server started");
  
  // Setup LED pin (GPIO 4 for ESP32-CAM)
  pinMode(4, OUTPUT);
  
  // Make LED pink on startup (blink pattern)
  for(int i = 0; i < 3; i++) {
    digitalWrite(4, HIGH);
    delay(100);
    digitalWrite(4, LOW);
    delay(100);
  }
  
  // Keep LED on (pink) after startup
  digitalWrite(4, HIGH);
  
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
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWiFiReconnect = 0;
    if (millis() - lastWiFiReconnect > 10000) {
      lastWiFiReconnect = millis();
      Serial.println("Reconnecting WiFi...");
      WiFi.reconnect();
      delay(1000);
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi reconnected!");
      }
    }
  }
  
  // Run inference
  runInference();
  
  // Small delay to prevent watchdog timer issues
  delay(10);
}