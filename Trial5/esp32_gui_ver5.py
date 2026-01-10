"""
Simple ESP32-CAM Web Viewer
Run with: python simple_viewer.py
"""

from flask import Flask, render_template, Response, jsonify, request
import cv2
import numpy as np
import threading
import time
import json
import base64
import queue
import paho.mqtt.client as mqtt
import requests
from datetime import datetime

app = Flask(__name__)

# Configuration
ESP32_IP = "192.168.100.4"  # Change to your ESP32 IP
STREAM_URL = f"http://{ESP32_IP}/stream"
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883

# Global state
latest_frame = None
latest_classification = None
mqtt_connected = False
streaming = True
frame_queue = queue.Queue(maxsize=10)

class MQTTManager:
    def __init__(self):
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.connect()
        
    def on_connect(self, client, userdata, flags, rc):
        global mqtt_connected
        mqtt_connected = (rc == 0)
        if rc == 0:
            print("Connected to MQTT broker")
            client.subscribe("esp32/cam/classification")
            client.subscribe("esp32/cam/status")
            
    def on_message(self, client, userdata, msg):
        global latest_classification
        if msg.topic == "esp32/cam/classification":
            latest_classification = json.loads(msg.payload.decode())
            print(f"Classification: {latest_classification.get('label')}")
            
    def connect(self):
        self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
        self.client.loop_start()

def video_stream_worker():
    """Background thread for video streaming"""
    global latest_frame, streaming
    
    while True:
        if not streaming:
            time.sleep(1)
            continue
            
        try:
            stream = requests.get(STREAM_URL, stream=True, timeout=5)
            bytes_data = bytes()
            
            for chunk in stream.iter_content(chunk_size=1024):
                if not streaming:
                    break
                    
                bytes_data += chunk
                a = bytes_data.find(b'\xff\xd8')
                b = bytes_data.find(b'\xff\xd9')
                
                if a != -1 and b != -1:
                    jpg = bytes_data[a:b+2]
                    bytes_data = bytes_data[b+2:]
                    
                    try:
                        img_array = np.frombuffer(jpg, dtype=np.uint8)
                        img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                        
                        if img is not None:
                            latest_frame = img
                            if not frame_queue.full():
                                frame_queue.put(img)
                                
                    except:
                        pass
                        
        except:
            time.sleep(2)

@app.route('/')
def index():
    """Main page"""
    return '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32-CAM Viewer</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
            .container { max-width: 1200px; margin: 0 auto; }
            .header { background: #333; color: white; padding: 20px; border-radius: 10px; }
            .content { display: flex; gap: 20px; margin-top: 20px; }
            .video-panel { flex: 2; background: white; padding: 20px; border-radius: 10px; }
            .info-panel { flex: 1; background: white; padding: 20px; border-radius: 10px; }
            #videoStream { width: 100%; max-width: 800px; border: 2px solid #333; }
            .controls { margin: 20px 0; }
            button { padding: 10px 20px; margin: 5px; background: #4CAF50; color: white; border: none; cursor: pointer; }
            .classification { background: #e8f5e9; padding: 15px; border-radius: 5px; margin: 10px 0; }
            .prob-bar { height: 20px; background: #ddd; margin: 5px 0; border-radius: 10px; overflow: hidden; }
            .prob-fill { height: 100%; background: #4CAF50; }
            .status { padding: 10px; margin: 5px 0; border-radius: 5px; }
            .connected { background: #d4edda; }
            .disconnected { background: #f8d7da; }
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <h1>ESP32-CAM Live Stream</h1>
                <p>Real-time video streaming with AI classification</p>
            </div>
            
            <div class="content">
                <div class="video-panel">
                    <h2>Live Feed</h2>
                    <img id="videoStream" src="/video_feed" alt="Live Stream">
                    
                    <div class="controls">
                        <button onclick="toggleStream()">Toggle Stream</button>
                        <button onclick="captureFrame()">Capture Frame</button>
                        <button onclick="location.reload()">Refresh</button>
                    </div>
                </div>
                
                <div class="info-panel">
                    <h2>Classification Results</h2>
                    <div id="classificationResult">Waiting for data...</div>
                    <div id="probabilities"></div>
                    
                    <h2>System Status</h2>
                    <div id="statusDisplay">
                        <div class="status" id="mqttStatus">MQTT: Disconnected</div>
                        <div class="status" id="streamStatus">Stream: Stopped</div>
                        <div id="clientId">Client: --</div>
                    </div>
                    
                    <h2>Log</h2>
                    <div id="log" style="height: 200px; overflow-y: auto; background: #f8f9fa; padding: 10px; font-size: 12px;"></div>
                </div>
            </div>
        </div>
        
        <script>
            let lastUpdate = Date.now();
            
            // Update video stream
            function updateStream() {
                const img = document.getElementById('videoStream');
                img.src = '/video_feed?t=' + Date.now();
            }
            
            // Update classification
            function updateClassification() {
                fetch('/api/classification')
                    .then(response => response.json())
                    .then(data => {
                        if (data.label) {
                            const resultDiv = document.getElementById('classificationResult');
                            const confidence = (data.confidence * 100).toFixed(1);
                            resultDiv.innerHTML = `
                                <div class="classification">
                                    <h3>${data.label}</h3>
                                    <p>Confidence: ${confidence}%</p>
                                    <p>Time: ${new Date(data.timestamp).toLocaleTimeString()}</p>
                                </div>
                            `;
                            
                            // Update probabilities
                            if (data.probabilities) {
                                const probDiv = document.getElementById('probabilities');
                                probDiv.innerHTML = '<h4>Probabilities:</h4>';
                                data.probabilities.forEach(prob => {
                                    const value = (prob.value * 100).toFixed(1);
                                    probDiv.innerHTML += `
                                        <div>
                                            <span>${prob.label}: </span>
                                            <div class="prob-bar">
                                                <div class="prob-fill" style="width: ${value}%"></div>
                                            </div>
                                            <span>${value}%</span>
                                        </div>
                                    `;
                                });
                            }
                            
                            // Update log
                            addLog(`Classification: ${data.label} (${confidence}%)`);
                        }
                    });
            }
            
            // Update status
            function updateStatus() {
                fetch('/api/status')
                    .then(response => response.json())
                    .then(data => {
                        const mqttDiv = document.getElementById('mqttStatus');
                        const streamDiv = document.getElementById('streamStatus');
                        
                        mqttDiv.textContent = `MQTT: ${data.mqtt_connected ? 'Connected' : 'Disconnected'}`;
                        mqttDiv.className = `status ${data.mqtt_connected ? 'connected' : 'disconnected'}`;
                        
                        streamDiv.textContent = `Stream: ${data.streaming ? 'Active' : 'Stopped'}`;
                        streamDiv.className = `status ${data.streaming ? 'connected' : 'disconnected'}`;
                        
                        if (data.client_id) {
                            document.getElementById('clientId').textContent = `Client: ${data.client_id}`;
                        }
                    });
            }
            
            // Toggle stream
            function toggleStream() {
                fetch('/control/toggle', {method: 'POST'})
                    .then(() => {
                        updateStatus();
                        addLog('Stream toggled');
                    });
            }
            
            // Capture frame
            function captureFrame() {
                fetch('/capture', {method: 'POST'})
                    .then(response => response.json())
                    .then(data => {
                        if (data.filename) {
                            addLog(`Captured: ${data.filename}`);
                        }
                    });
            }
            
            // Add log entry
            function addLog(message) {
                const logDiv = document.getElementById('log');
                const timestamp = new Date().toLocaleTimeString();
                logDiv.innerHTML = `[${timestamp}] ${message}<br>` + logDiv.innerHTML;
            }
            
            // Auto-refresh
            setInterval(updateStream, 100);
            setInterval(updateClassification, 1000);
            setInterval(updateStatus, 2000);
            
            // Initial update
            updateStatus();
            updateClassification();
            addLog('System started');
        </script>
    </body>
    </html>
    '''

@app.route('/video_feed')
def video_feed():
    """MJPEG video stream"""
    def generate():
        while True:
            if latest_frame is not None:
                # Convert to JPEG
                _, buffer = cv2.imencode('.jpg', latest_frame)
                frame = buffer.tobytes()
                
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
            time.sleep(0.1)
    
    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/api/classification')
def get_classification():
    if latest_classification:
        return jsonify(latest_classification)
    return jsonify({'label': 'No data', 'confidence': 0})

@app.route('/api/status')
def get_status():
    return jsonify({
        'mqtt_connected': mqtt_connected,
        'streaming': streaming,
        'client_id': latest_classification.get('client_id', 'Unknown') if latest_classification else 'Unknown'
    })

@app.route('/control/toggle', methods=['POST'])
def toggle_stream():
    global streaming
    streaming = not streaming
    return jsonify({'streaming': streaming})

@app.route('/capture', methods=['POST'])
def capture():
    """Capture and save a frame"""
    if latest_frame is not None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"capture_{timestamp}.jpg"
        cv2.imwrite(filename, latest_frame)
        return jsonify({'filename': filename})
    return jsonify({'error': 'No frame available'}), 400

if __name__ == '__main__':
    # Start MQTT
    mqtt_manager = MQTTManager()
    
    # Start video stream thread
    stream_thread = threading.Thread(target=video_stream_worker)
    stream_thread.daemon = True
    stream_thread.start()
    
    print(f"\nESP32-CAM Web Viewer")
    print(f"=====================")
    print(f"Open in browser: http://localhost:5000")
    print(f"ESP32 IP: {ESP32_IP}")
    print(f"=====================\n")
    
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)