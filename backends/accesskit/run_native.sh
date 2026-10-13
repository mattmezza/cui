#!/bin/sh
# SPDX-License-Identifier: MIT
# Keep accessibility settings in memory and DBus/X11 in isolated test sessions.
set -eu
if [ "${CUI_A11Y_PRIVATE_SESSION:-}" != 1 ]; then
    exec env GSETTINGS_BACKEND=memory dbus-run-session -- xvfb-run -a \
        env CUI_A11Y_PRIVATE_SESSION=1 "$0" "$@"
fi
binary=${1:-./build-accesskit/test_accessibility_native}
gdbus call --session --dest org.a11y.Bus --object-path /org/a11y/bus \
    --method org.freedesktop.DBus.Properties.Set org.a11y.Status IsEnabled '<true>' >/dev/null
# Arch's activation can require a systemd user manager, absent in private DBus.
registry_binary=
for candidate in /usr/lib/at-spi2-registryd /usr/libexec/at-spi2-registryd /usr/lib/at-spi2-core/at-spi2-registryd; do
    if [ -x "$candidate" ]; then registry_binary=$candidate; break; fi
done
if [ -z "$registry_binary" ]; then registry_binary=$(command -v at-spi2-registryd); fi
"$registry_binary" &
registry_pid=$!
trap 'kill "$registry_pid" 2>/dev/null || true' EXIT HUP INT TERM
sleep 1
"$binary"
