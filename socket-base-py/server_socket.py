#!/usr/bin/python3

import socketserver
import signal
import threading
import time
import cv2

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    stop = False

class MyTCPHandler(socketserver.StreamRequestHandler):
    def handle(self):
        print(f"Got new client from {self.client_address[0]}")
        while not self.server.stop:
            data = self.rfile.readline().strip().decode('utf-8')
            if not data:
                break  # Client disconnected
            command = data.split()
            if command:
                print(f"Received Command: {command}")
                if command[0] == "quit":
                    break
                elif command[0] == "off":
                    self.send("Server shutdown")
                    self.server.stop = True
                    break
                else:
                    self.process_command(command)
        print(f"Client on {self.client_address[0]} disconnected")

    def send(self, msg):
        self.wfile.write(msg.encode('utf-8') + b"\r\n")
        self.wfile.flush()

    def process_command(self, command):
        if command[0] == "help":
            self.send("Commands: quit, off, aruco, golf, help")
        elif command[0] == "aruco":
            self.send("Detected ArUco marker: ID 123 at position (x:100, y:200)")
        elif command[0] == "golf":
            try:
                self.send("Taking image...")

                camera_stream_url = "http://10.197.216.166:8081"
                cap = cv2.VideoCapture(camera_stream_url, cv2.CAP_FFMPEG)
                if not cap.isOpened():
                    print("Cannot open camera stream.")
                    exit()
                self.send("Taking image...")

                ret, frame = cap.read()
                if ret:
                    image_path = 'captured_image.jpg'
                    cv2.imwrite(image_path, frame)
                    print(f"Image captured and saved at {image_path}")
                else:
                    print("Failed to capture image from the stream.")

                cap.release()

                self.send("Golf command received. Image captured and saved.")
              
            except Exception as e:
                self.send(f"Error capturing image: {str(e)}")
        else:
            self.send("Unknown command")

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    server.stop = True

if __name__ == "__main__":
    PORT = 25004
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
