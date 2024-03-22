#!/usr/bin/python3

import serial
import time
import threading
import os
import cv2
import signal

class SerialHandler:
    def __init__(self, port='/dev/ttyACM0', baudrate=9600):
        self.ser = serial.Serial(port, baudrate, timeout=1)
        self.stop = False

    def read_line(self):
        """
        Read a line from the serial port
        """
        try:
            return self.ser.readline().decode('utf-8').strip()
        except serial.SerialException:
            return None

    def send(self, msg):
        """
        Send a message to the serial port
        """
        self.ser.write((msg + "\r\n").encode('utf-8'))

    def handle(self):
        """
        Handle incoming commands from the serial port
        """
        while not self.stop:
            data = self.read_line()
            if data:
                command = data.split()
                print(f"Received Command: {command}")
                self.process_command(command)

    def process_command(self, command):
        """
        Process the received command
        """
        if command[0] == "help":
            self.send("Commands: quit, off, aruco, golf, help")
        elif command[0] == "aruco":
            self.send("Detected ArUco marker: ID 123 at position (x:100, y:200)")
        elif command[0] == "golf":
            message, success = self.take_image()
            self.send(message)  # Send success or failure message
        else:
            self.send("Unknown command")

    def take_image(self):
        """
        Take an image using the camera and save it
        """
        # Your image capturing logic remains the same
        # Return a tuple (message, success)
        return "Demo message", True

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    handler.stop = True

if __name__ == "__main__":
    handler = SerialHandler('/dev/ttyACM0', 9600)  # Adjust your serial port and baud rate as necessary
    signal.signal(signal.SIGINT, signal_handler)

    handler_thread = threading.Thread(target=handler.handle)
    handler_thread.start()
    handler_thread.join()

    print("Serial handler terminated")
