# Dialog source placement: existing-tree proof

TEST ONLY on Core `d4a1d847`, following component contract `7431bdd3`.
No source-position component API is admitted by static preparation alone.

## Frozen source authority

GPUI Kit0.7.1 `87d10ae5e1299d1be18670a37570c5402f952e34`,
`crates/component/src/dialog/dialog.rs:560-745` computes preferred y as
viewport height/10 plus layer index*16, width constrained by horizontal margins,
and Panel max height as viewport height minus margins and layer offset.
`crates/base/src/positioner.rs:210-225,303-324` resolves a top-left corner and
clamps bottom overflow upward, then clamps top to margin. Exact raw file is
26213 bytes, SHA256
`7034aaeee0a19d301c814d38487ab13103a3e83a3d5f733ac4e5183c0478f883`.
Only this missing pinned source was fetched into the existing audit cache; no
crate, engine, application or dependency was built. Existing28-row Taffy engine
oracle remains unchanged. Positioner itself is inspected source, not executed
GPUI; numerical constants below follow its explicit clamp statements.

## Candidate tree using public APIs only

Root-attached portal Mount fills usable inset viewport; horizontal alignment CENTER.
Before the ordinary V2 Panel submit a GROW spacer with maximum
`max(0, preferred_top-margin)`. For zero cap OMIT the spacer: public max_height0
means uncapped, so a zero maximum would be incorrect. Panel width is fixed and
independent of allocated Y. Core V2 stages current Panel height before its
already-approved second stored-tree calculation; spacer then receives remaining
height up to its cap. No prior bounds, callback replay, third calculation,
private Core access, prepaint mutation or new Core record is involved.

This tree should place Panel at
`margin + min(max(0,preferred_top-margin),usable_height-current_panel_height)`
for admitted current Panel height <= usable height. Exact Positioner equivalent
is its bottom-overflow shift followed by top clamp. Oversized Footer may extend
beyond Panel by existing source geometry policy; it must not change Panel's
placement demand into a whole-descendant bounds union.

## Frozen expected runtime cases

Margins16; same UI transitions between every row, without priming frames.

| Viewport | Current Panel height | Preferred top | Expected top | Panel width |
| --- | --- | --- | --- | --- |
|400x600|40|60|60|240|
|400x600|550|60|34|240|
|400x600|568|60|16|240|
|180x220|188|22|16|148|
|180x220|40|22|22|148|
|800x800|80|96|96|420|
|800x800|740|96|44|420|
|400x600|40|0|16|240|
|400x600|568|60|16|240|
|400x600|40|60|60|240|

The explicit preferred96 case models viewport/10+one16px layer offset;
its caller-provided Panel max in this fixture is deliberately independent, not
an assertion that component layer defaults have already been implemented.
Fixture asserts current Panel/Body/tracked/semantic/prepaint positions, ranges,
software pixels immediately inside/outside top/bottom and callbacks once.
This remains synthetic numerical content; current-font wrapping and component
wheel/touch/action/lifecycle proof are separate required gates.

Runtime: actual production source and generated distribution both PASS, ten
placement transitions plus the unchanged28 source rows/contracts in each target.
Final session23912 compiled four steps (two fixture TUs/two links), strict C11
O3 Wall/Wextra/Wpedantic/Werror, nice19 CPU0 single job in the reused builddir.
Private-Xvfb wrapper source/distribution2/2PASS0.29s. No Core TU was rebuilt.
The first attempted run rejected a parent-attached absolute Mount without a
parent before any geometry oracle; changing the TEST fixture to the specified
root-attached portal fixed that API misuse. No source oracle or production
algorithm was altered to obtain passing positions.

```sh
cmake --build "$BUILD" --target test_dialog_flex_regions test_dialog_flex_regions_distribution -j1
python scripts/test_local.py --build-dir "$BUILD" -R '^dialog_flex_regions_(native|distribution)$' -j1 -V
```

No CMake/public API/production mutation, engine execution or new build directory.
The source-compatible placement tree is now demonstrated for this admitted
synthetic-content matrix, not a completed Dialog component or font/raster claim.
Canonical skill freshness and offline reference authority are checked separately.

## Source minimum beyond a small viewport

Separate `small_viewport.rs` uses the exact existing verified Taffy0.13.0 rlib;
original28-row main/CSV/receipt remain byte-identical. Actual warning-clean
rustc session85403, edition2021/O1/debug0/codegen1, produced four rows: min96 wins
max48 for body20 and body900; min96 wins max8 for empty Body; ordinary max568
also yields96 for short Body. `small-viewport.csv` is captured actual output;
its separate receipt includes driver, old engine receipt, rlib, binary and CSV
hashes/features/toolchain. Added binary571296 bytes, existing reference cache
now9.5MiB, no engine rebuild/fetch/new dependency or buildtree.

Source CSS/Taffy accepts min96>raw max48/8 and gives the minimum precedence.
Core's public record deliberately rejects min>max. The component must normalize
its EFFECTIVE Core maximum to max(source minimum, source constrained maximum),
retaining source minimum96; it must not lower the minimum or reject an otherwise
ordinary small viewport. This is typed source-style normalization before strict
Core admission, not a relaxation of Core validation or a new layout solver.

Three additional actual CUI source/distribution placement rows pass: viewport
180x80, Body20/900, effective Panel96, top16; viewport180x40, empty Body,
effective Panel96, top16. The same GROW spacer collapses0 when Panel exceeds
usable48/8. Current bounds, semantic/prepaint heights/top and once-only callback
remain96/16; long-body software pixels clip at the host bottom. Ordinary Panel
and Footer overflow policy is retained; no whole-Outer fallback appears.

Session23510 strict five steps and private-Xvfb source/distribution2/2PASS0.30s.
Besides two fixture TUs/two links, Ninja correctly rebuilt the touched generated
Core distribution probe once after canonical selective regeneration. No source
Core algorithm or public API was changed. The existing target now checks13
placement transitions plus its original28 source rows/contracts. Offline receipt
and engine authority checks7/7PASS. Component default normalization and real
font/input/lifecycle validation remain subsequent production work.
