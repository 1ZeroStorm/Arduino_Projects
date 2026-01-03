import tkinter as tk
from tkinter import ttk, scrolledtext
import paho.mqtt.client as mqtt
import json
import threading
import time
from datetime import datetime
import queue

class ClassificationGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32-CAM Classification Monitor")
        self.root.geometry("1000x700")
        
        # MQTT Configuration
        self.broker = "broker.hivemq.com"
        self.port = 1883
        self.classification_topic = "esp32/cam/classification"
        self.status_topic = "esp32/cam/status"
        
        # Data storage
        self.current_classification = {
            "label": "N/A",
            "confidence": 0.0,
            "timestamp": 0,
            "client_id": "N/A",
            "probabilities": []
        }
        self.message_count = 0
        self.connected_devices = {}
        
        # Queues for thread-safe communication
        self.classification_queue = queue.Queue(maxsize=10)
        self.status_queue = queue.Queue(maxsize=10)
        self.log_queue = queue.Queue(maxsize=50)
        
        # MQTT client
        self.mqtt_client = None
        self.connected = False
        
        # Setup UI
        self.setup_ui()
        
        # Connect to MQTT
        self.connect_mqtt()
        
        # Start GUI update loop
        self.update_gui()
        
    def setup_ui(self):
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)
        
        # Header
        header_frame = ttk.Frame(main_frame)
        header_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        title = ttk.Label(header_frame, text="ESP32-CAM Real-time Classification", 
                         font=("Arial", 18, "bold"))
        title.pack(side=tk.LEFT)
        
        # Status indicator
        self.status_indicator = tk.Canvas(header_frame, width=20, height=20, bg="red")
        self.status_indicator.pack(side=tk.RIGHT, padx=10)
        self.status_indicator.create_oval(5, 5, 15, 15, fill="red")
        
        self.status_label = ttk.Label(header_frame, text="Disconnected", foreground="red")
        self.status_label.pack(side=tk.RIGHT)
        
        # Main content (notebook for tabs)
        notebook = ttk.Notebook(main_frame)
        notebook.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        
        # Tab 1: Classification Results
        classification_frame = ttk.Frame(notebook)
        notebook.add(classification_frame, text="Classification")
        
        # Configure classification frame grid
        classification_frame.columnconfigure(0, weight=1)
        classification_frame.columnconfigure(1, weight=1)
        classification_frame.rowconfigure(0, weight=1)
        classification_frame.rowconfigure(1, weight=1)
        
        # Left panel: Current classification
        current_frame = ttk.LabelFrame(classification_frame, text="Current Detection", padding="20")
        current_frame.grid(row=0, column=0, rowspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        current_frame.columnconfigure(0, weight=1)
        
        # Prediction label
        ttk.Label(current_frame, text="Detected Object:", font=("Arial", 14)).pack(pady=(0, 10))
        self.prediction_label = ttk.Label(current_frame, text="N/A", 
                                         font=("Arial", 24, "bold"), foreground="green")
        self.prediction_label.pack(pady=(0, 20))
        
        # Confidence
        ttk.Label(current_frame, text="Confidence Level:", font=("Arial", 12)).pack(pady=(0, 10))
        
        confidence_frame = ttk.Frame(current_frame)
        confidence_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.confidence_var = tk.DoubleVar(value=0.0)
        self.confidence_bar = ttk.Progressbar(confidence_frame, length=300, 
                                               variable=self.confidence_var, maximum=1.0)
        self.confidence_bar.pack(side=tk.LEFT, padx=(0, 10))
        
        self.confidence_text = ttk.Label(confidence_frame, text="0.0%", font=("Arial", 14))
        self.confidence_text.pack(side=tk.LEFT)
        
        # Inference time
        ttk.Label(current_frame, text="Inference Time:", font=("Arial", 12)).pack(pady=(0, 5))
        self.inference_label = ttk.Label(current_frame, text="0 ms", font=("Courier", 12))
        self.inference_label.pack()
        
        # Device info
        ttk.Label(current_frame, text="Device ID:", font=("Arial", 12)).pack(pady=(10, 5))
        self.device_label = ttk.Label(current_frame, text="N/A", font=("Courier", 10))
        self.device_label.pack()
        
        # Timestamp
        ttk.Label(current_frame, text="Last Update:", font=("Arial", 12)).pack(pady=(10, 5))
        self.timestamp_label = ttk.Label(current_frame, text="Never", font=("Courier", 10))
        self.timestamp_label.pack()
        
        # Right top: Probability bars
        prob_frame = ttk.LabelFrame(classification_frame, text="Class Probabilities", padding="15")
        prob_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        self.prob_canvas = tk.Canvas(prob_frame, bg="white")
        self.prob_canvas.pack(fill=tk.BOTH, expand=True)
        
        # Right bottom: Statistics
        stats_frame = ttk.LabelFrame(classification_frame, text="Statistics", padding="15")
        stats_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        stats_grid = ttk.Frame(stats_frame)
        stats_grid.pack(fill=tk.BOTH, expand=True)
        
        # Message count
        ttk.Label(stats_grid, text="Messages Received:", font=("Arial", 10)).grid(
            row=0, column=0, sticky=tk.W, pady=5, padx=5)
        self.count_label = ttk.Label(stats_grid, text="0", font=("Arial", 10, "bold"))
        self.count_label.grid(row=0, column=1, sticky=tk.W, pady=5, padx=20)
        
        # Connected devices
        ttk.Label(stats_grid, text="Connected Devices:", font=("Arial", 10)).grid(
            row=1, column=0, sticky=tk.W, pady=5, padx=5)
        self.devices_label = ttk.Label(stats_grid, text="0", font=("Arial", 10, "bold"))
        self.devices_label.grid(row=1, column=1, sticky=tk.W, pady=5, padx=20)
        
        # Update frequency
        ttk.Label(stats_grid, text="Update Frequency:", font=("Arial", 10)).grid(
            row=2, column=0, sticky=tk.W, pady=5, padx=5)
        self.freq_label = ttk.Label(stats_grid, text="0 Hz", font=("Arial", 10))
        self.freq_label.grid(row=2, column=1, sticky=tk.W, pady=5, padx=20)
        
        # Controls
        btn_frame = ttk.Frame(stats_frame)
        btn_frame.pack(fill=tk.X, pady=(10, 0))
        
        ttk.Button(btn_frame, text="Clear Stats", command=self.clear_stats).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="Reconnect", command=self.reconnect_mqtt).pack(side=tk.LEFT, padx=5)
        
        # Tab 2: Raw Data
        raw_frame = ttk.Frame(notebook)
        notebook.add(raw_frame, text="Raw Data")
        
        self.raw_text = scrolledtext.ScrolledText(raw_frame, font=("Courier", 10))
        self.raw_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        self.raw_text.configure(state='disabled')
        
        # Tab 3: Log
        log_frame = ttk.Frame(notebook)
        notebook.add(log_frame, text="Event Log")
        
        self.log_text = scrolledtext.ScrolledText(log_frame, font=("Courier", 9))
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        self.log_text.configure(state='disabled')
        
        # Footer
        footer_frame = ttk.Frame(main_frame)
        footer_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=(10, 0))
        
        ttk.Label(footer_frame, text=f"Broker: {self.broker}:{self.port} | Topic: {self.classification_topic}", 
                 font=("Arial", 9)).pack(side=tk.LEFT)
        
        # Frequency calculation
        self.last_update_time = time.time()
        self.update_count = 0
        
    # MQTT Callbacks
    def on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self.connected = True
            self.status_indicator.itemconfig(1, fill="green")
            self.status_label.config(text="Connected", foreground="green")
            self.log_message("Connected to MQTT broker")
            
            # Subscribe to topics
            client.subscribe(self.classification_topic)
            client.subscribe(self.status_topic)
            self.log_message(f"Subscribed to {self.classification_topic}")
            
        else:
            self.connected = False
            self.status_indicator.itemconfig(1, fill="red")
            self.status_label.config(text=f"Failed (rc={rc})", foreground="red")
            self.log_message(f"Connection failed with code: {rc}")
    
    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode('utf-8')
            data = json.loads(payload)
            
            if msg.topic == self.classification_topic:
                self.classification_queue.put(data)
                self.message_count += 1
                
                # Update device connection time
                client_id = data.get('client_id', 'unknown')
                self.connected_devices[client_id] = time.time()
                
            elif msg.topic == self.status_topic:
                self.status_queue.put(data)
                
        except json.JSONDecodeError:
            self.log_message("Invalid JSON received")
        except Exception as e:
            self.log_message(f"Error processing message: {e}")
    
    def on_disconnect(self, client, userdata, rc, properties=None):
        self.connected = False
        self.status_indicator.itemconfig(1, fill="red")
        self.status_label.config(text="Disconnected", foreground="red")
        self.log_message("Disconnected from MQTT broker")
    
    def connect_mqtt(self):
        """Connect to MQTT broker"""
        self.log_message(f"Connecting to {self.broker}:{self.port}...")
        
        try:
            self.mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, 
                                          client_id=f"gui-{int(time.time())}")
            self.mqtt_client.on_connect = self.on_connect
            self.mqtt_client.on_message = self.on_message
            self.mqtt_client.on_disconnect = self.on_disconnect
            
            self.mqtt_client.connect(self.broker, self.port, keepalive=60)
            self.mqtt_client.loop_start()
            
        except Exception as e:
            self.log_message(f"Connection error: {e}")
    
    def reconnect_mqtt(self):
        """Reconnect to MQTT broker"""
        self.log_message("Reconnecting to MQTT...")
        
        if self.mqtt_client:
            self.mqtt_client.disconnect()
            self.mqtt_client.loop_stop()
            time.sleep(1)
        
        self.connect_mqtt()
    
    def log_message(self, message):
        """Add message to log"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        formatted = f"[{timestamp}] {message}"
        self.log_queue.put(formatted)
    
    def clear_stats(self):
        """Clear statistics"""
        self.message_count = 0
        self.connected_devices = {}
        self.log_message("Statistics cleared")
    
    def update_probability_bars(self, probabilities):
        """Update probability bars on canvas"""
        self.prob_canvas.delete("all")
        
        if not probabilities:
            return
        
        # Get canvas dimensions
        canvas_width = self.prob_canvas.winfo_width()
        canvas_height = self.prob_canvas.winfo_height()
        
        if canvas_width < 10:
            return
        
        # Extract data
        labels = []
        values = []
        for prob in probabilities:
            if isinstance(prob, dict):
                labels.append(prob.get('label', ''))
                values.append(prob.get('value', 0))
            else:
                labels.append(f"Class {len(labels)}")
                values.append(float(prob))
        
        num_bars = len(values)
        bar_width = (canvas_width - 40) / num_bars
        max_val = max(values) if values else 1.0
        
        # Draw bars
        for i, (label, value) in enumerate(zip(labels, values)):
            x0 = i * bar_width + 20
            x1 = (i + 1) * bar_width + 15
            height = (value / max_val) * (canvas_height - 80) if max_val > 0 else 0
            
            # Bar color
            color = "#4CAF50" if value == max(values) else "#2196F3"
            
            # Draw bar
            self.prob_canvas.create_rectangle(x0, canvas_height - 60 - height,
                                               x1, canvas_height - 60,
                                               fill=color, outline="black")
            
            # Draw value
            self.prob_canvas.create_text((x0 + x1) / 2, canvas_height - 65 - height,
                                          text=f"{value:.2f}", font=("Arial", 8))
            
            # Draw label (rotated for readability)
            if len(label) > 8:
                label = label[:6] + "..."
            self.prob_canvas.create_text((x0 + x1) / 2, canvas_height - 40,
                                          text=label, font=("Arial", 8), angle=45)
    
    def update_gui(self):
        """Update GUI elements"""
        # Update statistics
        self.count_label.config(text=str(self.message_count))
        self.devices_label.config(text=str(len(self.connected_devices)))
        
        # Calculate update frequency
        current_time = time.time()
        self.update_count += 1
        
        if current_time - self.last_update_time >= 1.0:
            freq = self.update_count / (current_time - self.last_update_time)
            self.freq_label.config(text=f"{freq:.1f} Hz")
            self.last_update_time = current_time
            self.update_count = 0
        
        # Process classification queue
        try:
            while not self.classification_queue.empty():
                data = self.classification_queue.get_nowait()
                self.current_classification = data
                
                # Update UI
                label = data.get('label', 'N/A')
                confidence = data.get('confidence', 0.0)
                inference_time = data.get('inference_time', 0)
                client_id = data.get('client_id', 'N/A')
                timestamp = data.get('timestamp', 0)
                probabilities = data.get('probabilities', [])
                
                # Update prediction
                self.prediction_label.config(text=label)
                
                # Update confidence
                self.confidence_var.set(confidence)
                self.confidence_text.config(text=f"{confidence*100:.1f}%")
                
                # Color code based on confidence
                if confidence > 0.7:
                    self.prediction_label.config(foreground="green")
                elif confidence > 0.4:
                    self.prediction_label.config(foreground="orange")
                else:
                    self.prediction_label.config(foreground="red")
                
                # Update other info
                self.inference_label.config(text=f"{inference_time} ms")
                self.device_label.config(text=client_id)
                
                if timestamp:
                    dt = datetime.fromtimestamp(timestamp/1000)
                    self.timestamp_label.config(text=dt.strftime("%Y-%m-%d %H:%M:%S"))
                
                # Update probability bars
                self.update_probability_bars(probabilities)
                
                # Update raw data view
                self.raw_text.configure(state='normal')
                self.raw_text.delete(1.0, tk.END)
                self.raw_text.insert(tk.END, json.dumps(data, indent=2))
                self.raw_text.configure(state='disabled')
                
        except queue.Empty:
            pass
        
        # Process status queue
        try:
            while not self.status_queue.empty():
                data = self.status_queue.get_nowait()
                status = data.get('status', '')
                client_id = data.get('client_id', 'unknown')
                
                if status == 'connected':
                    self.log_message(f"Device connected: {client_id}")
                elif status == 'disconnected':
                    self.log_message(f"Device disconnected: {client_id}")
                    
        except queue.Empty:
            pass
        
        # Process log queue
        try:
            while not self.log_queue.empty():
                log_msg = self.log_queue.get_nowait()
                self.log_text.configure(state='normal')
                self.log_text.insert(tk.END, log_msg + "\n")
                self.log_text.see(tk.END)
                self.log_text.configure(state='disabled')
                
        except queue.Empty:
            pass
        
        # Schedule next update
        self.root.after(100, self.update_gui)
    
    def on_closing(self):
        """Clean shutdown"""
        if self.mqtt_client:
            self.mqtt_client.disconnect()
            self.mqtt_client.loop_stop()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = ClassificationGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()