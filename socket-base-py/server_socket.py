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
    


    def calculate_distances(self, image, corners,W_real, format = 'box'):
        
        #x_max and x_min 
        original_height, original_width = image.shape[:2]  # Original image size
        resize_scale_width = original_width / 640.0  # Calculate the resize scale

        xmin, ymin, xmax, ymax = corners
        
        xmin_original = xmin * resize_scale_width
        xmax_original = xmax * resize_scale_width
        
        # Camera matrix definition
        camera_matrix = np.array([[1060.6, 0.0, 734.8], [0.0, 1064, 361.8], [0.0, 0.0, 1.0]])
        c_x = camera_matrix[0, 2]
        focal_length_x = camera_matrix[0, 0]
        
        # DIstance calculation
        W_image_original = xmax_original - xmin_original  # Use original image width for distance calculation
        distance = (W_real * focal_length_x) / W_image_original
        distance = 0.88*distance-0.154 # funky conversion


        object_center_x_original = xmin_original + W_image_original / 2

        distance_to_middle = object_center_x_original-320

        return distance, distance_to_middle
  
    
    def scan_for_aruco(self, image):
        # Convert to grayscale
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        
        # Initialize the detector parameters using default values
        aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_250)
        
        # For versions that do not recognize DetectorParameters_create, use DetectorParameters
        parameters = cv2.aruco.DetectorParameters()

        detector = cv2.aruco.ArucoDetector(aruco_dict, parameters)

        corners, ids, rejectedImgPoints = detector.detectMarkers(gray)

        return corners, ids

    
    def image_to_direction_aruco_target(self, image, target_id, save_image=True):
        corners, ids = self.scan_for_aruco(image)

        W_real = 0.034  # Adjust based on the actual size of your ArUco marker

        if ids is not None and len(ids) > 0:
            # Flatten ids if necessary to ensure it's iterable
            ids = ids.flatten()

            selected_marker_info = None
            selected_corner = None

            for corner, id in zip(corners, ids):
                # Check if the current marker is the target marker
                if id == target_id:
                    # Calculate bounding box's corners
                    xmin, ymin, xmax, ymax = corner[0].min(axis=0)[0], corner[0].min(axis=0)[1], corner[0].max(axis=0)[0], corner[0].max(axis=0)[1]

                    # Calculate distance and displacement for the target marker
                    distance, distance_to_middle = self.calculate_distances(image, (xmin, ymin, xmax, ymax), W_real)
                    selected_marker_info = (id, distance, distance_to_middle)
                    selected_corner = corner
                    break  # Since we found our target marker, no need to check further

            # Visualization and saving of the target marker
            if save_image and selected_marker_info:
                img_with_selected_marker = cv2.aruco.drawDetectedMarkers(image.copy(), [selected_corner], np.array([[target_id]]))
                image_path = '/home/local/svn/robobot/socket-base-py/captured_images/aruco_target_detected.jpg'
                cv2.imwrite(image_path, img_with_selected_marker)
                return f"{selected_marker_info[0]}, {selected_marker_info[1]:.2f}, {selected_marker_info[2]:.2f}"
            elif not selected_marker_info:
                return "-2, -2, -2"  # Error code for target ID not within criteria or not found
        else:
            return "-1, -1, -1"  # Error code for no ArUco markers detected in the image

    
    def image_to_direction_aruco(self, image, save_image=True):
        corners, ids = self.scan_for_aruco(image)

        W_real = 0.034
        
        if ids is not None and len(ids) > 0:
            # Flatten ids if necessary to ensure it's iterable
            ids = ids.flatten()

            min_distance = float('inf')
            selected_marker_info = None
            selected_corner = None
            selected_id = None

            for corner, id in zip(corners, ids):
                # Calculate bounding box's corners
                xmin, ymin, xmax, ymax = corner[0].min(axis=0)[0], corner[0].min(axis=0)[1], corner[0].max(axis=0)[0], corner[0].max(axis=0)[1]
                
                # Calculate distance and displacement for each marker
                distance, distance_to_middle = self.calculate_distances(image, (xmin, ymin, xmax, ymax), W_real)

                if distance < min_distance and ymin > 440:
                    min_distance = distance
                    selected_corner = corner
                    selected_id = id
                    selected_marker_info = (id, distance, distance_to_middle)

            # Visualization and saving of the marker with the lowest distance
            if save_image and selected_marker_info:
                img_with_selected_marker = cv2.aruco.drawDetectedMarkers(image.copy(), [selected_corner], np.array([[selected_id]]))
                image_path = '/home/local/svn/robobot/socket-base-py/captured_images/aruco_closest_detected.jpg'
                cv2.imwrite(image_path, img_with_selected_marker)
                return f"{selected_marker_info[0]}, {selected_marker_info[1]:.2f}, {selected_marker_info[2]:.2f}"
            elif not selected_marker_info:
                return "-2, -2, -2" #Error code for no aruco within length limit
        else:
            return "-1, -1, -1" #Error code for not detected in image
    
    def image_to_direction_golf(self, image, save_image=True):

        image_resized = cv2.resize(image, (640, 640))
        boxes, scores = model.run_inference(image_resized)
        
        # Assuming W_real is the real width of the golf ball in meters
        W_real = 0.0427

        # Sort boxes by scores in descending order
        sorted_boxes = sorted(zip(boxes, scores), key=lambda x: x[1], reverse=True)

        detection = False
        for box, score in sorted_boxes:
            if score > 0.7:
                detection = True
                xmin, ymin, xmax, ymax = box
                # Use your calculate_distances function
                temp_distance, temp_distance_to_middle = self.calculate_distances(image, (xmin, ymin, xmax, ymax), W_real)
                
                # Check if the distance is within the desired range
                if temp_distance < 1.5:
                    distance, distance_to_middle = temp_distance, temp_distance_to_middle
                    break 
                # Found a suitable object, no need to check further
        # Visualization
        if save_image:
            img_rgb = cv2.cvtColor(image_resized, cv2.COLOR_BGR2RGB)
            fig, ax = plt.subplots()
            ax.imshow(img_rgb)
            for box, score in zip(boxes, scores):
                if score > 0.7:
                    xmin, ymin, xmax, ymax = box
                    ax.add_patch(patches.Rectangle((xmin, ymin), xmax - xmin, ymax - ymin, linewidth=1, edgecolor='r', facecolor='none'))
            plt.savefig('/home/local/svn/robobot/socket-base-py/captured_images/golf.png')
            plt.close(fig)

        if detection:
            return f"{distance:.2f}, {distance_to_middle:.2f}"
        else:
            print("No golf ball found")
            return "-1, -1"


    def process_command(self, command):
        if command[0] == "help":
            self.send("Commands: quit, off, aruco, golf, help")
        elif command[0] == "aruco":
            image, message, success = self.take_image()  # Adjusted to unpack three values
            data = self.image_to_direction_aruco(image,save_image=True) if success else None
            self.send(data)
        elif command[0] == "golf":
            image, message, success = self.take_image()  # Adjusted to unpack three values
            data = self.image_to_direction_golf(image,save_image=True) if success else None
            self.send(data) 
        elif command[0] == "arcuo_target":
            image, message, success = self.take_image()
            data = self.image_to_direction_aruco_target(image,save_image=True) if success else None
            self.send(data)
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
