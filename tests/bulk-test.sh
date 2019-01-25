#!/bin/sh

SECAMIZER="../build/flamethrower"

if [ ! -x "$SECAMIZER" ]; then
  echo "Can't find \`flamethrower\` executable! Did you build it?"
  exit 1
fi

[ -d "./secamized" ] || mkdir -p "./secamized"

for i in ./pictures/*; do
  echo "-- $i..."
  $SECAMIZER $* $i "./secamized/$(basename $i)" || exit 1
done

echo "-- Done!"
