# UVC camera jpeg frame producer

## Purpose

This example is based on the official `uvc` example from `ESP-IDF`.

## Configuration

- Access the Example Configuration menu via `menuconfig`, update the `IPv4 address` and `Port` to match the server 
- Access the Example Connection COnfiguration menu via `menuconfig`, provide a valid WiFi ssid and pwd for connection

## Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.


## Troubleshooting

Start KVS bridge server first, to receive data sent from this KVS bridge client solution.