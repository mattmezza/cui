# Typed Styled Slider: exact facade and presentation contract

Design checkpoint after signed style/seam `c7f1438a`, integrated as `384cd138`.
The declarations below are proposed, not implemented. This freezes step3's
previously descriptive records before production. The earlier
[complete source design](STYLED_SLIDER_MODEL_V1.md), its216 responsive geometry
and240 software ring proofs, and the exact Base request contract remain authority.
There is no new Core query, dispatcher, capture owner, callback transport or
legacy ABI change. All78 families remain present; five typed builders are future.

## Proposed declarations

New public header `cui_styled_slider_model.h`; includes the existing Base model,
owned style values and V2 support declarations. Exact v1 sizes are checked with
`sizeof`, without claiming identical 32/64-bit sizes. No field below is a pointer.
Existing Base options/state/handler/result are reused, not copied into a competing
input/reducer schema.

```c
typedef enum {
    CUI_STYLED_SLIDER_SOLID_V1 = 0,
    CUI_STYLED_SLIDER_BACKGROUND_V2 = 1
} cui_styled_slider_render_mode;
enum {
    CUI_STYLED_SLIDER_REDUCED_MOTION = 1u,
    CUI_STYLED_SLIDER_HIDE_FOCUS = 2u
};
typedef struct {
    uint32_t version, struct_size, render_mode, flags;
    uint32_t reserved[2];
    cui_slider_style_options_v1 style;
    cui_spring_settings spring_control;
    cui_draw_v2_support support; /* inactive in SOLID_V1 */
} cui_styled_slider_model_options_v1;
typedef struct {
    double position, velocity, target, updated_at;
} cui_styled_slider_model_motion_v1;
typedef struct {
    uint32_t version, struct_size, initialized_mask, previous_kind;
    cui_id owner;
    uint64_t last_admitted_frame;
    double last_clock, next_wake_seconds;
    cui_styled_slider_model_motion_v1 thumbs[2]; /* START=0, END=1 */
} cui_styled_slider_model_state_v1;
typedef enum {
    CUI_STYLED_SLIDER_RENDER_OK = 0,
    CUI_STYLED_SLIDER_INVALID_ARGUMENT,
    CUI_STYLED_SLIDER_INVALID_STATE,
    CUI_STYLED_SLIDER_UNSUPPORTED,
    CUI_STYLED_SLIDER_BASE_REJECTED,
    CUI_STYLED_SLIDER_CORE_FAILURE
} cui_styled_slider_render_status;
typedef struct {
    uint32_t version, struct_size, render_status, reserved;
    cui_slider_model_result_v1 base;
    double next_wake_seconds;
} cui_styled_slider_model_result_v1;
enum {
    CUI_STYLED_SLIDER_SURFACE = 0,
    CUI_STYLED_SLIDER_POINTER_RING = 1,
    CUI_STYLED_SLIDER_FOCUS_RING = 2
};
bool cui_styled_slider_model_options_from_theme_v1(
    const cui_theme *, float rem, uint32_t orientation,
    cui_styled_slider_model_options_v1 *out);
bool cui_styled_slider_model_state_init_v1(
    uint32_t state_size, cui_styled_slider_model_state_v1 *out);
cui_id cui_styled_slider_model_part_id_v1(
    cui_id owner, uint32_t thumb, uint32_t decorative_part);
cui_styled_slider_model_result_v1 cui_styled_slider_model_events_v1(
    cui_context *, cui_id owner, cui_slider_model_value_v1 confirmed,
    cui_slider_model_state_v1 *, const cui_slider_model_options_v1 *,
    cui_styled_slider_model_state_v1 *,
    const cui_styled_slider_model_options_v1 *, cui_slider_model_handler_v1);
```

