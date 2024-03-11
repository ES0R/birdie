#!/usr/bin/python3

import socketserver
import threading
import signal

class ThreadedTCPRequestHandler(socketserver.BaseRequestHandler):
    """
    The request handler class for our server.
    
    It is instantiated once per connection to the server and must override the handle() method to implement
    communication with the client.
    """
    def handle(self):
        print(f"Connection from: {self.client_address}")
        while True:
            data = self.request.recv(1024).strip()
            if not data:
                print("Client disconnected")
                break
            message = data.decode('utf-8')
            print(f"Received: {message}")
            if message == 'quit':
                self.request.sendall(b'Goodbye\n')
                break
            elif message == 'shutdown':
                self.server.shutdown()
                self.server.server_close()
                print("Server is shutting down...")
                break
            else:
                response = f"Echo: {message}\n"
                self.request.sendall(response.encode('utf-8'))

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    """
    Inherits from ThreadingMixIn to make each request run in a separate thread.
    """
    allow_reuse_address = True
    daemon_threads = True

def signal_handler(sig, frame):
    print('Shutting down server...')
    server.shutdown()
    server.server_close()
    sys.exit(0)

if __name__ == "__main__":
    # Port 0 means to select an arbitrary unused port
    HOST, PORT = "localhost", 0

    server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
    ip, port = server.server_address

    # Start a thread with the server -- that thread will then start one
    # more thread for each request
    server_thread = threading.Thread(target=server.serve_forever)
    # Exit the server thread when the main thread terminates
    server_thread.daemon = True
    server_thread.start()

    print(f"Server loop running in thread: {server_thread.name}")
    print(f"Listening on {ip}:{port}")

    # Setup signal handler for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)

    # Keep the main thread running, otherwise signals are ignored.
    server_thread.join()
