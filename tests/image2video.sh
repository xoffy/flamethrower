#!/bin/sh

( cd "../build" && ninja )

EXECUTABLE="../build/flamethrower"

if [ ! -x "$EXECUTABLE" ]; then
  echo "Can't find \`flamethrower\` executable! Is build failed?"
  exit 1
fi

INPUT=${1:-pictures/police-duck.jpg}
OUTPUT=$(basename $INPUT | sed 's?\.[^.]*??')
LENGTH=${2:-10}

[ -d "./experiments/videos" ] || mkdir -p "./experiments/videos"
[ -f "./experiments/videos/$OUTPUT.mp4" ] && rm -f "./experiments/videos/$OUTPUT.mp4"

$EXECUTABLE -R -f jpg -a $(($LENGTH * 25)) $INPUT - \
  | ffmpeg \
  -f image2pipe -r 25 -c:v mjpeg -i - \
  -f lavfi -i "sine=frequency=1000:duration=$LENGTH" \
  -c:v libx264 -pix_fmt yuv420p -c:a aac ./experiments/videos/$OUTPUT.mp4
