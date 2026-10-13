#!/bin/sh
# SPDX-License-Identifier: MIT
# Run inside xvfb-run. Usage: native-check.sh FONT LABEL COMMAND [ARGS...]
set -eu
font=$1
label=$2
shift 2
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CUI_EXAMPLE_CAPTURE="$example_dir/build/$label-native.bmp" \
    "$@" "$font" 300 >"$example_dir/build/$label-native.log" 2>&1 &
client=$!
window=''
attempt=0
while [ "$attempt" -lt 100 ]; do
    window=$(xdotool search --name '^CUI language example$' 2>/dev/null || true)
    # Bare Xvfb has no window manager/client list. Some xdotool versions miss
    # SDL3 windows there; inspect the actual X window tree as a fallback.
    if [ -z "$window" ]; then
        window=$(xwininfo -root -tree 2>/dev/null |
            awk '/"CUI language example":/ { print $1; exit }')
    fi
    [ -z "$window" ] || break
    sleep 0.05
    attempt=$((attempt+1))
done
if [ -z "$window" ]; then
    wait "$client"
    echo 'No native window appeared' >&2
    exit 1
fi
sleep 0.2
xdotool mousemove --window "$window" 70 85 click 1
wait "$client"
rg -q 'count: 1' "$example_dir/build/$label-native.log"
test -s "$example_dir/build/$label-native.bmp"
cat "$example_dir/build/$label-native.log"
