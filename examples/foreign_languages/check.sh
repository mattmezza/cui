#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$example_dir/build/cui_example_host_test"
"$example_dir/build/go-demo"
"$example_dir/build/rust-demo"
node "$example_dir/node/main.cjs"
node "$example_dir/typescript/main.ts"
