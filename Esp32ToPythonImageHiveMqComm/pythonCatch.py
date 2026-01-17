import paho.mqtt.client as mqtt
import base64
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import threading
import queue
import time
import io

class MQTTImageReceiver:
    def __init__(self):
        # MQTT Configuration
        self.broker = "broker.hivemq.com"
        self.port = 1883
        self.topic = "test/esp32_to_python"
        
        # Image processing variables
        self.image_data = ""
        self.image_size = 0
        self.received_chunks = 0
        self.total_chunks = 0
        self.is_receiving = False
        self.chunks_received = {}
        
        # Queue for thread-safe communication
        self.image_queue = queue.Queue()
        
        # MQTT Client
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
        # Tkinter window
        self.root = tk.Tk()
        self.root.title("ESP32-CAM Image Receiver")
        self.root.geometry("640x520")
        
        # Store current displayed image
        self.current_image = None
        
        self.setup_ui()
        
    def setup_ui(self):
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        main_frame.columnconfigure(0, weight=1)
        
        # Status label
        self.status_label = ttk.Label(main_frame, text="Disconnected", font=("Arial", 10))
        self.status_label.grid(row=0, column=0, columnspan=2, pady=(0, 10), sticky=tk.W)
        
        # Image frame with border
        img_frame = ttk.LabelFrame(main_frame, text="Camera View", padding="5")
        img_frame.grid(row=1, column=0, columnspan=2, pady=(0, 10), sticky=(tk.W, tk.E, tk.N, tk.S))
        img_frame.columnconfigure(0, weight=1)
        img_frame.rowconfigure(0, weight=1)
        
        # Create a canvas for image display
        self.canvas = tk.Canvas(img_frame, width=600, height=400, bg="lightgray", 
                               highlightthickness=1, highlightbackground="black")
        self.canvas.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Text on canvas (will be removed when image is displayed)
        self.canvas_text = self.canvas.create_text(300, 200, text="No image received", font=("Arial", 12))
        
        # Progress bar
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(main_frame, length=400, 
                                           variable=self.progress_var, 
                                           maximum=100)
        self.progress_bar.grid(row=2, column=0, columnspan=2, pady=(0, 10), sticky=tk.W+tk.E)
        
        # Stats frame
        stats_frame = ttk.Frame(main_frame)
        stats_frame.grid(row=3, column=0, columnspan=2, pady=(0, 10), sticky=(tk.W, tk.E))
        
        # Statistics text widget
        self.stats_text = tk.Text(stats_frame, height=4, width=60, font=("Courier", 9))
        self.stats_text.grid(row=0, column=0, sticky=(tk.W, tk.E))
        self.stats_text.insert("1.0", "Status: Waiting for connection\n")
        self.stats_text.insert("2.0", "Images received: 0\n")
        self.stats_text.insert("3.0", "Last received: Never\n")
        self.stats_text.insert("4.0", "Current size: 0 bytes")
        self.stats_text.config(state="disabled")
        
        # Buttons frame
        buttons_frame = ttk.Frame(main_frame)
        buttons_frame.grid(row=4, column=0, columnspan=2, pady=(10, 0))
        
        # Control buttons
        self.connect_button = ttk.Button(buttons_frame, text="Connect", 
                                        command=self.toggle_connection)
        self.connect_button.grid(row=0, column=0, padx=5)
        
        self.clear_button = ttk.Button(buttons_frame, text="Clear Image", 
                                      command=self.clear_image)
        self.clear_button.grid(row=0, column=1, padx=5)
        
        # Images counter
        self.images_received = 0
        self.last_received_time = "Never"
        
    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            print("✅ Connected to MQTT Broker!")
            client.subscribe(self.topic)
            self.root.after(0, self.update_status, "Connected to MQTT", "green")
        else:
            print(f"❌ Failed to connect, return code {reason_code}")
            self.root.after(0, self.update_status, f"Connection failed: {reason_code}", "red")
    
    def on_message(self, client, userdata, message):
        try:
            payload = message.payload.decode('utf-8')
            
            if payload.startswith("IMG_START:"):
                # Start receiving new image
                self.is_receiving = True
                self.image_data = ""
                self.chunks_received = {}
                
                parts = payload.split(":")
                if len(parts) >= 3:
                    self.image_size = int(parts[1])
                    self.total_chunks = int(parts[2])
                else:
                    self.image_size = 0
                    self.total_chunks = 0
                    
                self.received_chunks = 0
                print(f"📨 Starting to receive image. Expected chunks: {self.total_chunks}")
                
                self.root.after(0, self.update_status, "Receiving image...", "orange")
                self.root.after(0, self.update_progress, 0)
                
            elif payload.startswith("IMG_CHUNK:") and self.is_receiving:
                # Receive image chunk
                parts = payload.split(":", 2)
                if len(parts) == 3:
                    chunk_index = int(parts[1])
                    chunk_data = parts[2]
                    
                    # Store chunk by index (in case they arrive out of order)
                    self.chunks_received[chunk_index] = chunk_data
                    self.received_chunks = len(self.chunks_received)
                    
                    # Update progress
                    if self.total_chunks > 0:
                        progress = (self.received_chunks / self.total_chunks) * 100
                    else:
                        progress = 0
                        
                    self.root.after(0, self.update_progress, progress)
                    
                    self.root.after(0, self.update_stats, 
                                  f"Status: Received {self.received_chunks}/{self.total_chunks} chunks\n"
                                  f"Images received: {self.images_received}\n"
                                  f"Last received: {self.last_received_time}\n"
                                  f"Chunk {chunk_index}: {len(chunk_data)} chars")
                    
            elif payload == "IMG_END" and self.is_receiving:
                # Image complete - assemble all chunks
                self.is_receiving = False
                
                # Assemble chunks in order
                self.image_data = ""
                for i in range(self.total_chunks):
                    if i in self.chunks_received:
                        self.image_data += self.chunks_received[i]
                    else:
                        print(f"⚠️ Missing chunk {i}")
                
                print(f"📨 Image complete. Base64 length: {len(self.image_data)}")
                
                if len(self.image_data) > 0:
                    # Process image in a separate thread
                    threading.Thread(target=self.process_image, 
                                   args=(self.image_data,), 
                                   daemon=True).start()
                    
                    self.root.after(0, self.update_progress, 100)
                    self.root.after(0, self.update_status, "Processing image...", "blue")
                else:
                    print("❌ No image data received")
                    self.root.after(0, self.update_status, "No image data", "red")
                
        except Exception as e:
            print(f"❌ Error processing MQTT message: {e}")
            
    def process_image(self, base64_data):
        try:
            # Clean base64 data (remove any whitespace or newlines)
            clean_data = base64_data.replace('\n', '').replace('\r', '').replace(' ', '')
            
            # Check if we have enough data
            if len(clean_data) < 100:
                print(f"❌ Base64 data too short: {len(clean_data)} chars")
                self.root.after(0, self.update_status, "Image data too short", "red")
                return
            
            print(f"🔍 Processing {len(clean_data)} chars of base64 data")
            
            # Add padding if needed
            missing_padding = len(clean_data) % 4
            if missing_padding:
                clean_data += '=' * (4 - missing_padding)
            
            # Decode base64 to bytes
            try:
                image_bytes = base64.b64decode(clean_data, validate=True)
            except Exception as e:
                print(f"❌ Base64 decode error: {e}")
                # Try alternative approach - decode ignoring errors
                image_bytes = base64.b64decode(clean_data, validate=False)
            
            print(f"✅ Decoded image size: {len(image_bytes)} bytes")
            
            if len(image_bytes) < 100:
                print(f"❌ Decoded image too small: {len(image_bytes)} bytes")
                self.root.after(0, self.update_status, "Image too small", "red")
                return
            
            # Save raw bytes to file for debugging
            with open("debug_image.jpg", "wb") as f:
                f.write(image_bytes)
            print("💾 Saved debug image to debug_image.jpg")
            
            # Try to open as image
            try:
                image = Image.open(io.BytesIO(image_bytes))
                print(f"✅ Image opened successfully: {image.format}, Size: {image.size}")
                
                # Verify it's a valid image
                image.verify()  # Verify integrity
                image = Image.open(io.BytesIO(image_bytes))  # Re-open after verify
                
                # Put image in queue for main thread
                self.image_queue.put(image)
                
                # Update statistics
                self.images_received += 1
                self.last_received_time = time.strftime("%H:%M:%S")
                
                # Schedule UI update in main thread
                self.root.after(0, self.display_image_from_queue)
                
            except Exception as e:
                print(f"❌ Error opening image: {e}")
                self.root.after(0, self.update_status, f"Image error: {str(e)[:30]}", "red")
                
        except Exception as e:
            print(f"❌ Error in process_image: {e}")
            self.root.after(0, self.update_status, f"Processing error", "red")
    
    def display_image_from_queue(self):
        try:
            # Get image from queue
            image = self.image_queue.get_nowait()
            
            # Calculate display size
            canvas_width = 600
            canvas_height = 400
            
            # Calculate new size maintaining aspect ratio
            original_width, original_height = image.size
            ratio = min((canvas_width - 20) / original_width, (canvas_height - 20) / original_height)
            new_size = (int(original_width * ratio), int(original_height * ratio))
            
            # Resize if needed
            if new_size != image.size:
                image = image.resize(new_size, Image.Resampling.LANCZOS)
            
            # Convert to PhotoImage for Tkinter
            photo = ImageTk.PhotoImage(image)
            
            # Clear canvas and display new image
            self.canvas.delete("all")
            self.canvas.create_image(canvas_width // 2, canvas_height // 2, 
                                    image=photo, anchor=tk.CENTER)
            
            # Keep reference to prevent garbage collection
            self.current_image = photo
            
            # Update status and stats
            self.root.after(0, self.update_status, f"Image {self.images_received} received!", "green")
            self.root.after(0, self.update_stats, 
                          f"Status: Image {self.images_received} displayed\n"
                          f"Images received: {self.images_received}\n"
                          f"Last received: {self.last_received_time}\n"
                          f"Image size: {image.size[0]}x{image.size[1]}")
            
            print(f"✅ Image {self.images_received} displayed: {image.size}")
            
        except queue.Empty:
            pass
        except Exception as e:
            print(f"❌ Error displaying image: {e}")
    
    def update_status(self, text, color="black"):
        self.status_label.config(text=text, foreground=color)
    
    def update_progress(self, value):
        self.progress_var.set(value)
    
    def update_stats(self, text):
        self.stats_text.config(state="normal")
        self.stats_text.delete("1.0", tk.END)
        self.stats_text.insert("1.0", text)
        self.stats_text.config(state="disabled")
    
    def toggle_connection(self):
        if not self.client.is_connected():
            # Connect in separate thread
            threading.Thread(target=self.connect_mqtt, daemon=True).start()
            self.connect_button.config(text="Disconnect")
        else:
            self.client.disconnect()
            self.connect_button.config(text="Connect")
            self.update_status("Disconnected", "red")
            self.update_progress(0)
    
    def connect_mqtt(self):
        try:
            self.update_status("Connecting...", "orange")
            print(f"🔗 Connecting to {self.broker}:{self.port}...")
            self.client.connect(self.broker, self.port, 60)
            print("🚀 Starting MQTT loop...")
            self.client.loop_forever()
        except Exception as e:
            print(f"❌ Connection error: {e}")
            self.root.after(0, self.update_status, f"Error: {str(e)}", "red")
            self.root.after(0, lambda: self.connect_button.config(text="Connect"))
    
    def clear_image(self):
        self.canvas.delete("all")
        self.canvas.create_text(300, 200, text="No image received", font=("Arial", 12))
        self.current_image = None
        self.update_status("Image cleared", "black")
        self.update_progress(0)
    
    def run(self):
        # Start MQTT connection automatically
        threading.Thread(target=self.connect_mqtt, daemon=True).start()
        
        # Start the Tkinter main loop
        self.root.mainloop()

if __name__ == "__main__":
    print("🚀 Starting ESP32-CAM Image Receiver...")
    print("📡 Subscribed to: test/esp32_to_python")
    print("🎯 Waiting for images...")
    print("Press Ctrl+C to exit")
    
    app = MQTTImageReceiver()
    app.run()