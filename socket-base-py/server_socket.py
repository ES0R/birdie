#!/usr/bin/python3

import socketserver
import signal
import threading
import time
import select

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    server.stop = True
    
class MyTCPHandler(socketserver.StreamRequestHandler):
    def handle(self):
        print(f"Got new client from {self.client_address[0]}")
        poll_obj = select.poll()
        poll_obj.register(self.rfile, select.POLLIN)
        while not server.stop:
            poll_result = poll_obj.poll(100)
            if poll_result:
                data = self.rfile.readline().strip().decode('utf-8')
                command = data.split()
                if command:
                    if command[0] == "quit":
                        break
                    elif command[0] == "off":
                        self.send("Server shutdown")
                        server.stop = True
                        break
                    else:
                        self.process_command(command)
            else:
                time.sleep(0.05)
        print(f"Client on {self.client_address[0]} disconnected")

    def send(self, msg):
        self.request.sendall(msg.encode('utf-8') + b"\r\n")

    def process_command(self, command):
        if command[0] == "help":
            self.send("Commands: quit, off, aruco, golf, help")
        elif command[0] == "aruco":
            # Placeholder for processing the 'aruco' command
            # Simulate detecting an ArUco marker with ID 123 at position (x, y)
            self.send("Detected ArUco marker: ID 123 at position (x:100, y:200)")
        elif command[0] == "golf":
            # Placeholder for processing the 'golf' command
            # Simulate detecting a golf ball at position (x, y)
            self.send("Detected golf ball at position (x:150, y:250)")
        else:
            self.send("Unknown command")



class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True

class UServer:
    def __init__(self, port=25003):
        self.port = port
        self.stop = False

    def run(self):
        address = ("0.0.0.0", self.port)
        with ThreadedTCPServer(address, MyTCPHandler) as server:
            print(f"Server starting on port {self.port}")
            server_thread = threading.Thread(target=server.serve_forever)
            server_thread.daemon = True
            server_thread.start()
            try:
                while not self.stop:
                    time.sleep(0.2)
            finally:
                server.shutdown()
                print(f"Server on port {self.port} terminated")

if __name__ == "__main__":
    server = UServer()
    signal.signal(signal.SIGINT, signal_handler)
    server.run()