**Explicit resolution for review:** options own spring policy as well as palette,
full radius and rem. The older descriptive design suggested borrowing a theme to
snapshot radius/spring. The complete example will instead snapshot
`theme.spring_control`, `theme.radius == 0 ? 0 : cui_radius(CUI_RADIUS_FULL, 1)` and
the explicitly chosen palette into this owned record before submission. This
matches the pinned source `Theme::radius_full` zero-radius rule and avoids a
second theme radius competing with the existing style value's full radius.
Custom authored full radius remains possible. No theme pointer survives or is
reread during callbacks. Changing this decision requires review before code.

The optional pure options constructor borrows theme only for the constructor
call, validates finite nonnegative theme radius and valid spring_control plus
positive finite rem/known axis, and copies the custom-theme palette using the
existing from_theme rule (no preset inference). It writes complete canonical v1
records, SOLID_V1, no flags, zero inactive support/overrides, spring_control from
theme and full radius0 for square or `cui_radius(CUI_RADIUS_FULL,1)` otherwise.
Horizontal defaults PARENT, vertical FIXED120px, matching source defaults. Axis
selects defaults only; the resulting options contain no competing stored axis.
Users can then choose a named owned palette or explicit dimensions/corners. The
constructor is context/allocation-free; invalid/null input preserves all output
bytes. It never stores the borrowed theme or changes a legacy theme struct.

The facade's nested `style.flags` must be zero: logical own drag and Base disabled
state derive ACTIVE/DISABLED after reduction. Rejecting authored nonzero flags
prevents a caller from painting a false interaction state. The standalone pure
resolver still permits those explicit flags. All other existing style tags,
length/rem/corner/color masks and inactive-field rules remain unchanged. Spring
response/damping must be finite and nonnegative, epsilon finite and positive;
travel=false and response=0 are valid snap policies. Unknown facade flags/mode,
nonzero reserved words and wrong version/size reject atomically.

Init accepts exact size, zeroes all bytes, writes version/size and otherwise owns
no context/value/capture. Invalid/null output stays unchanged. State validation
checks known initialized bits (START1, END2), finite active position/velocity/
updated_at, target exactly0 or1, nonnegative active times and last_clock/wake.
Uninitialized endpoint motion fields are inactive. With owner0, mask/frame/wake
must be zero; previous_kind and inactive motions are ignored. With nonzero owner,
previous_kind is Single/Range and last_admitted_frame nonzero. Single cannot retain
an initialized START. The caller reinitializes both reducer and presentation state
when recreating a context; a raw ID/frame counter is not global context identity.
No context pointer, heap storage, resources or bounds pointer enters this record.

## Preflight, one reduction, publication and failure

Validate/copy all borrowed active UI/model/options/strings before requests, including
existing Base state/domain/value/identity requirements, new state and nested style.
The future implementation should reuse/extract the existing private Base admission
validation rather than introduce a drifting second Base validator. Style/motion/
support/collision/visible-gradient rejection must precede calling Base. Failure
submits nothing, consumes no input, invokes no callbacks and leaves both states
and external owner unchanged. Return a versioned result, zero deadline and a
versioned Base result with no submitted/valid effects. For a Base-only rejection,
retain its exact passive result/status and use BASE_REJECTED. No fabricated counts.

Visible inner surface means painting is enabled, Base disabled is false and
`max(0, rem-2)>0`. V1 rejects a resolved LINEAR thumb with UNSUPPORTED before any
Base effects. Inspect the resolved brush, so an explicitly masked transparent
solid override is legitimate. Disabled omission and layout-only/zero-inner-area
geometry do not demand invisible gradient support; they never flatten it.

BACKGROUND_V2 copies the actual host's prior consumer declaration. Check exact
support size/ABI2/output1, only known background feature bits, required background
feature present, positive command/background/pixel/byte budgets, and at least the
number of visible endpoint backgrounds. Responsive final area/remaining frame
capacity cannot be preflighted from these declarations. The host must query a
real initialized consumer before begin and open the matching feature-admitted V2
frame. There is no current-frame capability getter and no component-generated
capability receipt. `cui_set_background` inside each painted inner surface remains
Core lifecycle/capacity authority. A lying/mismatched host contract can fail there
after reduction; no callbacks/input/owner effects are rolled back. Current live
adapters remain unsupported; fabricated-support tests prove transport only.

