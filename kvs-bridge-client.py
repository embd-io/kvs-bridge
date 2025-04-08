import socket
import time
import glob

fps = 30

HOST = '127.0.0.1'
PORT = 5000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

# Get sorted list of JPEGs
jpeg_files = sorted(glob.glob("frames/frame_*.jpg"))
loops = 1

for _ in range(loops):
    for jpg_file in jpeg_files:
        with open(jpg_file, "rb") as f:
            data = f.read()
            # Send frame size first (4 bytes)
            sock.sendall(len(data).to_bytes(4, 'big'))
            print(f"Sending frame: {jpg_file}, size: {len(data)} bytes")
            # Then send the actual JPEG data
            sock.sendall(data)
        time.sleep(1 / fps)

sock.close()
