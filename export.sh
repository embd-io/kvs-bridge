# !/bin/bash

# Gstreamer plugin for Kinesis Video Streams
KVS_SDK_PATH="amazon-kinesis-video-streams-producer-sdk-cpp"
KVS_BUILD_PATH="`pwd`/${KVS_SDK_PATH}/build"
export GST_PLUGIN_PATH=$KVS_BUILD_PATH

# AWS KVS credentials
export AWS_DEFAULT_REGION="xx-yyyy-N"
export AWS_ACCESS_KEY_ID="xxxxxxxxxxxxxxxxxxxx"
export AWS_SECRET_ACCESS_KEY="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"