import tkinter as tk
from tkinter import ttk, scrolledtext
import serial
import serial.tools.list_ports
import threading
import json
import time
from PIL import Image, ImageTk
import io
import queue
import re

class ESP32CameraGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32-CAM Classification Monitor")
        self.root.geometry("1200x800")
        
        # Serial connection
        self.serial_port = None
        self.serial_connected = False
        self.data_queue = queue.Queue()
        
        # Current data
        self.current_label = "N/A"
        self.current_confidence = 0.0
        self.current_anomaly = 0.0
        self.current_timing = {"dsp": 0, "classification": 0}
        self.probabilities = []
        
        # Camera feed placeholder
        self.camera_image = None
        self.image_label = None
        
        self.setup_ui()
        
    def setup_ui(self):
        # Create main frames
        control_frame = ttk.Frame(self.root, padding="10")
        control_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        display_frame = ttk.Frame(self.root, padding="10")
        display_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        log_frame = ttk.Frame(self.root, padding="10")
        log_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.columnconfigure(1, weight=2)
        self.root.rowconfigure(0, weight=3)
        self.root.rowconfigure(1, weight=1)
        
        # Control Panel
        ttk.Label(control_frame, text="Serial Connection", font=("Arial", 12, "bold")).grid(row=0, column=0, columnspan=2, pady=5)
        
        # Port selection
        ttk.Label(control_frame, text="Port:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(control_frame, textvariable=self.port_var, width=25)
        self.port_combo.grid(row=1, column=1, pady=5)
        self.refresh_ports()
        
        # Baud rate
        ttk.Label(control_frame, text="Baud Rate:").grid(row=2, column=0, sticky=tk.W, pady=5)
        self.baud_var = tk.StringVar(value="115200")
        self.baud_combo = ttk.Combobox(control_frame, textvariable=self.baud_var, 
                                       values=["9600", "19200", "38400", "57600", "115200"], width=25)
        self.baud_combo.grid(row=2, column=1, pady=5)
        
        # Connect button
        self.connect_btn = ttk.Button(control_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=3, column=0, columnspan=2, pady=10)
        
        # Refresh button
        ttk.Button(control_frame, text="Refresh Ports", command=self.refresh_ports).grid(row=4, column=0, columnspan=2, pady=5)
        
        # Status indicator
        self.status_label = ttk.Label(control_frame, text="Status: Disconnected", foreground="red")
        self.status_label.grid(row=5, column=0, columnspan=2, pady=10)
        
        # Classification Results
        ttk.Separator(control_frame, orient="horizontal").grid(row=6, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        ttk.Label(control_frame, text="Classification Results", font=("Arial", 12, "bold")).grid(row=7, column=0, columnspan=2, pady=5)
        
        # Confidence gauge
        self.confidence_frame = ttk.LabelFrame(control_frame, text="Confidence Level", padding="10")
        self.confidence_frame.grid(row=8, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        
        self.confidence_var = tk.DoubleVar(value=0.0)
        self.confidence_bar = ttk.Progressbar(self.confidence_frame, length=200, 
                                              variable=self.confidence_var, maximum=1.0)
        self.confidence_bar.grid(row=0, column=0, pady=5)
        
        self.confidence_text = ttk.Label(self.confidence_frame, text="0.0%")
        self.confidence_text.grid(row=1, column=0)
        
        # Prediction label
        self.prediction_frame = ttk.LabelFrame(control_frame, text="Prediction", padding="10")
        self.prediction_frame.grid(row=9, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        
        self.prediction_label = ttk.Label(self.prediction_frame, text="N/A", font=("Arial", 16, "bold"))
        self.prediction_label.grid(row=0, column=0)
        
        # Timing info
        ttk.Label(control_frame, text="Processing Times:", font=("Arial", 10)).grid(row=10, column=0, columnspan=2, pady=(10, 0))
        self.timing_label = ttk.Label(control_frame, text="DSP: 0ms | Classification: 0ms")
        self.timing_label.grid(row=11, column=0, columnspan=2)
        
        # Display Panel (Camera Feed)
        ttk.Label(display_frame, text="Camera Feed", font=("Arial", 12, "bold")).grid(row=0, column=0, pady=5)
        
        # Image display
        self.image_canvas = tk.Canvas(display_frame, width=640, height=480, bg="black")
        self.image_canvas.grid(row=1, column=0, pady=10)
        
        # Placeholder text on canvas
        self.image_canvas.create_text(320, 240, text="Camera feed will appear here", 
                                      fill="white", font=("Arial", 14))
        
        # Probability bars
        ttk.Label(display_frame, text="Class Probabilities", font=("Arial", 12, "bold")).grid(row=2, column=0, pady=(20, 5))
        
        self.probability_canvas = tk.Canvas(display_frame, width=640, height=150, bg="white")
        self.probability_canvas.grid(row=3, column=0, pady=10)
        
        # Log Panel
        ttk.Label(log_frame, text="Serial Log", font=("Arial", 12, "bold")).grid(row=0, column=0, sticky=tk.W, pady=5)
        
        # Clear log button
        ttk.Button(log_frame, text="Clear Log", command=self.clear_log).grid(row=0, column=1, sticky=tk.E, pady=5)
        
        # Log text area
        self.log_text = scrolledtext.ScrolledText(log_frame, width=100, height=15, font=("Courier", 10))
        self.log_text.grid(row=1, column=0, columnspan=2, pady=10)
        self.log_text.configure(state='disabled')
        
        # Start update loop
        self.update_gui()
        
    def refresh_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports:
            self.port_combo.set(ports[0])
            
    def toggle_connection(self):
        if not self.serial_connected:
            self.connect_serial()
        else:
            self.disconnect_serial()
            
    def connect_serial(self):
        port = self.port_var.get()
        baud = int(self.baud_var.get())
        
        try:
            self.serial_port = serial.Serial(port, baud, timeout=1)
            self.serial_connected = True
            self.connect_btn.config(text="Disconnect")
            self.status_label.config(text="Status: Connected", foreground="green")
            
            # Start serial reading thread
            self.serial_thread = threading.Thread(target=self.read_serial, daemon=True)
            self.serial_thread.start()
            
            self.log_message(f"Connected to {port} at {baud} baud")
            
        except Exception as e:
            self.log_message(f"Connection failed: {str(e)}")
            
    def disconnect_serial(self):
        if self.serial_port:
            self.serial_port.close()
        self.serial_connected = False
        self.connect_btn.config(text="Connect")
        self.status_label.config(text="Status: Disconnected", foreground="red")
        self.log_message("Disconnected")
        
    def read_serial(self):
        buffer = ""
        while self.serial_connected:
            try:
                if self.serial_port.in_waiting:
                    line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Check for JSON result
                        if line.startswith("RESULT:"):
                            json_str = line[7:]  # Remove "RESULT:" prefix
                            try:
                                data = json.loads(json_str)
                                self.data_queue.put(('result', data))
                                self.log_message(f"Result: {data['label']} ({data['confidence']:.2%})")
                            except json.JSONDecodeError:
                                self.log_message(f"Failed to parse JSON: {json_str}")
                        else:
                            # Regular log message
                            self.data_queue.put(('log', line))
                            
            except Exception as e:
                if self.serial_connected:  # Only log if we're supposed to be connected
                    self.log_message(f"Serial read error: {str(e)}")
                break
                
    def log_message(self, message):
        timestamp = time.strftime("%H:%M:%S")
        formatted_message = f"[{timestamp}] {message}"
        
        self.data_queue.put(('ui_log', formatted_message))
        
    def clear_log(self):
        self.log_text.configure(state='normal')
        self.log_text.delete(1.0, tk.END)
        self.log_text.configure(state='disabled')
        
    def update_gui(self):
        # Process queued data
        while not self.data_queue.empty():
            data_type, data = self.data_queue.get()
            
            if data_type == 'result':
                self.process_result(data)
            elif data_type == 'log':
                self.display_log(data)
            elif data_type == 'ui_log':
                self.display_log(data)
                
        # Update UI elements
        self.confidence_bar['value'] = self.current_confidence
        self.confidence_text.config(text=f"{self.current_confidence:.1%}")
        self.prediction_label.config(text=self.current_label)
        
        # Color code prediction based on confidence
        if self.current_confidence > 0.7:
            self.prediction_label.config(foreground="green")
        elif self.current_confidence > 0.4:
            self.prediction_label.config(foreground="orange")
        else:
            self.prediction_label.config(foreground="red")
            
        self.timing_label.config(text=f"DSP: {self.current_timing['dsp']}ms | Classification: {self.current_timing['classification']}ms")
        
        # Update probability bars
        self.draw_probability_bars()
        
        # Schedule next update
        self.root.after(100, self.update_gui)
        
    def process_result(self, data):
        self.current_label = data.get('label', 'N/A')
        self.current_confidence = data.get('confidence', 0.0)
        self.current_anomaly = data.get('anomaly', 0.0)
        self.current_timing = data.get('timing', {'dsp': 0, 'classification': 0})
        self.probabilities = data.get('probabilities', [])
        
    def display_log(self, message):
        self.log_text.configure(state='normal')
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.see(tk.END)
        self.log_text.configure(state='disabled')
        
    def draw_probability_bars(self):
        self.probability_canvas.delete("all")
        
        if not self.probabilities:
            return
            
        max_prob = max(self.probabilities) if self.probabilities else 1.0
        bar_width = 600 / len(self.probabilities)
        
        for i, prob in enumerate(self.probabilities):
            x0 = i * bar_width + 20
            x1 = (i + 1) * bar_width + 15
            height = (prob / max_prob) * 100 if max_prob > 0 else 0
            
            # Draw bar
            color = "#4CAF50" if prob == max(self.probabilities) else "#2196F3"
            self.probability_canvas.create_rectangle(x0, 120 - height, x1, 120, fill=color, outline="black")
            
            # Draw label
            self.probability_canvas.create_text((x0 + x1) / 2, 130, text=f"{prob:.2f}", 
                                               font=("Arial", 8))
            self.probability_canvas.create_text((x0 + x1) / 2, 140, text=f"Class {i}", 
                                               font=("Arial", 8))

def main():
    root = tk.Tk()
    app = ESP32CameraGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()