#!/usr/bin/python3

import socket

def main():
    # Server's IP address and port
    server_host = '127.0.0.1'  # Adjust as necessary, e.g., to the server's IP if testing over a network
    server_port = 25002  # Ensure this matches the port used by the server

    # Create a socket object
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # Connect to server
        sock.connect((server_host, server_port))
        print("Connected to server at {}:{}".format(server_host, server_port))

        # Sending a message to the server
        message_to_send = "golf"  # Change this to send different commands
        sock.sendall(message_to_send.encode('utf-8'))
        print("Sent:", message_to_send)

        # Receiving a response from the server
        response = sock.recv(1024).decode('utf-8')  # Adjust buffer size as necessary
        print("Received:", response)

    except Exception as e:
        print("An error occurred:", e)

    finally:
        # Closing the connection
        sock.close()
        print("Connection closed.")

if __name__ == "__main__":
    main()
