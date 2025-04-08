import os
import socket
import subprocess
import struct

# Configuration
STREAM_NAME = "kvs-bridge-stream" # Kinesis Video Stream name

HOST = '0.0.0.0'
PORT = 5000

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(1)
conn, addr = server.accept()
print(f"Connection from {addr}")

# Fetch AWS credentials
assert os.environ.get("AWS_ACCESS_KEY_ID") and os.environ.get("AWS_SECRET_ACCESS_KEY"), "Missing AWS credentials"

# GStreamer subprocess command
gst_cmd = [
    "gst-launch-1.0", "-v",
    "fdsrc", "!",  # Feed from stdin
    "jpegdec", "!",
    "videoconvert", "!",
    "x264enc", "tune=zerolatency", "speed-preset=ultrafast", "!",  # Low latency H.264 encoding
    "video/x-h264,stream-format=avc,alignment=au", "!",
    "kvssink", f"stream-name={STREAM_NAME}"
]

print("Starting GStreamer...")
gst_proc = subprocess.Popen(gst_cmd, stdin=subprocess.PIPE)

try:
    while True:
        # Read 4-byte length prefix
        size_data = conn.recv(4)
        if not size_data:
            break
        frame_size = int.from_bytes(size_data, 'big')
        print(f"Receiving frame of size: {frame_size} bytes")
        
        # Read the actual frame
        frame_data = b''
        while len(frame_data) < frame_size:
            chunk = conn.recv(frame_size - len(frame_data))
            if not chunk:
                break
            frame_data += chunk

        # Send to GStreamer stdin
        gst_proc.stdin.write(frame_data)

# Ctrl+C handling
except KeyboardInterrupt:
    print("Interrupted.")

# Handle other exceptions
except Exception as e:
    print(f"An error occurred: {e}")

# Exit gracefully
finally:
    conn.close()
    gst_proc.stdin.close()
    gst_proc.terminate()
