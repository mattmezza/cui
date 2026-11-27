# Own-root responsive Skeleton presentation

Alpha.4 ships the C presentation API. Strict native C11, four C/C++ header-order
consumers, responsive/contract tests, frozen legacy checks and pinned WASM
compilation passed. The retained bridge and Python, JavaScript, TypeScript, Go and
Rust builders shipped responsive options in alpha.5. See the
[worker receipt](validation/skeleton-responsive-first.json) and
[root receipt](validation/skeleton-responsive-root.json).

Include `cui/cui_skeleton_presentation.h`. This is a separate additive route;
`cui_skeleton`, `cui_skeleton_ex` and `cui_skeleton_with_palette` are unchanged.

## Loading a profile, article list or table

The gallery's **Layout** control offers three C recipes. **Profile card** combines
a circular avatar placeholder, two fractional text bars and a full-width block.
**Article list** pairs a square thumbnail with two lines per row. **Table rows**
keeps the column headers visible and gives the first column the remaining width
after reserving a fixed-width status column. Choose **Base size** to scale the
shapes, text and spacing from an application-owned logical rem.

The **Theme** appearance reads the active preset's owned Skeleton palette.
**Secondary** halves its alpha. **Accent** uses the theme's primary color as an
explicit override. For a custom theme, `cui_skeleton_palette_from_theme` copies
`theme.secondary`. An independently authored Skeleton color can be passed in
the owned palette. The strict legacy ThemeSet importer currently rejects
`skeleton.background`; partial imports skip it and report an unsupported field.

Choose **Loaded** to replace the shapes with a profile, article titles or file
names and statuses. Each recipe keeps its surrounding region and announces the
completion through an application-owned status. Placeholders are decorative
and add no focus stops or accessibility nodes. Each shape owns independent
motion state. The host schedules the earliest positive deadline and goes idle
when the ready content replaces the placeholders.

**Download the C recipe header** gives you the public-API implementation used
in the preview. **Copy code** gives you the call for the selected layout, scale
and paint. Include the downloaded header, keep one zero-initialized
`skeleton_example_state` per instance, and pass your own loading flag and a
stable key. The source is also available in [the repository example](../examples/skeleton_recipes/scene.h).

```c
/* Persistent application-owned state, one per independent placeholder. */
cui_loading_motion motion = {0};
cui_skeleton_palette palette;
/* In application setup: explicit custom-theme fallback, no preset inference. */
bool ready = cui_skeleton_palette_from_theme(&theme, &palette);

/* In each admitted ordinary UI frame while loading: */
if (ready) {
    cui_skeleton_presentation p = cui_skeleton_presentation_default();
    /* Default actual root: full parent width, height16, radius0, period2seconds. */
    p.root.radius = 4;
    p.now_seconds = host_absolute_seconds;
    p.reduced_motion = application_reduced_motion;
    cui_loading_motion_result result;
    if (cui_skeleton_present(ui, placeholder_id, &palette, &motion, p, &result))
        next_wake_seconds = result.next_wake_seconds;
}
```

Setup and frame fragments are intentionally separated: do not reinitialize motion
on each frame. For complete compilable authored Core composition, see
[the public API fixture](../tests/unit/test_skeleton_responsive.c), whose `main`
creates the context/software renderer, composes and resizes the actual root, and
exercises registered scrolling. [The contract fixture](../tests/unit/test_skeleton_presentation_contract.c)
contains real callback/config snapshots, grow/fraction/fixed sizing and failures.
Build and run these examples headlessly from a configured repository:

```sh
cmake --build build --target test_skeleton_responsive test_skeleton_presentation_contract
python scripts/test_local.py --build-dir build -- -R '^skeleton_(responsive|presentation_contract)$' --output-on-failure
```

The painted own root receives `cui_box` layout directly. Defaults use percent width1,
not a fixed160px child or a growing wrapper. Under a definite parent, set
`p.root.width=.5f` for half width; choose grow with `width=-1,width_percent=false`,
or fixed120 with `width=120,width_percent=false`. Core's existing fraction/fixed/
max precedence, explicit zero, FIT, padding, edges, corners, clip and offsets apply.
No prior-frame bounds, input replay or second dispatcher. Full width under an
unconstrained/FIT parent follows Core behavior; no implicit viewport-width guess.
Height16 is a logical-pixel source default; application rem scaling is explicit.

Source paint precedence is deliberate:

1. Start with the owned palette; secondary halves its alpha.
2. With `custom_background=true`, replace that base with `root.background` entirely,
   including alpha0. The custom replacement is unaffected by secondary.
3. After synchronous motion sampling, animation replaces root opacity. Caller
   `root.opacity` and `custom_opacity` are inactive, even with NaN/zero. Fade a group
   via its ancestor. Border and background both receive pulse and parent opacity.

Presentation and palette values snapshot before custom easing. Userdata may mutate
external application values; this call still paints its captured style. The callback
runs once synchronously on admitted sampling and is never retained. Reduced motion
skips easing and returns opacity1/no wake. Custom alpha0 never requests fallback.
Encoded secondary alpha then Core root opacity may differ by one byte from the
old fixed painter's single background-alpha truncation. This opt-in source-ordered
route does not change or redefine old output.

State and result are caller-owned disjoint writable objects; they must not overlap
borrowed inputs. Invalid mandatory input/zero ID, duration or motion sampling fails
without state/result/paint changes. Callback effects cannot be rolled back. Other
root fields follow existing Core normalization. Success means component submission;
inspect completed-frame diagnostics for later layout/capacity/render failures.
Later Core errors do not undo state or callback effects. Submit during active
ordinary composition, not from prepaint/input/easing callbacks into the same UI.

Schedule the finite absolute `result.next_wake_seconds` in the same domain as
`now_seconds`; zero is idle. Omission preserves motion state; reset to zero explicitly
to restart. Layout-only/inert follows Core suppression, without implying motion
rollback. Skeleton is decorative: applications own busy/status and content replacement.

Registered Y bodies with fixed Skeleton height/full width passed the real public
API first-frame/resize tests. Percentage HEIGHT remains rejected
by existing registered scroll admission. A cleared diagnostic cannot make that
failed geometry valid; rebuild a fresh healthy frame. No Core admission widening,
GPU/native accessibility or full Styled/platform parity is claimed. The gallery
browser test verifies responsive dimensions and busy/loaded semantics at desktop
and mobile width, plus preset, secondary and accent pixels.

The [retained C bridge](BRIDGE_SKELETON.md) exposes a copied configuration and
owns the per-node pulse. It keeps the direct frame API separate.
