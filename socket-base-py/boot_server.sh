#!/bin/bash

# Path to your python server script
SERVER_SCRIPT="/home/local/svn/robobot/socket-base-py/server_socket.py"
DEFAULT_PORT=25002
PORT=${1:-$DEFAULT_PORT}

echo "Welcome to the Server Boot Script"

# Function to check if the port is available
check_port() {
    if lsof -i:$PORT &>/dev/null; then
        echo "Port $PORT is already in use. Please free up the port or choose a different one."
        return 1
    else
        echo "Port $PORT is available."
        return 0
    fi
}

# Prompt user to start the server
read -p "Do you want to start the server? (Y/n) " -n 1 -r
echo    # Move to a new line

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Exiting without starting the server."
    exit 1
fi

# Check if the port is available
if check_port; then
    echo "Starting the server on port $PORT..."
    python3 "$SERVER_SCRIPT"
else
    echo "Server could not be started due to port issues."
fi
