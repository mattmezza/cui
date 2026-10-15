# Debug a native CUI app

Start with the smallest failing frame and check the installed revision's public
headers. [Layout](LAYOUT_GUIDE.md) and [IDs/lifetimes](IDS_AND_LIFETIMES.md) explain
the contracts most often involved. The feature manifest remains the authority
for incomplete behavior across all 78 entries.

After `cui_end_frame`, inspect `cui_last_error(ui)` before rendering. Errors are
sticky: record the failure before explicitly clearing it with `cui_clear_error`.
A draw-command capacity failure returns an empty draw list so clips remain
balanced; an empty frame is not proof of a renderer failure. Check configured
element, command and string capacities rather than repeatedly clearing errors.

`cui_first_diagnostic(ui, &diagnostic)` preserves the first failure and
`cui_latest_diagnostic(ui, &diagnostic)` tracks the most recent one. Both return
whether a snapshot exists. Snapshots include error/detail, subsystem, operation, bounded message, frame sequence, implicated
ID, optional used/requested/limit counts, and a bounded outermost-box ID path
with a truncation flag. Zero counts mean unspecified. `cui_clear_error` clears
both snapshots too. `cui_set_diagnostic_callback` installs an optional callback;
copy the borrowed snapshot if retaining it, and do not reenter/mutate the context
from that callback. The library does not print diagnostics automatically.
Application/component code can report a failure with `cui_report_diagnostic`.

For geometry inspection call `cui_set_layout_debug(ui, true)` before composition.
After `cui_end_frame`, and before the next begin, export using
`cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, buffer, capacity, &required)`
or `CUI_DEBUG_LAYOUT_TEXT`. A null/zero-capacity sizing query returns false and
sets `required` including the terminating NUL; allocate that much storage and
retry. Insufficient buffers remain untouched. Export covers boxes, not individual
text nodes: IDs, parent indexes, requested dimensions, resolved bounds/clips and
resolution status help distinguish sizing from paint failures. JSON IDs are
strings so all 64 bits survive JavaScript parsing. Export performs no file I/O;
the application owns persistence and buffer allocation. Disable capture when no
longer needed; opt-in snapshots are a debugging aid, not an accessibility tree.

| Symptom | First checks |
| --- | --- |
| Content too wide or not wrapping | Parent width constraint, FIT versus grow, text-line API, font measurement |
| Missing or cut-off content | Clip ancestors, fixed height, max dimensions, capacity errors |
| Wrong row activates after reorder | Persistent row key and local ID scope; duplicates |
| First click seems ignored | Input targets exist only after a completed frame |
| Missing icon | `cui_icon_query` distinguishes unknown, catalog-not-embedded and embedded assets; inspect `CUI_ICON_NOT_FOUND`; optionally enable `debug_placeholder` |
| Missing glyph or wrong weight | Font script coverage and registered style; no universal automatic fallback |
| Native app fails before first frame | Font path, SDL3 error, pkg-config prerequisites and actual host display |

Do not use emoji as an icon fallback: font coverage and appearance vary. Use a
registered icon name, a deliberate text label, or an application-owned bitmap.
A declared optional API does not prove its implementation is in the bundle.

For a reproducible report include revision, renderer, platform, window dimensions,
font files, triggering interaction, expected result, error, and bounded capture.
Consumer captures use `--frames 3 --capture frame.bmp`. Automated native Linux
runs must use invisible Xvfb; repository validation uses
`python scripts/test_local.py --build-dir build`. Never use `DISPLAY=:0` or the
maintainer's Wayland session. Browser tests run headless. GPU hardware behavior
unsupported by the virtual display remains explicitly deferred.

The bundle host registers a diagnostic callback that writes subsystem, operation,
message, error/detail, ID, frame and capacity counters to stderr. This covers
reported Core/component failures after context creation; font and SDL setup or
render failures use their own explicit host messages. It is not a universal
warning stream for every missing asset or every optional API.

```sh
./build/native_starter --frames 1 --debug-layout text
./build/native_starter --frames 1 --debug-layout json > layout.json
```

The option prints the first completed frame once, then continues the normal
bounded or interactive run. JSON goes to stdout; diagnostics go to stderr.

## Default context budgets

Zero-valued capacity fields in `cui_config` select these defaults:

| Field | Default | Budget |
| --- | --- | --- |
| `max_elements` | 4096 | Layout/component elements |
| `max_commands` | 8192 | Draw commands |
| `frame_bytes` | 262144 (256 KiB) | Copied frame data |
| `max_resources` | 128 | Context-owned resource entries |
| `resource_bytes` | 1048576 (1 MiB) | Context-owned resource storage |

Use a diagnostic's subsystem and used/requested/limit counters to choose which
budget needs attention. Raising draw-command capacity does not repair an
unbalanced scope or duplicate ID. These are defaults for this revision, not
promises that every application fits them.

Missing-icon availability is separate from context error reporting. Read
`cui_icon_query(cui_string("clock-3"))`: the default bundle reports
`CUI_ICON_CATALOG_NOT_EMBEDDED`; a full-assets build reports `CUI_ICON_EMBEDDED`.
Check the icon call's result, and use `.debug_placeholder = true` only when a
visible development outline is useful. It retains `CUI_ICON_NOT_FOUND` and the
full accessible name. See [Icon](ICON.md) for loader and asset limits.
