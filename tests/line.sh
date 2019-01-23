#!/bin/sh

( cd "../build" && ninja )

EXECUTABLE="../build/experiments/line"

if [ ! -x "$EXECUTABLE" ]; then
  echo "Can't find \`experiments/line\` executable! Is build failed?"
  exit 1
fi

[ -d "./experiments" ] || mkdir -p "./experiments"

$EXECUTABLE "./experiments/line.png"
