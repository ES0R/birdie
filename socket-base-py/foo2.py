#!/usr/bin/python3

# [License and copyright header]

import socketserver
import socket
import signal
import sys
import threading
import time
import select

## function to handle ctrl-C and reasonable shutdown
def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    server.stop_server()  # Added method to cleanly stop the server

class MyTCPHandler(socketserver.StreamRequestHandler):
    # [Request handler class as before]
    
    def handle(self):
        # [Handle method as before, no changes needed here]
        pass
    
    def send(self, msg):
        # [Send method as before, no changes needed here]
        pass

    def arucoRequest(self, gg):
        # [Placeholder for aruco request handling, as before]
        pass

    def golfRequest(self, gg):
        # [Placeholder for golf request handling, as before]
        pass

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True

class UServer:
    def __init__(self):
        self.stop = False
        self.port = 25001
        self.socserver = None

    def stop_server(self):
        self.stop = True
        if self.socserver:
            self.socserver.shutdown()  # Properly shutdown the server
            self.socserver.server_close()  # Close the server socket
    
    def run(self):
        # [Initialization for socserver and optional additional server instances]
        # Added proper checks and method calls to start and stop the server correctly.
        try:
            self.socserver = ThreadedTCPServer(("127.0.0.1", self.port), MyTCPHandler)
            # self.socserver = ThreadedTCPServer((socket.gethostname() + ".local", self.port), MyTCPHandler)
            server_thread = threading.Thread(target=self.socserver.serve_forever)
            server_thread.daemon = True
            server_thread.start()
            print("# Server starting on port " + str(self.port))
            
            # Additional server instances (localhost, etc.) initialization as before
            
            while not self.stop:
                time.sleep(0.2)
            
        finally:
            if self.socserver:
                self.socserver.shutdown()
                self.socserver.server_close()
            print("# Server on port " + str(self.port) + " terminated")

if __name__ == "__main__":
    server = UServer()
    signal.signal(signal.SIGINT, signal_handler)
    server.run()
