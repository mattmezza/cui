#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cmake -S "$example_dir" -B "$example_dir/build" -G Ninja "$@"
cmake --build "$example_dir/build" --target cui_example_host cui_node cui_example_host_test
CGO_CFLAGS="-I$example_dir -Wall -Wextra -Werror" \
CGO_LDFLAGS="-L$example_dir/build -lcui_example_host -Wl,-rpath,$example_dir/build" \
    go build -C "$example_dir/go" -o "$example_dir/build/go-demo" .
rustc --edition=2024 -D warnings "$example_dir/rust/main.rs" \
    -L "$example_dir/build" -l cui_example_host \
    -C "link-arg=-Wl,-rpath,$example_dir/build" -o "$example_dir/build/rust-demo"
