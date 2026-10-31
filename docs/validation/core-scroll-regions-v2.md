# Core scroll-body V2 local validation

Additive C Core only. Public24-byte region/144-byte body record, fixed-width Y
allocation for two explicit peers and separate intrinsic Footer. Existing V1
80-byte record, geometry, dispatch and resource allocation remain. Layout-record
version2 is independent of draw ABI2/background negotiation; these oracles use
ordinary begin_frame/end_frame V1 draw lists and need no new renderer support. No Base/Styled
Dialog, Root/native host or five-facade slot/action completion is claimed.

Source proof is the actual28-row pinned Taffy0.13.0 reference at
`tests/reference/dialog_flex`, with verified unchanged engine source, explicit
features/minimal manifest deltas and binary/driver/output receipts. The original
18-row prefix stays byte-identical. No Cargo/network dependency in normal tests.
New V2 tests call actual production source and generated Core distribution;
private probe contains only inspection, allocator counters/failure injection and
cache corruption, never an alternate layout implementation.

Focused reused portable Release build: GCC16.2.1, C11, O3, UNDEBUG,
Wall/Wextra/Wpedantic/Werror. C++11 validates public sizes, offsets and declaration.
All builds/tests nice19 CPU0, single job; private-Xvfb wrapper only.

```sh
cmake --build "$BUILD" --target test_dialog_flex_regions test_dialog_flex_regions_distribution test_scroll_layout_contract test_scroll_layout_distribution test_scroll_regions_header -j1
python scripts/test_local.py --build-dir "$BUILD" -R '^(dialog_flex_regions_(native|distribution)|dialog_flex_reference_authority|scroll_layout_contract|scroll_layout_distribution|scroll_regions_header_cpp)$' -j1
```

Six focused CTest checks pass: source/distribution28 numerical/geometry/software
rows plus contract permutations, six offline reference authority checks, frozen
V1 source/distribution and C++ header. Current bounds/ranges/prepaint/caret and
same-UI width/height/omission/AUTO resizing pass. Source gap16/8 at explicit rem16,
Body OMIT versus present-empty, bounded/AUTO Footer400 and software clipping/
ordinary Footer overflow remain distinct. Explicit-height numerical leaves do
not establish source font wrapping/raster or arbitrary rem/gutter closure.

Actual LP64 V1 resource remains12856 bytes/one slot; context255520/pointer8
unchanged in frozen V1 probe. V2 sidecar2560/two slots total15416. Zero-opt-in
resource calloc0. V1-only actual base calloc1/sidecar0. Exact budgets and each
byte/slot-short case, injected actual sidecar allocator failure, copied records,
no partial model commit, clear-before-end failed frame and next V1 recovery pass.
Nine history transitions give actual base alloc/free3/3, sidecar3/3. Mixed call
orders share capacity.32 bodies/64 clamps admit;33rd fails atomically/recovery.

Cached-slot zero/out-of-range/wrong-owner-slot/missing flag and mismatched V2
entry/base-version markers fail closed with empty draw/non-OK/no model commit,
then recover after clear/rebuild. Full/folded late aliases, retained bounds alias,
late nonfinite model, optional-ID/standalone clamp conflicts in both orders,
inert/layout-only model suppression, independent portals versus forbidden
referenced portal, wrong phase and independent-context failure marker pass.
Resource collectors reset independently before cache reuse; no new dispatcher,
third calculation, user callback or input replay is added.

Canonical source/API metadata, SDK/dist/skill generation and full installed-skill
checks accompany the signed handoff. Local evidence is portable software/source
and distribution; hosted Windows/macOS, actual new V2 current-font cases and
full Dialog queued wheel/touch/actions/facade ownership remain separate checks.
