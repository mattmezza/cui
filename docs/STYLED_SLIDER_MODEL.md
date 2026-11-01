# Typed Styled Slider (unreleased, C only)

Include `cui/cui_styled_slider_model.h`. This additive facade uses the existing
controlled [Base model](BASE_SLIDER_MODEL.md) and the owned
[style resolver](SLIDER_STYLE_VALUES.md). Legacy float/double Slider APIs and all
old records remain unchanged. Full family closure is still open: visible Aurora
Light gradients need actual V2 adapters, vertical registered-scroll layout is
still rejected, native numeric hosts and all five typed builders remain pending.

Start with the complete, windowless
[controlled application](../examples/styled_slider_model_controlled.c). It contains
real application storage, checked init/default construction, callback, composition
and main; accepts two ordered key proposals externally, demonstrates actual hover
motion and advances the Core clock to a settled idle deadline. Reuse callback and
compose in your own native/browser host. This example does not create a window,
install a renderer or imply a foreign-language builder exists.

From the repository root, with a C11 compiler, CMake≥3.20, Ninja and Python3
([dependency guide](DEPENDENCIES.md)), build and run only this example:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build --target test_styled_slider_example --parallel 1
./build/test_styled_slider_example
```

On Windows the Ninja executable has the `.exe` suffix. Expected output is
`Selection [0.250, 0.500]; accepted 2 requests`. It is windowless, so direct
execution is safe; other native automated tests still use the private-display
`python scripts/test_local.py --build-dir build` wrapper. Native dependencies,
a font and a display are unnecessary for this target.

```c
cui_theme theme = cui_theme_light();
cui_styled_slider_model_options_v1 style;
cui_styled_slider_model_state_v1 presentation;
if (!cui_styled_slider_model_options_from_theme_v1(
        &theme, 16, CUI_SLIDER_MODEL_HORIZONTAL, &style) ||
    !cui_styled_slider_model_state_init_v1(sizeof presentation, &presentation)) {
    /* Application handles invalid construction. */
}
```

The constructor copies custom-theme palette values (no preset inference),
full radius0 for a square theme or9999 otherwise, and spring_control. Horizontal
defaults to parent fill; vertical to fixed120px. The axis argument selects defaults
only; the existing Base options remain orientation authority. Rem16 gives a24px
hit cross extent,6px track,16px outer thumb and14px inner surface. Padding1,
axis offset−8, cross offset−5 and maximum pointer-ring width3 are literal pixels.
They deliberately remain uncentered at other rem values. Select a named palette
explicitly with `cui_slider_palette_preset_get`; do not infer it from theme bytes.

Options and presentation state are owned, pointer-free exact version1/sizeof
records. Wrong tags/sizes, unknown flags/masks, nonzero reserved words or invalid
active values reject before input/state/UI/callbacks. Nested style.flags must be0:
the facade derives ACTIVE from admitted own drag and DISABLED from Base options.
The independent pure resolver still accepts explicitly authored style flags.
Inactive fields follow their respective style/support contracts. Spring response
and damping are finite/nonnegative, epsilon finite/positive; travel=false or
response0 snaps. Initialize reducer state separately with its existing helper.
Reinitialize both states when recreating a context; IDs and frame counts are local.

The existing Base handler is synchronous and retains all original-order typed
Single/Range/double/domain/input evidence. It never accepts owner values for you.
`current` stays the entry snapshot; `requested` intent evolves across events.
Callback/userdata may update external owner state and enqueue application work,
but must not mutate active UI/reducer/presentation/options/string storage or
recursively submit. Paint uses ongoing drag preview and otherwise confirmed entry;
external acceptance appears on the next submission. CANCEL may carry an old
kind/domain and must not be blindly accepted. Styled does not change Base's
pointer-down proposal, keyboard, touch cancellation or request-phase policies;
source behavior exceptions remain in the linked Base contracts.

`cui_styled_slider_model_part_id_v1(owner, thumb, part)` returns pure identity for
SURFACE, POINTER_RING or FOCUS_RING, deriving through the Base thumb helper.
Root/track/fill/outer thumb IDs keep the existing Base getter. Nonzero identity
does not establish presence: Single omits START, disabled omits visual thumb
children, zero inner extent omits surface, zero pointer-ring width omits its box,
and unfocused/hidden focus omits focus decoration. Decorative boxes add no
semantics, capture or input consumption. Track corners can override independently;
fill/outer/inner retain full radius. Disabled keeps transparent outer semantic
geometry, while track/fill preserve their palette. Readonly/handlerless/zero-span
retain visible thumb and eligible hover/focus but never fabricate drag proposals.

Actual clipped/occluded prior-thumb hover or the final admitted logical own drag
retargets its interruptible pointer-ring spring. Track hover alone does not. Same
batch up/cancel cannot leave pressed stuck. Visible progress clamps to[0,1] while
internal motion can overshoot. The ring is a fractional outward border of width3p;
its alpha is REPLACED by .5p, not multiplied by authored ring alpha. Core applies
ancestor opacity once. Existing Core truncates the resulting alpha byte after
that multiplication; the facade does not apply or round it again. Keyboard-visible focus gets a separate2px border with2px
gap, using the authored ring alpha; HIDE_FOCUS suppresses it. It does not animate
the pointer ring. REDUCED_MOTION or Core reduced motion snaps motion to target.

Scheduling uses `result.next_wake_seconds`, an absolute accumulated Core clock
deadline, zero when idle. Wait until that deadline in your host's frame scheduler
and supply actual elapsed dt to the next begin. The facade does not spin an
unconditional invalidate loop. Only use draw/deadline after successful end_frame.
Initial geometry, inert/layout-only, modal-blocked or recovery-only calls return
no wake and do not advance presentation state. Sequence0 is never converted to
an invented generation. Omission/owner/recovery gaps reset cold; Single clears
START, which initializes cold on returning to Range. Admitted disabled calls reset
cold; nonadmitted calls preserve state. Clock reversal never integrates negatively.

SOLID_V1 encodes straight-linear RGB to sRGBA8 with independent half-up alpha
quantization. Visible LINEAR thumbs return UNSUPPORTED before Base submission,
state changes, input consumption or callbacks. An explicitly masked solid override
is allowed, including transparent. Disabled/layout-only/zero-inner-area geometry
needs no invisible gradient rendering. No gradient is flattened to a byte color.

BACKGROUND_V2 requires a copied declaration from an actual initialized consumer
queried before begin and a matching feature-admitted Core V2 frame. The facade
checks known size/ABI/output/features/budgets, then uses the checked Core setter
inside each inner surface. It cannot inspect remaining frame capacity or verify
a host's claim through a fictional Core capability query. Current software/SDL/
GPU/Sokol/Canvas adapters do not support this transport; tests with fabricated
support prove Core transport only. A lying host or later capacity/layout/renderer
failure can occur after callbacks: no already delivered request/input/owner effect
is rolled back. CORE_FAILURE returns no wake and does not publish staged motion;
a later end failure cannot roll back earlier state. Rebuild from a healthy frame.

## Browser gallery authoring

The Slider gallery uses this controlled facade for Single/Range, horizontal or
vertical, reverse and linear/log input. The owner policy control explicitly
accepts or rejects UPDATE proposals; the preview shows confirmed owner and copied
current/requested/phase. Mode/domain/axis changes synchronize the demo owner,
retain the reducer for captured CANCEL delivery and reset only presentation.
CANCEL is not blindly accepted. Progress retains its independent legacy route.

The copy panel is a complete C initialization/callback/composition example:
`web/modules/slider_usage.js`. Keep the application record across frames,
render the completed draw list, then schedule its absolute Core-clock deadline.
The browser maps that deadline to its monotonic host clock and coalesces it with
other wakes; reduced motion and settled state return zero. No manual animation
loop is required. V1 reports unsupported only when actual resolved visible paint
is linear; a theme name alone never selects gradient capability. This gallery
migration does not implement native numeric delivery, typed foreign builders,
V2 gradient adapters or vertical registered-scroll admission.
