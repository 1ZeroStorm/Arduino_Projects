// README: note from the creator
/*
- just capturing the image, 180 degree flip, 
- converting it into base64 string
- sending to MQTT hivemq broker with wifi
- for inital testing: using FRAMESIZE_QVGA (previous code uses FRAMESIZE_SVGA if psram is available)
- mission testing if publishing data (and received) is working for differend framesize
*/

// camera_test_working.ino
#include "esp_camera.h"
#include "base64.h" // Required for string conversion
#include <WiFi.h> // wifi to connect the MQTT server
#include <PubSubClient.h> // lib to send the data to MQTT
#include "mbedtls/aes.h"

// AI Thinker ESP32-CAM pin definitions (most common)
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

// WIFI CONFIG
const char* ssid = "Aryahiro";
const char* password = "angga1407";
const char* mqtt_server = "broker.hivemq.com"; // From HiveMQ panel
const char* mqtt_topic = "esp32/camera/images";
const int mqtt_port = 1883; // 8883 for SSL, but start with 1883 for testing

// MQTT CONFIG
WiFiClient espClient;
PubSubClient client(espClient);

// WIFI SETUP
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// CONNECTION TO MQTT hivemq handling
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Use your HiveMQ Cluster username and password
    if (client.connect("ESP32CAM_Client", "user", "pass")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// function to encrypt the base64
// This must match your Python script exactly!
unsigned char key[16] = {'m','y','s','u','p','e','r','s','e','c','r','e','t','k','e','y'};
unsigned char iv[16]  = {'1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6'};

// Function to encrypt the image
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

//  MAIN SETUP
void setup() {
  Serial.begin(115200);
  delay(3000);  // Give time to open Serial Monitor
  Serial.println("\n=================================");
  Serial.println("ESP32-CAM Camera Test");
  Serial.println("=================================");
  
  // init wifi
  Serial.println("Initializing wifi and client mqtt...");
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  // INCREASE BUFFER SIZE: MQTT standard limit is 256 bytes. 
  // An image string is ~20,000+ bytes!
  client.setBufferSize(40000);

  // initalize camera configuration
  Serial.println("Initializing camera...");
  camera_config_t config;
  
  // Assign pins CORRECTLY - using pin_dX members
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
  
  // Camera settings
  config.xclk_freq_hz = 20000000;  // 20MHz
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // Start small (320x240) to test (instead of using FRAMESIZE_VGA)
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  
  // camera not ok
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    
    // Try alternative configuration - smaller frame size
    Serial.println("Trying alternative configuration (QQVGA)...");
    
    config.frame_size = FRAMESIZE_QQVGA;  // 160x120 - smallest
    config.jpeg_quality = 10;
    config.fb_count = 1;
    
    err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
      Serial.printf("Alternative config also failed: 0x%x\n", err);
      Serial.println("\nPossible causes:");
      Serial.println("1. Wrong pin definitions for your board");
      Serial.println("2. Camera module not connected properly");
      Serial.println("3. Camera module is damaged");
      Serial.println("4. Power supply insufficient (need 5V 2A)");
      Serial.println("5. Try different camera model definitions");
      return;
    }
  }
  
  //  FLIP THE CAMERA 180 DEGREES
  sensor_t * s = esp_camera_sensor_get();
  s->set_hmirror(s, 1); // Flip horizontally
  s->set_vflip(s, 1);   // Flip vertically

  Serial.println("Camera initialized successfully!");
  
  // Get sensor info (updated for new ESP32 core)
  Serial.println("\nCamera Sensor Info:");
  Serial.printf("  PID: 0x%04X\n", s->id.PID);
  // manufacturer field might not exist in newer cores
  // Serial.printf("  Manufacturer: 0x%04X\n", s->id.manufacturer);
  Serial.printf("  Version: %d\n", s->id.VER);
  
  
  Serial.println("\n=================================");
  Serial.println("Camera test complete!");
  Serial.println("If you see frame sizes above, camera is working!");
  Serial.println("=================================");
}

void loop() {
    // Check if PSRAM is available
    if(psramFound()){
        Serial.println("PSRAM found - using higher quality settings is possible: 800x600");
    } else {
        Serial.println("No PSRAM - using lower quality settings is possible: 640x480");
    }

    // client connection handling
    if (!client.connected()) reconnect();
    client.loop();

    // camera capturing
    camera_fb_t *fb = esp_camera_fb_get();
    if(!fb) {
        Serial.println("Camera capture failed!");
    } else {
        //success log
        Serial.printf("  Success! Frame: %dx%d, Size: %d bytes\n", 
                    fb->width, fb->height, fb->len);
        Serial.printf("  Format: %d\n", fb->format);

        // Now we get an ENCRYPTED string (AES encription and base64)
        String encryptedString = encryptImage(fb);

        // Publish to HiveMQ
        bool success = client.publish(mqtt_topic, encryptedString.c_str());
        if(success) Serial.println("Published successfully!");
        else Serial.println("Publish failed - String might be too big");

        // Return the frame buffer
        esp_camera_fb_return(fb); // clean up memory
        Serial.printf("\n================ ended encryption =====================\n");
    }
    
    delay(1000);

  
}