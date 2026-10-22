# Distribution test compilation

Design checkpoint: share a private test-only `CUI_IMPLEMENTATION_ALL` object
between feature distribution consumers. Keep `dist_all` as an independent
self-contained implementation and consumer in one translation unit. Keep every
single/multi-file, standalone, legacy and additive module implementation target
unchanged, including standalone DateTimePicker and Stepper. Production libraries,
headers, amalgamations and install/export rules are outside this change.

At revision ad4c6f37, ten source fixtures plus the generated Popover fixture
compile the full umbrella implementation. Eligible feature consumers use public
APIs; any consumer requiring private static helpers must remain independent.
Compile the shared object and consumers with the existing warnings, assertions,
sanitizers and configuration optimization. Do not link the source CUI library
into these umbrella tests. Feature tests continue to run against the shipped
amalgamation. The independent `dist_all` retains same-translation-unit optimized
umbrella coverage. Consolidated feature tests lose their repeated per-feature
same-translation-unit optimization/inlining proof; they exercise separate consumer
and implementation objects instead. Packaging and standalone entry contracts stay
independent.

Completed [Windows Release run 39774911352](https://github.com/mattmezza/cui/actions/runs/39774911352)
spent 463 seconds building (12:01:13–12:08:56 UTC, 22 October 2026) and 410 seconds
running CTest. This was MSVC Release, not a GCC `-O3` measurement. Distribution
target diagnostic-to-link spans overlap and cannot establish isolated costs or
predicted savings. CTest also spent 149.25 seconds on package-source checks and
140.24 seconds on installed-skill checks. Consolidation does not remove those
checks. No Windows improvement is established until subsequent CI measurements.

Alternatives are retaining all repeated compilations (maximum same-TU coverage)
or adding compiled-output caches (more invalidation and provenance complexity).
Lowering optimization or dropping runtime/standalone tests is excluded.
