# version 2 with GUI and python integration

## Dependencies
```bash
pip install -r Trial2/requirements.txt
```

## How to run
1) run the .ino code via arduino IDE
- Select the correct COM port (Windows) or /dev/ttyUSB0 (Linux/Mac)
- Set baud rate to 115200
- Click "Connect"
2) run esp32_gui.py

## whats worked and not worked
1) worked: 
- serial log between arduino and python tkinter
- classification
- LED

2) needs to be fixed: 
- confidence rate: maybe an image classification where the box is empty (no object detected)
- camera live stream feedon the tkinter
