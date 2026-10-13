#!/bin/sh
# SPDX-License-Identifier: MIT
# Run under xvfb-run; font path is required. Coordinates target 1200x900.
set -eu
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
font=$1
geometry=$(xdotool getdisplaygeometry)
screen_width=${geometry%% *}
screen_height=${geometry##* }
if [ "$screen_width" -lt 1200 ] || [ "$screen_height" -lt 900 ]; then
    echo 'Use xvfb-run -s "-screen 0 1600x1200x24" for this 1200x900 interaction check' >&2
    exit 1
fi
"$example_dir/build/northstar" --font "$font" --page pricing --frames 240 \
    --capture "$example_dir/build/native-interactions.bmp" \
    >"$example_dir/build/native-interactions.log" 2>&1 &
client=$!
window=''
attempt=0
while [ "$attempt" -lt 100 ]; do
    window=$(xdotool search --name '^Northstar / CUI composed app$' 2>/dev/null || true)
    [ -z "$window" ] || break
    sleep 0.05
    attempt=$((attempt+1))
done
if [ -z "$window" ]; then
    wait "$client"
    echo 'No Northstar window appeared' >&2
    exit 1
fi
sleep 0.2
xdotool windowfocus "$window"
xdotool mousemove --window "$window" 710 560 click 1
sleep 0.15
xdotool mousemove --window "$window" 1120 36 click 1
sleep 0.15
xdotool key --window "$window" End Return
sleep 0.15
xdotool mousemove --window "$window" 100 181 click 1
sleep 0.15
xdotool mousemove --window "$window" 440 320 click 1
xdotool key --window "$window" ctrl+a
xdotool type --window "$window" --clearmodifiers --delay 5 'Grace Hopper'
sleep 0.15
xdotool mousemove --window "$window" 288 405 click 1
xdotool mousemove --window "$window" 294 480 click 1
xdotool mousemove --window "$window" 340 560 click 1
sleep 0.15
xdotool mousemove --window "$window" 1120 36 click 1
sleep 0.15
xdotool key --window "$window" End Return
wait "$client"
rg -q 'event=2 page=1 plan=1' "$example_dir/build/native-interactions.log"
rg -q 'event=1 page=1 plan=1 session=1' "$example_dir/build/native-interactions.log"
rg -q 'event=3 page=2 plan=1 session=1 saves=1 name=Grace Hopper notifications=0 sync=1' "$example_dir/build/native-interactions.log"
rg -q 'event=1 page=2 plan=1 session=0 saves=1' "$example_dir/build/native-interactions.log"
test -s "$example_dir/build/native-interactions.bmp"
cat "$example_dir/build/native-interactions.log"
