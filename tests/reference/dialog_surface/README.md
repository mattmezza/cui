# Independent legacy Dialog/Surface reducer oracle

`legacy_surface.c` is an exact snapshot of `src/base/cui_surface.c` at signed
`d4a1d847`, before the approved private reducer extraction. The SHA256 and source
revision live in `provenance.json`. Do not update it to follow production edits.
`tests/unit/dialog_surface_legacy_probe.c` compiles it as a separate TU with18
explicit public-symbol renames; no behavior patch, fake dispatch or copied new
policy is involved. Offline tests validate both bytes and renaming inventory.

`test_dialog_surface_legacy` runs the actual current source and frozen source
with separate contexts through the same four-kind64-frame scenario. It compares
surface state, focus, synchronous decision/closed callback observations, routed
event targets/activations/consumption and published semantic bounds. The close
observer asserts it sees closed state and records focus after restoration.
Wheel, veto, deferred request, Escape, reserved close/confirm/cancel activations,
outside and reopen are exercised. This is a legacy equivalence oracle; it does
not prove new component composition or new touch/scrollbar behavior.

```sh
cmake --build "$BUILD" --target test_dialog_surface_legacy -j1
python scripts/test_local.py --build-dir "$BUILD" -R '^dialog_surface_(legacy_trace|reference_authority)$' -j1 -V
```

Baseline captured before production reducer editing: session54374, strict C11
O3 Wall/Wextra/Wpedantic/Werror seven-step focused build, nice19 CPU0 single
job in the existing builddir. Private-Xvfb two tests PASS0.15s; actual64-frame
four-kind trace and two offline authority assertions pass. Full installed-skill
suite remains deferred until the component/reducer stable checkpoint; no native
window, GPU, renderer or new Dialog behavior completion is claimed.
