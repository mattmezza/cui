# Optional regex allocation oracle: CI failure and validation

The optional-regex job in CI run 38557271424 failed at `backends/regex/src/tests.rs:362` with `match allocated \\Aabc\\z` (observed 2, expected 0). The old test allocator used process-wide `TRACK`, `CALLS`, and `FAIL` atomics. `--test-threads=1` serializes Rust test functions, but it does not prevent other harness or runtime threads from allocating while the oracle measures `matching`. Those unrelated calls could enter both the zero-allocation count and the fault-injection sequence.

The allocator probe now uses a constant-initialized thread-local `Cell` for the measurement flag, allocation count, failure position, and live-byte balance. The `matching` assertion still requires exactly zero allocations on the calling thread. Fault injection still rejects each measured allocation position on that same thread, and the live-byte comparisons still check that failed preparations release their allocations. The new regression allocates on a worker thread while the test thread measures zero, then allocates on the test thread and verifies a positive count.

Validation used the exact pinned Rust 1.85.1 toolchain and existing verified crate archives in `/dev/shm/cui-next33-regex-native`:

```sh
python scripts/build_regex.py --cache /dev/shm/cui-next33-regex-native --toolchain /dev/shm/cui-next33-regex-toolchain/rust-1.85.1
CARGO_HOME=/dev/shm/cui-next33-regex-native/cargo RUSTC=/dev/shm/cui-next33-regex-toolchain/rust-1.85.1/bin/rustc /dev/shm/cui-next33-regex-toolchain/rust-1.85.1/bin/cargo test --offline --locked --release --manifest-path /dev/shm/cui-next33-regex-native/source/Cargo.toml --target-dir /dev/shm/cui-next33-regex-native/target -- --test-threads=1
```

Build succeeded; both tests passed. The same locked offline test command also passed ten consecutive runs with `--test-threads=2`. The vendored `regex-automata` emits its existing unused-method warning; no new warnings were observed from `cui-regex`.
