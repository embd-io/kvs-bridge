import os
import subprocess
import time
from pathlib import Path

# Configuration
STREAM_NAME = "kvs-bridge-stream" # Kinesis Video Stream name
FRAME_FOLDER = "./frames"  # Folder containing static MJPEG frames
FRAME_INTERVAL = 1 / 30  # 30 FPS

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

# Fetch static frames
frame_files = sorted(Path(FRAME_FOLDER).glob("*.jpg"))
print(f"Sending {len(frame_files)} frames...")
loops = 1

# Read and write frames into GStreamer -> KVS stream
try:
    for _ in range(loops):
        for frame_path in frame_files:
            print(f"Sending frame: {frame_path}")
            # Read and send each frame to GStreamer
            with open(frame_path, "rb") as f:
                frame_data = f.read()
                gst_proc.stdin.write(frame_data)
                gst_proc.stdin.flush()
            time.sleep(FRAME_INTERVAL)

# Ctrl+C handling
except KeyboardInterrupt:
    print("Interrupted.")

# Handle other exceptions
except Exception as e:
    print(f"An error occurred: {e}")

# Exit gracefully
finally:
    print("Closing GStreamer...")
    gst_proc.stdin.close()
    gst_proc.wait()
