# Pinned standalone Dialog flex reference

TEST ONLY. The source component pin is GPUI Kit0.7.1 at
`87d10ae5e1299d1be18670a37570c5402f952e34`; its Cargo.lock pins Taffy0.13.0.
`reference.py` records all four exact package/archive checksums. It compiles only
Taffy flexbox/tree/content-size with std, arrayvec0.7.8, slotmap1.1.1 and build-time
version_check0.9.5. No GPUI, fonts, platform/window host or CUI dependency.

Preparation is explicit and separate from OFFLINE execution. Use a subdirectory
of the already authorized existing build directory; do not create a new CUI build.
`prepare` uses verified existing archives first. `--network` permits only the four
pinned crate archive URLs, never a registry update or package/toolchain install.
Two archives were already cached for the initial run (arrayvec/version_check).

```sh
python tests/reference/dialog_flex/reference.py --cache "$BUILD/taffy-reference" --network prepare
python tests/reference/dialog_flex/reference.py --cache "$BUILD/taffy-reference" verify
# Only during the root-coordinated compiler slot:
nice -n 19 taskset -c 0 python tests/reference/dialog_flex/reference.py --cache "$BUILD/taffy-reference" build
```

Minimal local manifests remove unrelated optional/dev/example dependencies and
workspace metadata. Each exact manifest delta is retained with its hash; every
Rust source is checked against the verified upstream archive before build/run.
The selected feature names are recorded. No upstream algorithm source is edited.
Cargo builds OFFLINE, one job, O1, no debug/incremental output. The target has an
explicit100MiB artifact cap; the planned source budget is5MiB. No artifacts enter
the repository. A build receipt ties the unchanged driver/source/manifest receipt
to the exact executable; capture refuses stale or changed binaries.

Execute the capture under the normal private-Xvfb wrapper using a temporary
CTest registration inside the same existing build directory:

```cmake
# $BUILD/taffy-reference/ctest/CTestTestfile.cmake (absolute paths):
add_test(dialog_flex_taffy_reference "python" "/ABS/CUI/tests/reference/dialog_flex/reference.py"
 "--cache" "/ABS/BUILD/taffy-reference" "capture")
```

```sh
nice -n 19 taskset -c 0 python scripts/test_local.py --build-dir "$BUILD" --test-dir "$BUILD/taffy-reference/ctest" -R '^dialog_flex_taffy_reference$' -j1
```

The driver models the pinned structural defaults: independent Header/Title/
Description demands before Content and clipped Body peers, Footer outside their
inner flex group. Description is a composable Header child in actual GPUI; the
zero-gap fixed-height decomposition is equivalent only for this allocation
oracle, not a claim of separate automatic source submission. The tree uses percent-zero flex1 basis, automatic Content minimum and zero minimum
for overflow-hidden Body. No padding/gaps initially isolates allocation; actual
component spacing/typography and source rendering remain separate oracles.
It disables pixel rounding and outputs actual panel/inner/Content/Body/viewport
heights and positions. Auto/min96 versus min0 are explicit different cases.
Content minimum overflow, explicit min0/max/fixed, omitted/present-empty peers,
fixed-sibling overflow and resize are included. Numeric results must come from
an executed checksum-verified engine, not a hand-filled table. The output receipt
records engine/dependency versions, hashes/features/driver/compiler/output and
artifact size. It does not prove full GPUI rendering or CUI production parity.

`python tests/test_dialog_flex_reference.py` checks preparation/authority OFFLINE
with tiny synthetic archives. It never fetches, compiles or opens windows.

## Real-tree CUI comparison (test only)

The independent frozen CSV is an input to a separate CUI fixture. It never loads
Taffy or requires Cargo/network at test time. Build only the focused target in an
existing configured Release build during the coordinated compiler slot:

```sh
nice -n 19 taskset -c 0 cmake --build "$BUILD" --target test_dialog_flex_regions test_dialog_flex_regions_distribution -j1
nice -n 19 taskset -c 0 python scripts/test_local.py --build-dir "$BUILD" -R '^dialog_flex_(regions_(native|distribution)|reference_authority)$' -j1
```

The fixture now calls the public production Core V2 API. Its private probe
includes the actual source or generated distribution, with no copied allocator
or layout algorithm. The earlier test-copy generator/adapter were retired after
promotion; their signed historical checkpoints remain in Git. Persistent bounds
outlive end_frame. Current own-viewport prepaint bounds/semantic ranges, software
Footer/clip pixels, same-context resize, frozen V1/nested controls and32/64 shared
capacity/failure recovery remain exercised. Additional public contract tests
check exact resource bytes/slots, actual calloc/free counts, injected allocator
failure, mixed V1/V2, omitted history, cache fail-closed, aliases and lifetimes.
The public API remains C Core-only; no Dialog or foreign-facade API is implied.
This does not execute GPUI, source typography/raster, Dialog action ownership,
Root/native-host integration or foreign facades. Geometry/semantic evidence must
not be relabelled as queued wheel/touch/action completion.

The executed capture now has28 rows, including source vertical padding16, Panel
gap16, innergap8 at explicit rem16, Body omission versus an empty present child,
and bounded/AUTO oversized Footer. Source Footer overflow remains ordinary
paint beyond Panel, never a silent Outer scrolling policy.
