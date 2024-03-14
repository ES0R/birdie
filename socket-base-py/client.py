#!/usr/bin/python3

import socket

def send_command(host, port, command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        try:
            sock.connect((host, port))
            # Send the command.
            print(f"Sending command: {command}")
            sock.sendall(command.encode('utf-8') + b'\r\n')
            
            # Wait for and print the specific response to the command.
            response = sock.recv(1024).decode('utf-8')
            print(f"Received response: {response}")
        except ConnectionRefusedError:
            print(f"Connection refused. Is the server running on {host}:{port}?")
        except Exception as e:
            print(f"An error occurred: {e}")

if __name__ == "__main__":
    server_host = '127.0.0.1'
    server_port = 25004
    send_command(server_host, server_port, "golf")
    print("---")
