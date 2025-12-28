# Wireless ESP32 (wokwi) to python communication via MQTT using hivemq broker
arduino code sends the ```"Hello World from ESP32!"``` every 5 seconds to the mqtt server with specified port and server. The python from vs code decodes the message and responds back ```"Hello again ESP32!"``` back to the ESP 32


## dependancies

Wokwi/Arduino IDE
- PubSubClient library

python 
- ```pip install paho-mqtt```
