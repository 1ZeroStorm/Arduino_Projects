"""
Simple ESP32-CAM Viewer (No matplotlib required)
"""

import tkinter as tk
from tkinter import ttk, scrolledtext
import paho.mqtt.client as mqtt
import json
from datetime import datetime
import threading

class SimpleViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32-CAM Viewer")
        self.root.geometry("900x700")
        
        # Colors for confidence levels
        self.colors = {
            'high': '#2ecc71',    # Green
            'medium': '#f39c12',  # Orange
            'low': '#e74c3c'      # Red
        }
        
        # MQTT
        self.mqtt_broker = "broker.hivemq.com"
        self.mqtt_port = 1883
        self.topic = "esp32/cam/classification"
        
        # Setup UI
        self.setup_ui()
        
        # Connect to MQTT
        self.connect_mqtt()
        
    def setup_ui(self):
        # Configure style
        style = ttk.Style()
        style.configure('Title.TLabel', font=('Helvetica', 18, 'bold'))
        style.configure('Result.TLabel', font=('Helvetica', 24, 'bold'))
        style.configure('Confidence.Horizontal.TProgressbar', 
                       background=self.colors['medium'])
        
        # Create main container
        main_container = ttk.Frame(self.root, padding="20")
        main_container.pack(fill=tk.BOTH, expand=True)
        
        # Header
        header_frame = ttk.Frame(main_container)
        header_frame.pack(fill=tk.X, pady=(0, 20))
        
        ttk.Label(header_frame, text="ESP32-CAM AI Classification", 
                 style='Title.TLabel').pack(side=tk.LEFT)
        
        self.status_label = ttk.Label(header_frame, text="● Disconnected", 
                                     foreground="red")
        self.status_label.pack(side=tk.RIGHT, padx=10)
        
        # Main content in two columns
        content_frame = ttk.Frame(main_container)
        content_frame.pack(fill=tk.BOTH, expand=True)
        
        # Left column - Current result
        left_frame = ttk.Frame(content_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 10))
        
        # Result card
        result_card = ttk.LabelFrame(left_frame, text="Current Detection", padding="25")
        result_card.pack(fill=tk.BOTH, expand=True)
        
        self.result_label = ttk.Label(result_card, text="WAITING FOR DATA", 
                                     style='Result.TLabel', foreground="gray")
        self.result_label.pack(pady=30)
        
        # Confidence
        confidence_frame = ttk.Frame(result_card)
        confidence_frame.pack(fill=tk.X, pady=20)
        
        ttk.Label(confidence_frame, text="Confidence:").pack(side=tk.LEFT)
        
        self.confidence_bar = ttk.Progressbar(confidence_frame, 
                                            style='Confidence.Horizontal.TProgressbar',
                                            length=300)
        self.confidence_bar.pack(side=tk.LEFT, padx=10, expand=True, fill=tk.X)
        
        self.confidence_text = ttk.Label(confidence_frame, text="0%", 
                                        font=('Helvetica', 14))
        self.confidence_text.pack(side=tk.RIGHT)
        
        # Inference info
        info_frame = ttk.Frame(result_card)
        info_frame.pack(fill=tk.X, pady=10)
        
        self.time_label = ttk.Label(info_frame, text="Inference time: -- ms")
        self.time_label.pack(side=tk.LEFT)
        
        self.timestamp_label = ttk.Label(info_frame, text="Last update: --")
        self.timestamp_label.pack(side=tk.RIGHT)
        
        # Right column - History and details
        right_frame = ttk.Frame(content_frame)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # History card
        history_card = ttk.LabelFrame(right_frame, text="Recent Detections", padding="15")
        history_card.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # Create treeview for history
        columns = ('Time', 'Label', 'Confidence')
        self.history_tree = ttk.Treeview(history_card, columns=columns, 
                                        show='headings', height=8)
        
        for col in columns:
            self.history_tree.heading(col, text=col)
            self.history_tree.column(col, width=100)
        
        self.history_tree.pack(fill=tk.BOTH, expand=True)
        
        # Scrollbar for treeview
        scrollbar = ttk.Scrollbar(history_card, orient="vertical", 
                                 command=self.history_tree.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.history_tree.configure(yscrollcommand=scrollbar.set)
        
        # Details card
        details_card = ttk.LabelFrame(right_frame, text="Detection Details", padding="15")
        details_card.pack(fill=tk.BOTH, expand=True)
        
        # Create text widget for probabilities
        self.details_text = scrolledtext.ScrolledText(details_card, height=10, 
                                                     state='disabled')
        self.details_text.pack(fill=tk.BOTH, expand=True)
        
        # Log card at bottom
        log_card = ttk.LabelFrame(main_container, text="System Log", padding="10")
        log_card.pack(fill=tk.X, pady=(20, 0))
        
        self.log_text = scrolledtext.ScrolledText(log_card, height=4, state='disabled')
        self.log_text.pack(fill=tk.BOTH)
        
    def connect_mqtt(self):
        self.log("Connecting to MQTT broker...")
        
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        
        # Start in background thread
        thread = threading.Thread(target=self.mqtt_thread, daemon=True)
        thread.start()
        
    def mqtt_thread(self):
        try:
            self.mqtt_client.connect(self.mqtt_broker, self.mqtt_port, 60)
            self.mqtt_client.loop_forever()
        except Exception as e:
            self.root.after(0, self.log, f"MQTT error: {str(e)}")
            
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.root.after(0, self.update_status, True)
            client.subscribe(self.topic)
            self.root.after(0, self.log, "Connected to MQTT broker")
        else:
            self.root.after(0, self.update_status, False)
            self.root.after(0, self.log, f"Connection failed with code {rc}")
            
    def on_message(self, client, userdata, msg):
        try:
            data = json.loads(msg.payload.decode())
            self.root.after(0, self.update_display, data)
        except Exception as e:
            self.root.after(0, self.log, f"Error processing message: {str(e)}")
            
    def update_status(self, connected):
        if connected:
            self.status_label.config(text="● Connected", foreground="green")
        else:
            self.status_label.config(text="● Disconnected", foreground="red")
            
    def update_display(self, data):
        # Extract data
        label = data.get('label', 'Unknown')
        confidence = data.get('confidence', 0) * 100
        inference_time = data.get('inference_time', 0)
        timestamp = data.get('timestamp', 0)
        
        # Update main result
        self.result_label.config(text=label.upper())
        
        # Update confidence bar
        self.confidence_bar['value'] = confidence
        self.confidence_text.config(text=f"{confidence:.1f}%")
        
        # Update confidence bar color
        if confidence > 70:
            color = self.colors['high']
        elif confidence > 50:
            color = self.colors['medium']
        else:
            color = self.colors['low']
        
        # Update inference info
        self.time_label.config(text=f"Inference time: {inference_time:.0f} ms")
        
        # Convert timestamp to readable time
        if timestamp:
            dt = datetime.fromtimestamp(timestamp / 1000)
            time_str = dt.strftime("%H:%M:%S")
            self.timestamp_label.config(text=f"Last update: {time_str}")
        
        # Add to history
        current_time = datetime.now().strftime("%H:%M:%S")
        self.history_tree.insert('', 0, values=(current_time, label, f"{confidence:.1f}%"))
        
        # Keep only last 10 entries
        if len(self.history_tree.get_children()) > 10:
            self.history_tree.delete(self.history_tree.get_children()[-1])
        
        # Update details
        self.details_text.configure(state='normal')
        self.details_text.delete(1.0, tk.END)
        
        if 'probabilities' in data:
            self.details_text.insert(tk.END, "Probability Distribution:\n\n")
            for prob in data['probabilities']:
                value = prob['value'] * 100
                self.details_text.insert(tk.END, 
                                       f"  {prob['label']}: {value:.1f}%\n")
        
        self.details_text.configure(state='disabled')
        
        # Log
        self.log(f"Detected: {label} ({confidence:.1f}%)")
        
    def log(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.configure(state='normal')
        self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)
        self.log_text.configure(state='disabled')
        
    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    root = tk.Tk()
    app = SimpleViewer(root)
    app.run()