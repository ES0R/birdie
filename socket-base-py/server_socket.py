#!/usr/bin/python3

import socketserver
import signal
import threading
import os
import cv2
import time
import cv2
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from golf_detection.model import Model
import math
import warnings

warnings.filterwarnings("ignore", category=UserWarning, module="numpy.core.getlimits", message="The value of the smallest subnormal for <class 'numpy.float64'> type is zero.")

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    stop = False
    

class MyTCPHandler(socketserver.StreamRequestHandler):
    def handle(self):
        print(f"Got new client from {self.client_address[0]}")
        # Initializes model for golf detection
        while not self.server.stop:
            try:
                data = self.rfile.readline().strip().decode('utf-8')
                if not data:
                    break  # Client disconnected
                command = data.split()
                print(f"Received Command: {command}")
                self.process_command(command)
            except ConnectionResetError:
                break  # Handle sudden client disconnection
        print(f"Client on {self.client_address[0]} disconnected")

    def send(self, msg):
        try:
            self.wfile.write(msg.encode('utf-8') + b"\r\n")
            self.wfile.flush()
        except BrokenPipeError:
            print("Client disconnected before message was sent.")

    def take_image(self):
        camera_stream_url = "http://10.197.216.166:8081"
        cap = cv2.VideoCapture(camera_stream_url, cv2.CAP_FFMPEG)
        if not cap.isOpened():
            return "Cannot open camera stream.", False
        
        ret, frame = cap.read()
        cap.release()

        if ret:
            # Ensure the directory exists
            image_dir = os.path.join(os.getcwd(), "captured_images")
            os.makedirs(image_dir, exist_ok=True)

            image_path = os.path.join('/home/local/svn/robobot/socket-base-py/captured_images', 'captured_image_test.jpg')
            if cv2.imwrite(image_path, frame):
                return frame,f"Image captured and saved at {image_path}", True
            else:
                return "Failed to write image to filesystem.", False
        else:
            return "Failed to capture image from the stream.", False
        
    def image_to_direction(self, image, save_image=True):
        original_height, original_width = image.shape[:2]  # Original image size
        resize_scale_width = original_width / 640.0  # Calculate the resize scale
        resize_scale_height = original_height / 640.0
        image_resized = cv2.resize(image, (640, 640))
        boxes, scores = model.run_inference(image_resized)
        img_rgb = cv2.cvtColor(image_resized, cv2.COLOR_BGR2RGB)

        fig, ax = plt.subplots()
        ax.imshow(img_rgb)

        camera_matrix = np.array([[1060.6, 0.0, 734.8], [0.0, 1064, 361.8], [0.0, 0.0, 1.0]])
        c_x = camera_matrix[0, 2]
        focal_length_x = camera_matrix[0, 0]
        W_real = 0.0427  # Real width of the golf ball
        distance = "-1"
        angle = "-1"
        for box, score in zip(boxes, scores):
            if score > 0.7:
                xmin, ymin, xmax, ymax = box
                # Scale bounding box coordinates back to original image size
                xmin_original = xmin * resize_scale_width
                xmax_original = xmax * resize_scale_width

                ax.add_patch(patches.Rectangle((xmin, ymin), xmax - xmin, ymax - ymin, linewidth=1, edgecolor='r', facecolor='none'))

                W_image_original = xmax_original - xmin_original  # Use original image width for distance calculation
                distance = (W_real * focal_length_x) / W_image_original

                object_center_x_original = xmin_original + W_image_original / 2
                dx = object_center_x_original - c_x  # Use original image center for angle calculation
                angle = math.degrees(math.atan2(dx, focal_length_x))

        if save_image:
            plt.savefig('/home/local/svn/robobot/socket-base-py/captured_images/output_test.png')
            plt.show()

        return str(distance) if distance else "-1", str(angle) if angle else "-1"

    def process_command(self, command):
        if command[0] == "help":
            self.send("Commands: quit, off, aruco, golf, help")
        elif command[0] == "aruco":
            self.send("Detected ArUco marker: ID 123 at position (x:100, y:200)")
        elif command[0] == "golf":
            image, message, success = self.take_image()
            distance, angle = self.image_to_direction(image,save_image=True) if success else None
            self.send(distance + "," + angle) 
        else:
            self.send("Unknown command")

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    server.stop = True

if __name__ == "__main__":
    model = Model('/home/local/svn/robobot/socket-base-py/golf_detection/models/best18/best_int8.tflite')

    PORT = 25005
    server = ThreadedTCPServer(("0.0.0.0", PORT), MyTCPHandler)
    print(f"Server starting on port {PORT}")
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
    signal.signal(signal.SIGINT, signal_handler)

    try:
        while not server.stop:
            time.sleep(0.1)
    finally:
        server.shutdown()
        print(f"Server on port {PORT} terminated")
