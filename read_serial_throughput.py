import serial
import time
import sys

# Connect to serial port, send init command, then read data continuously and report throughput every second.


# Define the serial port and baud rate
port = "/dev/tty.usbmodemB5A865C91"
baud_rate = 9600

# Connect to the serial port
ser = serial.Serial(port, baud_rate)

# Send initialization command
ser.write(b"init_command")

# Initialize variables
start_time = time.time()
total_bytes = 0

# Continuously read data and calculate throughput
while True:
    # Read data from serial port
    data = ser.read(1024)

    # Calculate the number of bytes read
    num_bytes = len(data)

    # Update the total number of bytes
    total_bytes += num_bytes

    # Calculate the elapsed time
    elapsed_time = time.time() - start_time

    # Check if one second has passed
    if elapsed_time >= 1:
        # Calculate the throughput in bytes per second
        throughput = total_bytes / elapsed_time / 1024.0

        # Print the throughput
        print(f"Throughput: {throughput:.2f} KB/s")

        # Reset the variables
        start_time = time.time()
        total_bytes = 0
