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
- &#10006; <!-- (done) --> try the same but with esp shield
- &#10006; <!-- (done) --> reverse engineer cam edge impulse recog (imageRecogStudy.ino)
- &#10006; <!-- (not done) --> image recognition while simultaneously send the image to mqtt and to laptop