After preflight, call the private Base seam exactly once. Snapshot confirmed entry
and style before callbacks; Base handles all original-order proposals/cancellation,
acceptance remains external, and style never rereads accepted owner storage. The
in-box composer receives the same preview-or-confirmed paint pair as Base. External
application/userdata mutation and work enqueue are allowed; active UI, reducer,
presentation/options/style and borrowed string storage remain protected through
return. No callback request refers to presentation storage.

Stage presentation locally and publish only on a successful submitted Base result,
healthy current readiness and eligible phase. A synchronous Core failure returns
CORE_FAILURE, deadline0 and leaves staged presentation unpublished; Base/input/
callback effects already performed stand. A later end-frame/layout/renderer error
cannot roll back published effects. Hosts must ignore draw/deadline after failed
end and rebuild; no claim of physical presentation or AT delivery follows from
this result. Public NULL Base and every legacy Slider route remain untouched.

## Actual Core clock, lifecycle and input authority

Current Core advances `frame_time` by admitted finite nonnegative dt in
`cui_core_begin_frame`; if addition would overflow it retains the finite clock.
`cui_frame_time` reads that clock. `cui_frame_sequence` returns0 outside admitted
ordinary composition, including inert/layout-only ancestry; submission alone is
insufficient. `cui_frame_input_snapshot_ready` is the existing in-frame valid-prior
snapshot/current-error gate. Never synthesize a frame generation from time, stored
state or outside-frame readiness.

Presentation may advance only when real sequence!=0, readiness true, current
modal policy allows this coordinate-independent owner, Base succeeds and readiness
still holds after callbacks. Initial geometry, inert/layout-only, modal-blocked and
recovery-only calls paint cold confirmed geometry without advancing presentation
state or returning a wake. They preserve stored state; the returned deadline0 is
authority for this call, not a stale stored deadline. Layout-only composes size but
skips prepaint/background setters, which Core forbids in that phase.

First eligible call, owner change or nonadjacent admitted frame resets motion to
cold0 with zero velocity at the actual Core clock, then retargets. Use real wrap
adjacency UINT64_MAX→1, never0. If Base still rejects an old owner, no presentation
reset publishes. Admitted disabled calls clear motions cold and return no wake;
nonadmitted disabled calls retain state like other nonadmitted calls. Single
omits/clears START. Returning to Range initializes START cold; END can retain its
valid continuous motion. Omission/recovery gaps reset both. No intent replay or
geometry-only frame may renew admitted ownership.

The existing dispatcher suffices. After Base registers/reduces, actual enabled
thumb hover is `cui_hovered(thumb_id)`: prior clipped region, higher interactive
occlusion and prior modal policy, not ROOT/track hover or coordinate-only bounds.
Current modal gate also applies. Pressed ring target comes only from the actual
Base DRAGGING flag and active endpoint after reduction, including ROOT-selected
capture. Do not infer press from global pointer or decorate with an independent
capture. Same-batch down/up/cancel cannot stick pressed. Touch capture/keys/numeric
requests keep original targets, indices and existing Base rules.

Focus decoration reads current `cui_interaction_state` after Base registration;
require focused && focus_visible && !disabled, unless HIDE_FOCUS. ROOT key
continuation/focus fallback remain Base authority. Readonly/handlerless/zero-span
still have visible thumb/eligible hover/focus, but cannot manufacture drag or
value proposals. Decorations add no semantic or activation nodes.

## Motion, fractional rings and scheduling

Extract the existing private Switch spring arithmetic to a private owned-double
kernel; do not cast Slider motion to Switch/Disclosure structs. Switch remains a
fieldwise wrapper preserving tooltip and every current branch/clock/wake behavior.
Before extraction, record an independent signed-old-source trace across cold,
stationary, target reversals, repeated/backwards clock, zero dt, under/critical/
overdamping, large dt, extreme finite settings, travel=false and reduced motion.
Compare old/new values and Switch commands/IDs/events, not just a new helper to
itself. No public Switch structure/ABI or policy changes.

