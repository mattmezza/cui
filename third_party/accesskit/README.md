# Optional AccessKit dependency

`pin.json` identifies the immutable accesskit-c 0.23.0 archive and its Cargo.lock.
`scripts/build_accesskit.py` checks the archive, every cached source file, and lock,
then builds with `cargo build --release --locked` in a private local cache.
There is no global install, nightly requirement, Corrosion, or default Core Rust
dependency. Rust 1.85+ is required for this optional build.

The CUI-specific private lifetime extension is
`backends/accesskit/cui_lifetime.rs`, with hashes in that directory's
`extension.json`. The only upstream entry-point change is appending
`mod cui_lifetime;`. It adds an owned Unix constructor without changing existing
upstream ABI. Shared Rust handler ownership calls a C release callback after the
last handler drops, including asynchronous adapter removal.

AccessKit is MIT OR Apache-2.0. The preserved LICENSE-MIT, LICENSE-APACHE and
LICENSE.chromium contain the upstream notices. Cargo.lock pins transitive
packages. `LICENSES.dependencies.txt` preserves the 90-package Linux host tree
(including build/proc-macro packages conservatively). Four AccessKit Rust crates
omit workspace license files from their crate archives; their notices are fetched
from the exact `.cargo_vcs_info.json` commit recorded/hashes in `pin.json`.
A binary distributor must additionally review target-specific and Rust toolchain
notices; this source inventory is not a complete cross-platform binary audit.
The optional artifact must not be distributed before the integrator has included
those notices in package/license verification.

Source: https://github.com/AccessKit/accesskit-c/tree/0824c4a1e3a4d13ce5582df20e394fba49485a15
