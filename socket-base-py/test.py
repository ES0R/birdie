#!/usr/bin/python3

import socketserver
import signal
import threading
import time
import select

# Function to handle ctrl-C and ensure a clean shutdown
def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    server.stop = True

# Request handler class for our server
class MyTCPHandler(socketserver.StreamRequestHandler):
    def handle(self):
        stop = False
        print("Got new client from {}".format(self.client_address[0]))
        self.send("Welcome to python vision server (send 'help' or 'quit' to quit)")
        poll_obj = select.poll()
        poll_obj.register(self.rfile, select.POLLIN)
        while not stop and not server.stop:
            poll_result = poll_obj.poll(100)
            if poll_result:
                self.data = self.rfile.readline().strip()
                try:
                    got = str(self.data, 'utf-8')
                except:
                    got = "invalid string"
                gg = got.split()
                if len(gg) > 0:
                    if gg[0] == "quit":
                        stop = True
                    elif gg[0] == "off":
                        self.send("Server shutdown")
                        stop = True
                        server.stop = True
                    elif gg[0] == "aruco" or gg[0] == "golf" or gg[0] == "help":
                        self.process_command(gg)
                    else:
                        self.send(f"Unknown command: {got}, try 'help' for a list of commands")
            else:
                time.sleep(0.05)
        print("Client on {} disconnected".format(self.client_address[0]))

    def send(self, msg):
        self.request.sendall(bytes(msg + "\r\n", "utf-8"))

    def process_command(self, gg):
        if gg[0] == "help":
            self.send("Commands: quit, off, aruco, golf, help")
        # Placeholder for processing aruco and golf commands

# Server class
class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True

class UServer:
    stop = False
    port = 25002
  
    def run(self):
        server_address = ("0.0.0.0", self.port)
        self.socserver = ThreadedTCPServer(server_address, MyTCPHandler)
        server_thread = threading.Thread(target=self.socserver.serve_forever)
        server_thread.daemon = True
        server_thread.start()
        print(f"# Server starting on port {self.port}")
        while not self.stop:
            time.sleep(0.2)
        self.socserver.shutdown()
        print(f"# Server on port {self.port} terminated")

server = UServer()

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    server.run()
