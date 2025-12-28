# Wireless ESP32 (wokwi) to python communication via MQTT using hivemq broker
arduino code sends the ```"Hello World from ESP32!"``` every 5 seconds to the mqtt server with specified port and server. The python from vs code decodes the message and responds back ```"Hello again ESP32!"``` back to the ESP 32

## how to run
1) open project for esp 32 in wokwi
2) put ```diagram.json``` on the wokwi page for diagram
3) copy and pase sketch file to the page
4) open IDE for python and paste ```python-code.py```
5) ) run both projects

## dependencies
Wokwi/Arduino IDE
- PubSubClient library

python 
- ```pip install paho-mqtt```
