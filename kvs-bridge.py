import os
import time
from pathlib import Path
import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst

# AWS configs
assert os.environ.get("AWS_ACCESS_KEY_ID") and os.environ.get("AWS_SECRET_ACCESS_KEY"), "Missing AWS credentials"
assert os.environ.get("AWS_STREAM_NAME"), "Missing Kinesis Video Stream name"
STREAM_NAME = os.environ.get("AWS_STREAM_NAME") # Kinesis Video Stream name
fps  = 30

# Configuration
FRAME_FOLDER = "./frames"  # Folder containing static MJPEG frames
FRAME_INTERVAL = 1 / fps
frame_files = sorted(Path(FRAME_FOLDER).glob("*.jpg"))
print(f"Sending {len(frame_files)} frames...")
loops = 10

# Create the pipeline
Gst.init(None)
PIPELINE_NAME = "kvs-bridge-pipeline"
pipeline = Gst.parse_launch(
    f'appsrc name={PIPELINE_NAME} is-live=true format=time '
    '! jpegparse '
    '! decodebin '
    '! videoconvert '
    '! x264enc tune=zerolatency bitrate=512 speed-preset=ultrafast '
    f'! kvssink stream-name={STREAM_NAME}'
)

appsrc = pipeline.get_by_name(PIPELINE_NAME)
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

# Send frames to Kinesis Video Stream
try:
    for _ in range(loops):
        for frame_path in frame_files:
            frame_data = Path(frame_path).read_bytes()
            frame_size = len(frame_data)
            print(f"Sending frame of size: {len(frame_data)} bytes")
            if not push_image(frame_data, frame_size):
                print(f"Failed to push image into pipeline")

            frame_cnt += 1

            # Simulate desired fps
            time.sleep(FRAME_INTERVAL)

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