For Slider, sample old target through elapsed nonnegative time before retargeting
from final hover/logical press. First reset seeds an initialized cold kernel, so
normal first-hot retarget schedules travel instead of snapping hot accidentally.
Reduced facade flag OR Core reduced motion, travel=false or response0 snaps to
target/velocity0. Preserve finite internal overshoot, clamp visible progress to
[0,1], and settle exactly using policy epsilon. Backwards time never integrates
negative time or lowers the stored sample time. Numeric overflow settles safely
as the existing Switch kernel does; test those branches explicitly.

Return the earliest endpoint absolute next wake, nominally now+1/60 while moving,
otherwise0. If this increment rounds to now, use the next representable finite
future clock (coarser, never faster than60Hz). If no finite future exists, settle
at target and return0 rather than Infinity or an immediate busy wake. The result
is passive scheduling data: no unconditional `cui_invalidate` loop. The host uses
the deadline only after a successful end, and supplies actual elapsed dt on the
next frame. Settled/cold/omitted/suppressed frames produce no wake.

POINTER_RING is the named outward border box, inset by −3p with extent R+6p,
float stroke3p and radius0 for square or R/2+3p for round. Replace ring alpha by
.5p before quantization, irrespective of original ring alpha. At zero width,
register neither ring box nor prepaint; zero has no paint. Prepaint copies its
owned payload and emits a border from current resolved bounds only; command budget
remains bounded and it never claims input or semantics. Ancestor opacity applies
once through Core, as proven by the240 independent pixel cases. FOCUS_RING is the
explicit CUI addition:2px border/gap2, box inset−4/extent R+8, round radius R/2+4
or square0, authored palette ring alpha. It does not trigger pointer animation.

SURFACE retains1px padding and max(0,R−2) extent. Zero extent omits paint. Track
corner overrides affect only track; fill/outer/surface retain full radius. Source
bar normal/active alpha .2/.4 uses admitted own drag as the already documented
CUI selector policy; outer .5, fill unchanged. V1 quantizes each straight-linear
RGB channel through the sRGB transfer and half-up nearest byte, alpha independently;
hidden RGB at alpha0 is preserved. Do not use the sampler's alpha0 black fallback
or premultiply before encoding. V2 thumb copies the owned brush with no byte
roundtrip. Ordinary flow keeps both axes, responsive parent fill and literal −8/−5
source offsets. Vertical registered-scroll admission remains explicitly rejected.

The decorative getter validates owner/endpoint/part, derives through the existing
Base thumb identity helper and one shared private decoration derivation, and returns
identity only. No copied Base salts, MAIN alias, geometry lookup or false presence
check. Preflight verifies all submitted root/track/fill/thumb/decorative identities
are distinct (including current optional parts) before Base. Closed/Single START,
disabled/cold/focus-hidden/zero-area decorations may be absent despite a nonzero
getter. No previous public decorative IDs exist to preserve.

## Files, examples and bounded proof gates

Production ownership: new public facade header/component source, private source
geometry/header use, private shared spring helper and narrow fieldwise Switch
wrapper, focused tests and complete example/guide, source metadata and canonical
SDK/dist/skill generation. No Core/host/bridge source change. Keep reviewed
private NULL trace and existing Base/legacy tests, plus source/selective/umbrella
C11 and pinned WASM32 checks for new pointer-free records.

Tests must cover independent old/new Switch traces; actual source-preserving
style/colors/radii for four builtins/38 presets; V1 gradient atomic rejection with
queued pointer/key/touch/numeric inputs unchanged; disabled/layout-only/zero-inner
exceptions; honest V2 Core transport success and mismatch failure without claiming
an adapter. Real software pixels cover hover/press/focus, interrupted fractional
rings, alpha replacement, opacity, rounding and clip for both axes/sizes/modes.
Exercise original-order down/up, capture loss/cancel, ROOT chosen thumb, keyboard
focus, actual touch with identities, owner acceptance/rejection, readonly/handlerless,
inert/modal/omission/mode-change/reused-ID and current-error/clear/recovery timing.
Assert no callback replay, no presentation advance/wake when unadmitted, successful
idle settles and all rejected admission leaves outputs/states/events unchanged.

