import os
import socket
import subprocess
import struct
import sys
import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

import time

Gst.init(None)

# Configuration
HOST = '0.0.0.0'
PORT = 5000

# Create a TCP/IP socket and listen for incoming connection
try:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen(1)
    conn, addr = server.accept()
    print(f"Connection from {addr}")
except socket.error as e:
    print(f"Socket error: {e}")
    sys.exit(1)

# AWS configs
assert os.environ.get("AWS_ACCESS_KEY_ID") and os.environ.get("AWS_SECRET_ACCESS_KEY"), "Missing AWS credentials"
assert os.environ.get("AWS_STREAM_NAME"), "Missing Kinesis Video Stream name"
STREAM_NAME = os.environ.get("AWS_STREAM_NAME") # Kinesis Video Stream name
fps = 10

# Create the pipeline
pipeline = Gst.parse_launch(
    'appsrc name=mysrc is-live=true format=time '
    '! jpegparse '
    '! decodebin '
    '! videoconvert '
    '! x264enc tune=zerolatency bitrate=512 speed-preset=ultrafast '
    f'! kvssink stream-name={STREAM_NAME}'
)

appsrc = pipeline.get_by_name("mysrc")
appsrc.set_property("caps", Gst.Caps.from_string(f"image/jpeg,framerate={fps}/1"))

# Start the pipeline
pipeline.set_state(Gst.State.PLAYING)

# Push JPEG buffers
def push_image(frame_data, frame_size) -> bool:
    buf = Gst.Buffer.new_allocate(None, frame_size, None)
    buf.fill(0, frame_data)
    buf.pts = buf.dts = int(time.time() * Gst.SECOND)  # PTS/DTS are required
    buf.duration = Gst.SECOND // fps
    retval = appsrc.emit("push-buffer", buf)
    if retval != Gst.FlowReturn.OK:
        print(f"Buffer push failed: {retval}")
        return False
    return True

frame_cnt = 0

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
        print(f"Sending frame of size: {len(frame_data)} bytes")
        if not push_image(frame_data, frame_size):
            print(f"Failed to push image into pipeline")
        else:
            print(f"Frame successfully pushed to pipeline")
        with open(f"./frames_client/frame_{frame_cnt}.jpg", "wb") as f:
            f.write(frame_data)

        frame_cnt += 1


# Ctrl+C handling
except KeyboardInterrupt:
    print("Interrupted.")

# Handle other exceptions
except Exception as e:
    print(f"An error occurred: {e}")

# Exit gracefully
finally:
    appsrc.emit("end-of-stream")
    pipeline.set_state(Gst.State.NULL)
