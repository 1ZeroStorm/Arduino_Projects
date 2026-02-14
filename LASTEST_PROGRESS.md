# lastest progress (8 Feb 2026)
- focusing on cameraCapturingSendingMQTT folder (trial 1)
- camera image capture and encryption with base64 worked
- MQTT utilization worked, received via python
- camera flipped 180 deg

# run procedure 
- run the cpp code in arduino
- set esp 32 cam to 5 volt
- run the python file '''python f:/github/Arduino_Projects/cameraCapturingSendingMQTT/trial1/pythonReceiver.py''''

# next progress
```mermaid
flowchart TD

A[Troubleshoot ESP32-CAM with shield module ✅] --> B[Reverse engineer image recognition with Edge Impulse imageRecogStudy.ino ✅]
B --> C[Image recognition while sending image to MQTT Wi-Fi and laptop ❌]
C --> D[Swap cable to data cable ❌]
D --> E[Debug MQTT image retrieval from ESP32-CAM with shield and image classification ❌]
E --> F[Make Python GUI with Tkinter to display probability and image result ❌]
F --> G[Swap Arduino with ESP32, communicate via Wi-Fi and MQTT ❌]
G --> H[Connect ESP32 to NEMA 17 stepper motor ❌]
H --> I[Debug and test full set ❌]