Provide a complete callback/application/compose/main companion based on the
already compiled controlled Range example, including checked init, explicit owned
palette/radius/spring snapshot, two-key external acceptance and simulated admitted
clock progression. Explain real host deadline scheduling; do not hide undefined
application/callback fragments or imply foreign/host authoring exists. Keep five
language builders, actual V2 adapters/native numeric AT and whole Slider closure
open until their separate implementations and runtime proofs land.

Signed design checkpoint: `/root/hidden_windows`, 2026-11-01. No production code,
generator output, compiler, dependencies or build artifacts accompany this contract.

## Implemented step3 followup

The declared records/functions are now implemented in the canonical public header
`cui_styled_slider_model.h` and component source. Consumer authority is
[STYLED_SLIDER_MODEL.md](../STYLED_SLIDER_MODEL.md), with exact portable CMake build
and windowless run commands and a complete compiled application. This updates the
historical checkpoint's proposed status; it does not close whole Slider parity.

Shared Base admission was extracted without mutation so facade-only rejection
precedes reduction. The public NULL route still matches the independent pre-hook
8×6-frame trace. The Switch numerical function was extracted to a private owned
value kernel, with fieldwise wrapper preserving its public state/tooltip. Before
editing the wrapper, the signed `c7f1438a` full component source was compiled and
its actual public Switch run against the retained identical Core/library:6 modes
of10 frames. Actual commands/order/colors/bounds, IDs, semantics and motion hashes
match the new wrapper exactly. A frozen old-source function fixture additionally
compares924 samples fieldwise (each double bitwise, bool separately), including
backwards/zero clock, interrupt, all damping regimes, extreme finite settings,
travel=false and reduced motion. No struct-padding comparison is used.

The final focused17-case private-Xvfb suite passes source/selective/umbrella facade
and pure-style builds, existing Base three routes/legacy events, the frozen NULL
trace, independent38-preset oracle/data, shared kernel, actual Switch trace, the
complete example and public-facade software pixels. Facade cases include4 builtins,
37 named solids and explicit V1 rejection of the1 named gradient,14 invalid-record
atomic checks,256 channel byte roundtrips, both axes/current parent resize/mode
changes, real keys/mouse/touch/ROOT capture/acceptance, readonly/handlerless/disabled,
modal/inert/layout-only/omission/current-error-clear recovery, extreme clock,
host-mismatched V2 failure and genuine Core V2 brush transport with declared
fixture support. The latter is not a consumer/renderer capability claim.

Actual public facade raster proof covers160 fractional pointer-ring images and16
keyboard-focus images, both axes, rem16/24, square/round, clip and ancestor opacity.
The independent annulus/source-over oracle checks exact outside coverage and at
most1-byte channel rounding inside, with zero-ring no paint. An initial focus
oracle expected87 at source alpha173/parent opacity.5; existing Core's prepaint
byte multiplication truncates to86. Only that independent expectation was corrected
(no production change or wider tolerance), with the initial failure log preserved.
Atomic record assertions snapshot original bytes with memcpy, rather than compare
padding after struct assignment. Computed style/brush equality is fieldwise.

Strict pinned Zig0.16 wasm32-wasi C11/O2/Wall/Wextra/Wpedantic/Werror compilation
passes the facade and refactored Base TUs. Observed Linux64 record sizes are
options328/presentation112/motion32/result80; these are observations, not a cross
ABI size promise. Local bounded builds reused actual cached Core/software/text/
CUI libraries for final links and did not rebuild the engine or open desktop
windows. SDK/dist/skill copies are generated canonically from the source.

Remaining boundaries: real V2 background consumers/Aurora rendering, vertical
registered-scroll admission, native numeric host/physical AT delivery and all
five typed builders. Existing source/CUI input exceptions remain Base authority.
