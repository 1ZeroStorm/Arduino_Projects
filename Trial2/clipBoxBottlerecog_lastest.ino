/* Edge Impulse Arduino examples
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// These sketches are tested with 2.0.4 ESP32 Arduino Core
// https://github.com/espressif/arduino-esp32/releases/tag/2.0.4

/* Includes ---------------------------------------------------------------- */
#include <bottle_and_clipbox_recognition_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

#include "esp_camera.h"

// Select camera model - find more camera models in camera_pins.h file here
// https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Camera/CameraWebServer/camera_pins.h

//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#if defined(CAMERA_MODEL_ESP_EYE)
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    4
#define SIOD_GPIO_NUM    18
#define SIOC_GPIO_NUM    23

#define Y9_GPIO_NUM      36
#define Y8_GPIO_NUM      37
#define Y7_GPIO_NUM      38
#define Y6_GPIO_NUM      39
#define Y5_GPIO_NUM      35
#define Y4_GPIO_NUM      14
#define Y3_GPIO_NUM      13
#define Y2_GPIO_NUM      34
#define VSYNC_GPIO_NUM   5
#define HREF_GPIO_NUM    27
#define PCLK_GPIO_NUM    25

#elif defined(CAMERA_MODEL_AI_THINKER)
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

#else
#error "Camera model not selected"
#endif

/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; //points to the output of the capture

// For local Python GUI communication
String last_label = "N/A";
float last_confidence = 0.0f;
bool send_to_serial = true;  // Set to true to enable data output for Python GUI

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

/* Function definitions ------------------------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

/**
* @brief      Arduino setup function
*/
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("Edge Impulse Inferencing Demo - Local GUI Version");
    
    if (ei_camera_init() == false) {
        Serial.println("Failed to initialize Camera!");
    }
    else {
        Serial.println("Camera initialized");
    }
    
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);

    Serial.println("\nWaiting for connection from Python GUI...");
    Serial.println("Starting continuous inference...");
    delay(2000);
}

/**
* @brief      Get data and run inferencing
*
* @param[in]  debug  Get debug info if true
*/
void loop()
{
    digitalWrite(4, HIGH);
    delay(5);

    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

    if(snapshot_buf == nullptr) {
        Serial.println("ERR: Failed to allocate snapshot buffer!");
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    // Call the capture function
    if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
        Serial.println("Failed to capture image");
        free(snapshot_buf);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("ERR: Failed to run classifier (%d)\n", err);
        free(snapshot_buf);
        return;
    }

    // Process classification results
    float best_val = 0.0f;
    String best_label = "N/A";
    int best_idx = 0;

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    // Object detection mode
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) continue;
        if (bb.value > best_val) {
            best_val = bb.value;
            best_label = String(bb.label);
        }
    }
#else
    // Simple classification mode
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > best_val) {
            best_val = result.classification[i].value;
            best_idx = i;
            best_label = String(ei_classifier_inferencing_categories[best_idx]);
        }
    }
#endif

    last_label = best_label;
    last_confidence = best_val;

    // Send structured data to Python GUI
    if (send_to_serial) {
        // Create JSON-like output for Python GUI
        Serial.print("RESULT:");
        Serial.print("{\"label\":\"");
        Serial.print(last_label);
        Serial.print("\",\"confidence\":");
        Serial.print(last_confidence, 4);
        
#if EI_CLASSIFIER_HAS_ANOMALY
        Serial.print(",\"anomaly\":");
        Serial.print(result.anomaly, 4);
#endif
        
        Serial.print(",\"timing\":{\"dsp\":");
        Serial.print(result.timing.dsp);
        Serial.print(",\"classification\":");
        Serial.print(result.timing.classification);
        Serial.print("}");
        
        // Include all class probabilities
        Serial.print(",\"probabilities\":[");
        for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (i > 0) Serial.print(",");
            Serial.print(result.classification[i].value, 4);
        }
        Serial.print("]");
        
        Serial.println("}");
    }

    // Print debug info
    Serial.printf("Predictions (DSP: %d ms., Classification: %d ms.): \n",
                result.timing.dsp, result.timing.classification);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    Serial.println("Object detection bounding boxes:");
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) {
            continue;
        }
        Serial.printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n",
                bb.label,
                bb.value,
                bb.x,
                bb.y,
                bb.width,
                bb.height);
    }
#else
    Serial.println("Classification results:");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("  %s: ", ei_classifier_inferencing_categories[i]);
        Serial.printf("%.5f\n", result.classification[i].value);
    }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY
    Serial.printf("Anomaly prediction: %.3f\n", result.anomaly);
#endif

    free(snapshot_buf);
}

/**
 * @brief   Setup image sensor & start streaming
 */
bool ei_camera_init(void) {
    if (is_initialised) return true;

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x\n", err);
      return false;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1);
      s->set_brightness(s, 1);
      s->set_saturation(s, 0);
    }

    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);

    is_initialised = true;
    return true;
}

/**
 * @brief      Stop streaming of sensor data
 */
void ei_camera_deinit(void) {
    esp_err_t err = esp_camera_deinit();

    if (err != ESP_OK) {
        Serial.println("Camera deinit failed");
        return;
    }

    is_initialised = false;
    return;
}

/**
 * @brief      Capture, rescale and crop image
 *
 * @param[in]  img_width     width of output image
 * @param[in]  img_height    height of output image
 * @param[in]  out_buf       pointer to store output image
 *
 * @retval     false if not initialised, image captured, rescaled or cropped failed
 */
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    if (!is_initialised) {
        Serial.println("ERR: Camera is not initialized");
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return false;
    }

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, out_buf);
    esp_camera_fb_return(fb);

    if(!converted){
        Serial.println("Conversion failed");
        return false;
    }

    // Check if resizing is needed
    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || 
        (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height
        );
    }

    return true;
}

/**
 * @brief      Get data from camera buffer
 */
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    // Convert offset to pixel index (RGB888 has 3 bytes per pixel)
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        // Convert RGB888 to RGB888 (already in correct format)
        // The Edge Impulse SDK expects RGB format
        uint8_t r = snapshot_buf[pixel_ix];
        uint8_t g = snapshot_buf[pixel_ix + 1];
        uint8_t b = snapshot_buf[pixel_ix + 2];
        
        // Convert to float RGB
        out_ptr[out_ptr_ix] = (r << 16) + (g << 8) + b;

        // Move to next pixel
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif