import cv2
import numpy as np
import requests
import threading
import time
import json
import paho.mqtt.client as mqtt
from datetime import datetime
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import queue
import io

class ESP32CamStreamViewer:
    def __init__(self, esp32_ip=None):
        self.esp32_ip = esp32_ip
        self.stream_url = f"http://{esp32_ip}/stream" if esp32_ip else None
        self.jpg_url = f"http://{esp32_ip}/jpg" if esp32_ip else None
        
        # MQTT Configuration
        self.mqtt_broker = "broker.hivemq.com"
        self.mqtt_port = 1883
        self.topic_classification = "esp32/cam/classification"
        self.topic_status = "esp32/cam/status"
        
        # Data storage
        self.latest_classification = None
        self.latest_frame = None
        self.connected = False
        self.client_id = None
        self.streaming = False
        self.frame_queue = queue.Queue(maxsize=10)
        
        # Initialize GUI
        self.root = tk.Tk()
        self.root.title("ESP32-CAM Live Stream with Classification")
        self.root.geometry("1400x900")
        
        self.setup_gui()
        
        # Connect to MQTT
        self.setup_mqtt()
        
        # Start video stream thread if IP provided
        if self.esp32_ip:
            self.start_video_stream()
        
        # Start GUI update loop
        self.update_gui()
        
    def setup_gui(self):
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=3)
        main_frame.columnconfigure(1, weight=1)
        main_frame.rowconfigure(1, weight=1)
        
        # Status bar at top
        status_frame = ttk.Frame(main_frame)
        status_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        
        self.status_label = ttk.Label(status_frame, text="Disconnected", foreground="red")
        self.status_label.pack(side=tk.LEFT, padx=5)
        
        self.client_label = ttk.Label(status_frame, text="Client: None")
        self.client_label.pack(side=tk.LEFT, padx=20)
        
        self.ip_label = ttk.Label(status_frame, text="IP: Not connected")
        self.ip_label.pack(side=tk.LEFT, padx=20)
        
        # Video display area
        video_frame = ttk.LabelFrame(main_frame, text="Live Camera Feed", padding="5")
        video_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(0, 10))
        
        self.video_label = ttk.Label(video_frame, text="Waiting for stream...\n\nESP32 IP: " + 
                                    (self.esp32_ip if self.esp32_ip else "Not specified"),
                                    font=('Arial', 12))
        self.video_label.pack(expand=True, fill=tk.BOTH)
        
        # Right panel for classification
        right_frame = ttk.Frame(main_frame)
        right_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Classification results
        results_frame = ttk.LabelFrame(right_frame, text="AI Classification Results", padding="15")
        results_frame.pack(fill=tk.BOTH, expand=False, pady=(0, 10))
        
        self.class_label = ttk.Label(results_frame, text="No classification yet", 
                                    font=('Arial', 16, 'bold'), foreground="blue")
        self.class_label.pack(pady=10)
        
        self.confidence_label = ttk.Label(results_frame, text="Confidence: 0%", 
                                         font=('Arial', 14))
        self.confidence_label.pack(pady=5)
        
        self.timestamp_label = ttk.Label(results_frame, text="Last update: Never", 
                                        font=('Arial', 10))
        self.timestamp_label.pack(pady=5)
        
        # Inference time
        self.inference_label = ttk.Label(results_frame, text="Inference time: 0 ms", 
                                        font=('Arial', 10))
        self.inference_label.pack(pady=5)
        
        # Probabilities chart
        chart_frame = ttk.LabelFrame(right_frame, text="Probability Distribution", padding="10")
        chart_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        self.prob_canvas = tk.Canvas(chart_frame, bg='white', height=250)
        self.prob_canvas.pack(fill=tk.BOTH, expand=True)
        
        # Controls frame
        control_frame = ttk.LabelFrame(right_frame, text="Controls", padding="10")
        control_frame.pack(fill=tk.X, pady=(0, 10))
        
        # IP address input
        ip_frame = ttk.Frame(control_frame)
        ip_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(ip_frame, text="ESP32 IP:").pack(side=tk.LEFT, padx=(0, 5))
        self.ip_entry = ttk.Entry(ip_frame)
        self.ip_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        if self.esp32_ip:
            self.ip_entry.insert(0, self.esp32_ip)
        
        ttk.Button(ip_frame, text="Connect", command=self.connect_to_ip).pack(side=tk.LEFT)
        
        # Control buttons
        btn_frame = ttk.Frame(control_frame)
        btn_frame.pack(fill=tk.X)
        
        ttk.Button(btn_frame, text="Capture Frame", command=self.capture_frame).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Toggle Stream", command=self.toggle_stream).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Open in Browser", command=self.open_browser).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Exit", command=self.root.quit).pack(side=tk.RIGHT, padx=2)
        
        # Log frame
        log_frame = ttk.LabelFrame(right_frame, text="System Log", padding="5")
        log_frame.pack(fill=tk.BOTH, expand=True)
        
        # Create text widget with scrollbar
        text_frame = ttk.Frame(log_frame)
        text_frame.pack(fill=tk.BOTH, expand=True)
        
        self.log_text = tk.Text(text_frame, height=8, state='disabled', wrap=tk.WORD)
        scrollbar = ttk.Scrollbar(text_frame, command=self.log_text.yview)
        self.log_text.configure(yscrollcommand=scrollbar.set)
        
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
    def log_message(self, message):
        """Add message to log console"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.configure(state='normal')
        self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)
        self.log_text.configure(state='disabled')
        
    def setup_mqtt(self):
        """Setup MQTT client"""
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_mqtt_connect
        self.mqtt_client.on_message = self.on_mqtt_message
        self.mqtt_client.on_disconnect = self.on_mqtt_disconnect
        
        # Start MQTT in background thread
        mqtt_thread = threading.Thread(target=self.connect_mqtt)
        mqtt_thread.daemon = True
        mqtt_thread.start()
        
    def connect_mqtt(self):
        """Connect to MQTT broker"""
        try:
            self.mqtt_client.connect(self.mqtt_broker, self.mqtt_port, 60)
            self.log_message(f"Connecting to MQTT broker at {self.mqtt_broker}")
            self.mqtt_client.loop_forever()
        except Exception as e:
            self.log_message(f"MQTT connection error: {str(e)}")
            
    def on_mqtt_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            self.connected = True
            self.log_message(f"Connected to MQTT broker")
            client.subscribe(self.topic_classification)
            client.subscribe(self.topic_status)
        else:
            self.log_message(f"MQTT connection failed with code {rc}")
            
    def on_mqtt_disconnect(self, client, userdata, rc):
        """MQTT disconnection callback"""
        self.connected = False
        self.log_message("Disconnected from MQTT broker")
        
    def on_mqtt_message(self, client, userdata, msg):
        """MQTT message callback"""
        try:
            payload = msg.payload.decode('utf-8')
            
            if msg.topic == self.topic_status:
                # Status message
                data = json.loads(payload)
                self.client_id = data.get('client_id', 'Unknown')
                ip = data.get('ip', 'Unknown')
                self.log_message(f"ESP32 connected: {self.client_id} ({ip})")
                
            elif msg.topic == self.topic_classification:
                # Classification message
                data = json.loads(payload)
                self.latest_classification = data
                
                # Update GUI from background thread
                self.root.after(0, self.update_classification_display)
                
        except json.JSONDecodeError as e:
            self.log_message(f"JSON decode error: {str(e)}")
        except Exception as e:
            self.log_message(f"Error processing message: {str(e)}")
            
    def connect_to_ip(self):
        """Connect to ESP32 IP address"""
        new_ip = self.ip_entry.get().strip()
        if new_ip:
            self.esp32_ip = new_ip
            self.stream_url = f"http://{new_ip}/stream"
            self.jpg_url = f"http://{new_ip}/jpg"
            
            self.log_message(f"Connecting to ESP32 at {new_ip}")
            self.ip_label.config(text=f"IP: {new_ip}")
            
            if not self.streaming:
                self.start_video_stream()
            else:
                self.log_message("Stream already running")
                
    def start_video_stream(self):
        """Start video streaming thread"""
        if not self.esp32_ip:
            self.log_message("No ESP32 IP specified")
            return
            
        self.streaming = True
        stream_thread = threading.Thread(target=self.video_stream_thread)
        stream_thread.daemon = True
        stream_thread.start()
        self.log_message(f"Started video stream from {self.esp32_ip}")
        
    def video_stream_thread(self):
        """Thread function for video streaming"""
        try:
            stream = requests.get(self.stream_url, stream=True, timeout=5)
            bytes_data = bytes()
            
            for chunk in stream.iter_content(chunk_size=1024):
                if not self.streaming:
                    break
                    
                bytes_data += chunk
                a = bytes_data.find(b'\xff\xd8')  # Start of JPEG
                b = bytes_data.find(b'\xff\xd9')  # End of JPEG
                
                if a != -1 and b != -1:
                    jpg = bytes_data[a:b+2]
                    bytes_data = bytes_data[b+2:]
                    
                    try:
                        # Decode image
                        img_array = np.frombuffer(jpg, dtype=np.uint8)
                        img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                        
                        if img is not None:
                            # Resize if too large
                            if img.shape[1] > 800:
                                scale = 800 / img.shape[1]
                                new_width = 800
                                new_height = int(img.shape[0] * scale)
                                img = cv2.resize(img, (new_width, new_height))
                            
                            # Convert BGR to RGB
                            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                            self.latest_frame = img
                            
                            # Add to queue if not full
                            if not self.frame_queue.full():
                                self.frame_queue.put(img)
                                
                    except Exception as e:
                        self.log_message(f"Image decode error: {str(e)}")
                        
        except requests.exceptions.RequestException as e:
            self.log_message(f"Stream error: {str(e)}")
            self.streaming = False
        except Exception as e:
            self.log_message(f"Stream thread error: {str(e)}")
            self.streaming = False
            
    def toggle_stream(self):
        """Toggle video stream on/off"""
        self.streaming = not self.streaming
        if self.streaming:
            self.log_message("Stream started")
            self.start_video_stream()
        else:
            self.log_message("Stream stopped")
            
    def capture_frame(self):
        """Capture single frame from ESP32"""
        if not self.esp32_ip:
            self.log_message("No ESP32 IP specified")
            return
            
        try:
            response = requests.get(self.jpg_url, timeout=5)
            if response.status_code == 200:
                img_array = np.frombuffer(response.content, dtype=np.uint8)
                img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                
                if img is not None:
                    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                    self.latest_frame = img
                    self.log_message("Frame captured successfully")
                    
                    # Save to file
                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    cv2.imwrite(f"capture_{timestamp}.jpg", cv2.cvtColor(img, cv2.COLOR_RGB2BGR))
                    self.log_message(f"Saved as capture_{timestamp}.jpg")
            else:
                self.log_message(f"Failed to capture frame: HTTP {response.status_code}")
        except Exception as e:
            self.log_message(f"Capture error: {str(e)}")
            
    def open_browser(self):
        """Open ESP32 web interface in browser"""
        if self.esp32_ip:
            import webbrowser
            webbrowser.open(f"http://{self.esp32_ip}")
            self.log_message(f"Opened browser to http://{self.esp32_ip}")
            
    def update_classification_display(self):
        """Update classification display"""
        if not self.latest_classification:
            return
            
        try:
            # Update status
            if self.connected:
                self.status_label.config(text="Connected", foreground="green")
            else:
                self.status_label.config(text="Disconnected", foreground="red")
                
            if self.client_id:
                self.client_label.config(text=f"Client: {self.client_id}")
                
            # Update classification info
            label = self.latest_classification.get('label', 'Unknown')
            confidence = self.latest_classification.get('confidence', 0) * 100
            timestamp = self.latest_classification.get('timestamp', 0)
            inference_time = self.latest_classification.get('inference_time', 0)
            
            # Set color based on confidence
            color = "green" if confidence > 70 else "orange" if confidence > 50 else "red"
            
            self.class_label.config(text=f"Class: {label}", foreground=color)
            self.confidence_label.config(text=f"Confidence: {confidence:.1f}%")
            self.inference_label.config(text=f"Inference time: {inference_time:.0f} ms")
            
            # Convert timestamp
            if timestamp:
                dt = datetime.fromtimestamp(timestamp / 1000)
                time_str = dt.strftime("%H:%M:%S")
                self.timestamp_label.config(text=f"Last update: {time_str}")
                
            # Update probability chart
            self.update_probability_chart()
            
        except Exception as e:
            self.log_message(f"Error updating display: {str(e)}")
            
    def update_probability_chart(self):
        """Draw probability distribution chart"""
        if not self.latest_classification or 'probabilities' not in self.latest_classification:
            return
            
        canvas = self.prob_canvas
        canvas.delete("all")
        
        probabilities = self.latest_classification['probabilities']
        if not probabilities:
            return
            
        # Sort by probability
        sorted_probs = sorted(probabilities, key=lambda x: x['value'], reverse=True)
        
        # Chart dimensions
        width = canvas.winfo_width()
        height = canvas.winfo_height()
        
        if width <= 1 or height <= 1:  # Canvas not yet sized
            width = 300
            height = 250
            
        margin = 30
        chart_width = width - 2 * margin
        chart_height = height - 2 * margin
        bar_width = chart_width / len(sorted_probs) * 0.8
        
        # Find max probability for scaling
        max_prob = max(p['value'] for p in sorted_probs)
        if max_prob == 0:
            max_prob = 1
            
        # Draw bars and labels
        for i, prob in enumerate(sorted_probs):
            value = prob['value']
            label = prob['label']
            bar_height = (value / max_prob) * (chart_height * 0.7)
            
            x1 = margin + i * (chart_width / len(sorted_probs))
            y1 = height - margin - bar_height
            x2 = x1 + bar_width
            y2 = height - margin
            
            # Draw bar with gradient color
            color = '#4CAF50' if i == 0 else '#2196F3'
            canvas.create_rectangle(x1, y1, x2, y2, fill=color, outline='black', width=1)
            
            # Draw value on top
            canvas.create_text(x1 + bar_width/2, y1 - 10, 
                              text=f"{value*100:.0f}%", font=('Arial', 9, 'bold'))
            
            # Draw label (rotated if space is limited)
            if len(sorted_probs) <= 6:
                # Draw rotated label
                canvas.create_text(x1 + bar_width/2, height - margin + 5, 
                                  text=label[:12], font=('Arial', 8), angle=45, anchor='nw')
            else:
                # Draw abbreviated label
                canvas.create_text(x1 + bar_width/2, height - margin + 5, 
                                  text=label[:8], font=('Arial', 7), anchor='n')
                
        # Draw title
        canvas.create_text(width/2, 15, text="Classification Probabilities", 
                          font=('Arial', 10, 'bold'))
        
        # Draw Y-axis label
        canvas.create_text(15, height/2, text="Probability", angle=90, font=('Arial', 9))
            
    def update_video_display(self):
        """Update video display with latest frame"""
        try:
            # Get frame from queue
            if not self.frame_queue.empty():
                frame = self.frame_queue.get()
                
                # Convert to PhotoImage
                img = Image.fromarray(frame)
                imgtk = ImageTk.PhotoImage(image=img)
                
                # Update label
                self.video_label.config(image=imgtk, text="")
                self.video_label.image = imgtk  # Keep reference
                
        except Exception as e:
            # Don't log every error to avoid spam
            pass
            
    def update_gui(self):
        """Periodic GUI update"""
        try:
            # Update video display
            self.update_video_display()
            
            # Update connection status
            if self.connected:
                self.status_label.config(text="Connected", foreground="green")
            else:
                self.status_label.config(text="Disconnected", foreground="red")
                
            # Update streaming status
            if self.streaming and self.esp32_ip:
                self.video_label.config(text="")
            elif not self.streaming and self.esp32_ip:
                self.video_label.config(text="Stream paused\n\nClick 'Toggle Stream' to start")
                
            # Schedule next update
            self.root.after(50, self.update_gui)  # ~20 FPS
            
        except Exception as e:
            self.log_message(f"GUI update error: {str(e)}")
            self.root.after(1000, self.update_gui)
            
    def run(self):
        """Start the application"""
        self.root.mainloop()
        
        # Cleanup
        self.streaming = False
        if self.mqtt_client:
            self.mqtt_client.disconnect()

# Simple command-line viewer (alternative)
class SimpleStreamViewer:
    """Simple OpenCV-based viewer"""
    def __init__(self, esp32_ip):
        self.esp32_ip = esp32_ip
        self.stream_url = f"http://{esp32_ip}/stream"
        self.mqtt_client = mqtt.Client()
        
    def start(self):
        """Start simple viewer"""
        # Connect to MQTT
        self.mqtt_client.on_connect = self.on_mqtt_connect
        self.mqtt_client.on_message = self.on_mqtt_message
        self.mqtt_client.connect("broker.hivemq.com", 1883, 60)
        self.mqtt_client.loop_start()
        
        print(f"Connecting to ESP32-CAM at {self.esp32_ip}")
        print("Press 'q' to quit, 'c' to capture frame")
        
        # Start video stream
        cap = cv2.VideoCapture(self.stream_url)
        
        if not cap.isOpened():
            print("Failed to open stream")
            return
            
        while True:
            ret, frame = cap.read()
            if not ret:
                print("Failed to grab frame")
                break
                
            # Display frame
            cv2.imshow('ESP32-CAM Live Stream', frame)
            
            # Handle keypress
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('c'):
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                cv2.imwrite(f"capture_{timestamp}.jpg", frame)
                print(f"Captured: capture_{timestamp}.jpg")
                
        cap.release()
        cv2.destroyAllWindows()
        self.mqtt_client.loop_stop()
        
    def on_mqtt_connect(self, client, userdata, flags, rc):
        print(f"Connected to MQTT with code {rc}")
        client.subscribe("esp32/cam/classification")
        
    def on_mqtt_message(self, client, userdata, msg):
        try:
            data = json.loads(msg.payload.decode())
            label = data.get('label', 'Unknown')
            confidence = data.get('confidence', 0) * 100
            print(f"\nClassification: {label} ({confidence:.1f}%)")
        except:
            pass

if __name__ == "__main__":
    print("=" * 60)
    print("ESP32-CAM Live Stream & Classification Viewer")
    print("=" * 60)
    
    print("\nOptions:")
    print("1. GUI Application (Recommended)")
    print("2. Simple OpenCV Viewer")
    
    choice = input("\nSelect option (1 or 2): ").strip()
    
    if choice == "1":
        # Get ESP32 IP
        default_ip = "192.168.1.100"  # Common default, change as needed
        ip_input = input(f"Enter ESP32 IP address [{default_ip}]: ").strip()
        esp32_ip = ip_input if ip_input else default_ip
        
        app = ESP32CamStreamViewer(esp32_ip)
        app.run()
        
    elif choice == "2":
        ip_input = input("Enter ESP32 IP address: ").strip()
        if ip_input:
            viewer = SimpleStreamViewer(ip_input)
            viewer.start()
        else:
            print("IP address required!")
    else:
        print("Invalid choice!")