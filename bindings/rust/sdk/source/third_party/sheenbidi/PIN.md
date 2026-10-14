# SheenBidi source receipt

Upstream remains commit `cfe430e7375a7845b679adae9d51dac6deaa8858` from
https://github.com/Tehreer/SheenBidi/tree/cfe430e7375a7845b679adae9d51dac6deaa8858.
The Apache-2.0 license in LICENSE remains unchanged.

Local patch: Source/API/SBParagraph.c explicitly consumes each assertion-only
`stringLength` with `(void)stringLength`. Both original debug range assertions
remain intact. NDEBUG removes assertions without leaving an unused local under
GCC 16 Release warnings. No upstream version bump or runtime algorithm change.

Patched SHA-256: `947795f64e5d8c17c01a996e6ea05d50ae7e1c68b26b90da7222d051e4062fac`.
The integrator updates third_party/dependencies.json alongside this receipt.
