# Skeleton palette and fixed-size presentation

Unreleased focused proof: strict native C11 and C/C++ headers, actual C38-preset
oracle, software pixels, frozen compatibility/callback timing, and pinned WASM
compilation passed. Responsive composition is test-only, not a public API.
Root strict integration build and 17 targeted private-Xvfb checks pass, including
selective/umbrella distributions, C/C++ include orders and the actual 38-preset
C oracle (four methods, zero skips). All 18 consumer-skill tests and canonical
generated-file checks pass. This is not a whole-family, deployed-gallery or
installed-package support claim.

Include `cui/cui_skeleton_palette.h` for owned values and pure resolvers, and
`cui/cui_content.h` for `cui_skeleton_with_palette`. The existing phase API and
`cui_skeleton_ex` retain their prior behavior. None of their records grow.

```c
cui_skeleton_palette palette;
/* Explicit custom-theme fallback; never guesses a preset from theme bytes. */
if (!cui_skeleton_palette_from_theme(&theme, &palette))
    return;
/* Alternatively: explicit registry index, checked against existing preset count. */
if (!cui_skeleton_palette_preset_get(preset_index, &palette))
    return;
/* Application owns persistent state, separate for each independent placeholder. */
cui_skeleton_options options = cui_skeleton_options_default();
options.now_seconds = host_absolute_seconds;
options.width = 180;
options.height = 16;
options.radius = 4;
options.reduced_motion = application_reduced_motion;
cui_loading_motion_result result;
if (cui_skeleton_with_palette(ui, placeholder_id, &palette, &motion, options, &result)) {
    /* Schedule this absolute deadline in the SAME clock domain; zero means idle. */
    next_wake_seconds = result.next_wake_seconds;
}
```

The snippet belongs inside an existing admitted CUI frame; `motion` is initialized
once to zero by the application, not recreated per frame. `theme`, clock, ID and
host scheduling are application-owned. No new loader, clock or retained object.

A palette is one owned RGBA8 color. `from_theme` copies `theme.secondary` exactly,
including alpha0. Builtins use their actual CUI secondary token. `preset_get` uses
the existing38-name preset order and pinned `skeleton.background`, falling back
to raw source `secondary.background`. All four bytes are preserved. These are CUI
encoded paint bytes, not a source floating shader/color-space identity guarantee.
No global theme change or inference from preset-looking custom themes occurs.

Custom color is simply `cui_skeleton_palette p = {{21,34,55,0}};`.
Transparent color is authored, never an absent override. Source secondary
multiplies alpha by .5; pulse opacity1→.5→1 then a single byte truncation matches
existing ex. Parent opacity/clip remains the existing Core layer. Palette input
is snapshotted before easing (unlike the preserved legacy ex post-callback theme
read), and borrowed through submission only; copies need no source lifetime.

Nulls/invalid index leave pure resolver output unchanged. Painter requires nonzero
ID, positive finite dimensions<=4096, radius0..4096 and duration0..60 (zero uses2s).
It reuses the existing bounded motion sampler: state/clock/easing rules and reduced
motion behavior are unchanged. Failed prevalidation/sampling leaves state, result
and paint unchanged; easing callback effects cannot be undone. State/result must
not overlap inputs/each other. Submission success does not certify later layout,
rendering or native/AT delivery. Caller checks the existing frame diagnostic.
Omission keeps caller motion state; reset it explicitly to restart.

This fixed-size API does not supply source full/fraction/flex root sizing, own-root
Styled border/layout overrides or modern foreign options. Python, JS/TS, Go and
Rust retain their existing legacy Skeleton constructors and timing; none exposes
this new palette. Application-owned busy/status semantics remain as documented in
[the semantic audit](design/ALERT_SKELETON_SEMANTICS_AUDIT.md). See
[the parity checkpoint](design/SKELETON_PARITY_CHECKPOINT.md) for remaining scope.

Focused proof receipt: [skeleton-palette-first.json](validation/skeleton-palette-first.json).
The initial omission test incorrectly assumed an animation started at zero; the
corrected fixture preserves its actual first sample time. An initially skipped
C-oracle method was corrected to check its optional dumper at runtime; the final
run executed all four oracle methods with no skips. Logs retain both earlier
failures/limitations rather than presenting those runs as complete proof.

Integrated proof: [skeleton-palette-root.json](validation/skeleton-palette-root.json).
