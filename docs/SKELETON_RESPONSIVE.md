# Own-root responsive Skeleton presentation

Unreleased focused proof: strict native C11, four C/C++ header-order consumers,
real public responsive/contract tests, frozen legacy checks and pinned WASM
compilation passed. Root strict/runtime/distribution acceptance also passes;
modern five-binding authoring and publication remain pending. See the
[worker receipt](validation/skeleton-responsive-first.json) and
[root receipt](validation/skeleton-responsive-root.json).

Include `cui/cui_skeleton_presentation.h`. This is a separate additive route;
`cui_skeleton`, `cui_skeleton_ex` and `cui_skeleton_with_palette` are unchanged.

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
GPU/browser/native accessibility or full Styled/platform parity is claimed.
