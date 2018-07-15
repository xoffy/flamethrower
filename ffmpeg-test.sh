#!/bin/sh

SCRIPT_PATH=$(readlink -f $0)
SCRIPT_DIR=$(dirname $SCRIPT_PATH)
SECAM_DIR="$SCRIPT_DIR/build"
SECAM_EXEC="$SECAM_DIR/src/flamethrower"

command -v ffmpeg >/dev/null 2>&1
if [[ $? -ne 0 ]]; then
    echo >&2 "ffmpeg is not installed!"
    exit 1
fi

if [[ ! -x "$SECAM_EXEC" ]]; then
    echo >&2 "flamethrower must be built!"
    exit 1
fi

remove_garbage() {
    echo "[?] removing garbage ($OUTPUT_NAME-*.$OUTPUT_EXT)..."
    echo $(pwd)
    rm -- "$OUTPUT_NAME"-*."$OUTPUT_EXT"
}

SECAM_INPUT="$1"; shift
SECAM_OUTPUT="$1"; shift
SECAM_LENGTH="$1"; shift

echo "[1] secamizing..."
"$SECAM_EXEC" -i "$SECAM_INPUT" -o "$SECAM_OUTPUT" -a $SECAM_LENGTH \
    > "flamethrower.txt"

if [[ $? -ne 0 ]]; then
    echo >&2 "error in flamethrower!"
    remove_garbage
    exit 1
fi

OUTPUT_BASENAME=$(basename -- "$SECAM_OUTPUT")
OUTPUT_NAME="${OUTPUT_BASENAME%.*}"
OUTPUT_EXT="${OUTPUT_BASENAME##*.}"

FFMPEG_INPUT="$OUTPUT_NAME"-%d."$OUTPUT_EXT"
FFMPEG_OUTPUT="$OUTPUT_NAME".mp4

echo "[2] collecting frames to video..."
ffmpeg -i "$FFMPEG_INPUT" \
    -c:v libx264 \
    -vf fps=25 \
    -pix_fmt yuv420p \
    "$FFMPEG_OUTPUT" \
    > "ffmpeg.log"

if [[ $? -ne 0 ]]; then
    echo >&2 "error in ffmpeg!"
    remove_garbage
    exit 1
fi

remove_garbage