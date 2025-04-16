# MJPEG to KVS stream

## Reference

- https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/examples-gstreamer-plugin.html
- https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp.git
![alt text](doc/gst-pipeline.png)

## MP4 source example

- https://www.pexels.com/download/video/2886841/

## Procedure

1. Convert video into 640x480 MJPEG frames

```
./static-mjpeg.sh <path/to/video/file>.mp4
```
2. Install dependencies and build `kvssink` Gstreamer plugin

```
./build-gst-plugin.sh
```
3. Export path to plugin built and credentials

```
. ./export.sh
```

4. Run kvs-bridge

4.1. standalone kvs-bridge.py example

```
python3 kvs-bridge.py
```

4.2. server-client kvs-bridge example

```
# first start the server

python3 kvs-bridge-server-appsrc.py
```

```
# then start the client

python3 kvs-bridge-client.py
```

4.3. ESP32 based static jpeg producer

```
# first start the kvs bridge server

python3 kvs-bridge-server-appsrc.py
```

```
# then configure, build and flash ESP32 firmware
# ref. esp32/tcp_client/README.md 

idf.py -p <port> build flash monitor
```

4.4. ESP32 UVC live MJPEG camera frames

```
# first start the kvs bridge server

python3 kvs-bridge-server-appsrc.py
```

```
# then configure, build and flash ESP32 firmware
# ref. esp32/uvc/README.md 

idf.py -p <port> build flash monitor
```

5. Run kvs consumer sample

5.1. Purpose

This is useful for debugging and postprocessing purposes

5.2. Setup

```
git submodule add https://github.com/aws-samples/amazon-kinesis-video-streams-consumer-library-for-python.git
cd amazon-kinesis-video-streams-consumer-library-for-python
pip install virtualenv
python -m virtualenv venv
venv/bin/python -m pip install requirements.txt
```

5.3. Modify sample

E.g., update `on_fragment_arrived` callback to enable saving of mkv video contents into jpeg

5.4. Run consumer sample

```
venv/bin/python kvs_consumer_library_example.py
```