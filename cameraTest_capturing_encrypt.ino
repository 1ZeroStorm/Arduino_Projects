// camera_test_working.ino
#include "esp_camera.h"

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

void setup() {
  Serial.begin(115200);
  delay(3000);  // Give time to open Serial Monitor
  
  Serial.println("\n=================================");
  Serial.println("ESP32-CAM Camera Test");
  Serial.println("=================================");
  
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
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // Check if PSRAM is available
  if(psramFound()){
    Serial.println("PSRAM found - using higher quality settings");
    config.frame_size = FRAMESIZE_SVGA;  // 800x600
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    Serial.println("No PSRAM - using lower quality settings");
    config.frame_size = FRAMESIZE_VGA;   // 640x480
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }
  
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  
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
  sensor_t *s = esp_camera_sensor_get();
  Serial.println("\nCamera Sensor Info:");
  Serial.printf("  PID: 0x%04X\n", s->id.PID);
  // manufacturer field might not exist in newer cores
  // Serial.printf("  Manufacturer: 0x%04X\n", s->id.manufacturer);
  Serial.printf("  Version: %d\n", s->id.VER);
  
  // Test camera capture
  Serial.println("\nTesting camera capture...");
  for(int i = 0; i < 10; i++) {  
    //camera capture
    camera_fb_t *fb = esp_camera_fb_get();
    if(!fb) {
        Serial.println("Camera capture failed!");
    } else {
        //success log
        Serial.printf("  Success! Frame: %dx%d, Size: %d bytes\n", 
                    fb->width, fb->height, fb->len);
        Serial.printf("  Format: %d\n", fb->format);

        // Convert the binary buffer (fb->buf) into a Base64 String
        String imageAsString = base64::encode(fb->buf, fb->len);
        Serial.println("Image converted to string!");
        Serial.print("First 30 characters of string: ");
        Serial.println(imageAsString.substring(0, 30));

        // Return the frame buffer
        esp_camera_fb_return(fb); // clean up memory
        Serial.printf("\n================ ended encryption attempt %d =====================\n", i+1)
    }
    
    delay(1000);
  }
  
  Serial.println("\n=================================");
  Serial.println("Camera test complete!");
  Serial.println("If you see frame sizes above, camera is working!");
  Serial.println("=================================");
}

void loop() {
  
}