#!/bin/sh
set -eu
: "${CUI_BRIDGE_LIB_DIR:?set CUI_BRIDGE_LIB_DIR to absolute bridge build path}"
: "${CUI_TEST_FONT:?set CUI_TEST_FONT to a TTF path}"
command -v xvfb-run >/dev/null || { echo 'xvfb-run is required' >&2; exit 1; }
cd "$(dirname "$0")/../../.."
env -u DISPLAY -u WAYLAND_DISPLAY xvfb-run -a env CUI_TEST_VIRTUAL_DISPLAY=1 CUI_RUST_SMOKE_FRAMES=3 cargo run --manifest-path bindings/rust/Cargo.toml --example save_changes
