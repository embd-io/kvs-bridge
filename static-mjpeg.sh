# !/bin/bash

if [ -f $1 ]; then
	echo 'video file: $1'
else
	echo "invalid video file: '${1}'"
	exit 1
fi

mkdir -p frames

ffmpeg -i $1 -vf "scale=640:480" sample.mp4
ffmpeg -i sample.mp4 -qscale:v 2 -vf fps=30 frames/frame_%04d.jpg

rm sample.mp4

exit 0
