# Skeleton parity checkpoint

Source-only audit at CUI `956a853c`. No production, manifest, generated output,
compiler or runtime changes; existing evidence below was inspected, not rerun.
Keep the family open for the identified theme/layout/binding work. A `tested`
manifest row is not a claim that every source feature or platform is complete.

## Frozen authority

GPUI Kit0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`:

| Primary file | SHA256 of fetched exact bytes |
| --- | --- |
| [Skeleton source](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/skeleton.rs) | `1f8818cef643e2e7160151be4516331f28cddd255762d1bd0af2777561073c8d` |
| [Component guide](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/website/component/skeleton.md) | `da87bfccdb5f32831a146db75706f6b41aa5f511fab70956142ea8fac3fa4adb` |
| [Theme schema](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/schema.rs) | `d700cb811d96ed075cdec2df616e0b20321ae6488d2dec36e3fbf85b8892ba23` |
| [Theme color](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/theme_color.rs) | `2d42060c00a0cb0fc91d698a4111c6df2cd80d074a1ed87eb03771e027da38f1` |

Only these specific files and theme module declaration were fetched into
`/tmp/cui-skeleton-reference`; no checkout/dependency installation. The guide
hash matches `docs/gpui-parity.json`. Component source lines10–13 contain only
style refinement and secondary flag; lines40–60 establish full width, height4,
theme skeleton color, caller refinement, then repeating two-second opacity.
Theme schema lines533–535 and995 explicitly resolve `skeleton.background`,
fallback `tokens.secondary`; this is not the border token. ThemeColor stores
Skeleton as Hsla, so this family does not require gradient transport.

## Coverage, gaps and exceptions

| Feature / exact source behavior | Current implementation and inspected proof | Assessment |
| --- | --- | --- |
| Decorative placeholder; no action, focus stop, loading flag or content relation | `src/components/cui_content.c:994` submits one box; `tests/unit/test_status_motion.c:155` asserts zero semantics | Implemented. Do not add per-placeholder busy/status semantics to imitate nonexistent source behavior. |
| Primary theme skeleton color, fallback secondary | `cui_skeleton_ex` always uses `theme->secondary` (`cui_content.c:1011`); legacy uses border at903 | Concrete missing dedicated source token. Secondary fallback works only when no explicit source token. Legacy appearance must remain frozen. |
| Secondary variant multiplies color alpha by .5; animated element opacity1→.5→1 | ex multiplies existing alpha by result opacity and secondary factor; shared sampler `src/base/cui_status.c:344` | Implemented byte-color adaptation, not exact floating-Hsla compositing. Existing test submits secondary but does not independently inspect its rectangle alpha across the cycle. Need dedicated oracle, including translucent authored colors. |
| Two-second repeat, bounce(ease_in_out) | ex defaults2seconds, triangle then quadratic ease, caller absolute clock/state; `test_status_motion.c:52–87` samples endpoints/custom ease/errors | Implemented tested subset; source kernel equivalence at interior/boundary times deserves independent dense oracle. Legacy cosine pulse is intentionally different. |
| Source new() full parent width, height4 (16px default rem) | ex defaults160×16, radius0; legacy fallback160×16; bridge fallback120×16,radius4 | Missing source responsive default. Current APIs explicitly fixed positive dimensions, not source full-width defaults. Width0 is rejected by ex, so wrapping a grow parent does not make its painted child fill. |
| Width fraction/full, flex, explicit sizes, height utilities/rem | Core `cui_box` exposes percent/grow/fixed sizing; Skeleton facade exposes only positive float width/height <=4096 | Underlying composition exists, but not equivalent own painted-root sizing. Need additive direct root sizing; do not claim a percentage wrapper stretches current fixed child. Rem can be caller-computed now, with no named Skeleton size source enum required. |
| Square/circle/rectangle, arbitrary rounding | ex width/height/radius, renderer radius clamp; legacy content test checks rectangle120×16 | Basic capability exists; no dedicated current proof for circle pixels, square corners, fractional radii or resize/clip. Source has no separate shape enum. |
| Styled refinement overrides own background, border, spacing, corner/layout properties before animation | No custom background/style descriptor in ex; parent box can compose layout, inherited opacity/clip but cannot replace child color | Partial. General source Styled surface exceeds existing fixed facade. Parent composition is valid only where it preserves actual root geometry/paint. Do not mark all Styled properties implemented. |
| Conditional loading and profile/list/table/button/form placeholders | Application conditionally submits multiple Skeletons and real content; `examples/loading_region/scene.c` provides busy-region composition | Composition capability exists; not separate component modes. Full responsive example matrix is not demonstrated by current gallery. |
| Meaningful loading semantics | `docs/design/ALERT_SKELETON_SEMANTICS_AUDIT.md`, loading_region Core/DOM/AT-SPI fixtures establish application region/status semantics | Existing reviewed justified application ownership. Physical screen-reader delivery remains separate; no Skeleton input/keyboard work is missing from decorative source. |
| Reduced motion | ex merges caller and Core preference, returns opacity1/no wake; browser `tests/web/test_status_motion.py:26–52` checks actual changing pixels then idle | Intentional accessibility extension: source guide says animation cannot be disabled. Preserve quiescence and document difference. |
| Scheduling, omission/reinsertion, lifetime | caller-owned `cui_loading_motion`, finite next wake; gallery case18 translates Core deadline to host clock; no state allocation | Scheduler tested, but state persists across omission unless caller resets. This is explicit CUI ownership, not source element lifecycle parity. Add omission/resume/reset proof; never fabricate frame time. |
| Native/WASM rendering | status test includes software renderer; existing browser fixture checks changing canvas and no busy/status | Focused proof, not all renderers, themes, shape pixels, DPI or physical GPU/native window delivery. No new platform support inferred. |
| Shadow ownership | caller ID is painted Skeleton root; `docs/design/ELEMENT_SHADOW_COVERAGE.md` Skeleton row | Existing root identity. No separate source glyph/highlight owner to invent. Layout/painter changes must preserve IDs and ordering. |

### Theme counterexample is concrete

The retained pinned `third_party/gpui_themes/presets.json` contains explicit
Skeleton values for Default Light (`#f5f5f5ff` vs secondary `#e5e5e5ff`), Aurora
Light (`#f1f5f9ff` vs `#e2e8f0ff`) and Default Dark (`#171717ff` vs `#262626ff`).
Other presets need schema fallback, not guessed identity. Current theme public
record has no Skeleton field; palette adaptation discards this component token.
Changing legacy theme.secondary globally would incorrectly recolor other families.

### Public layers and foreign authoring

There is no separate public Base Skeleton widget/reducer. The portable shared
motion sampler lives in Base (`cui_status.c`) but its declaration is currently
in `cui_content.h`; Styled painting is in Components. Preserve Core→Base→Components,
no platform/Clay types, and old by-value options layout.

All five bindings currently expose bridge kind35, not `cui_skeleton_ex`:
`src/bridge/cui_bridge.c:4938` calls legacy `cui_skeleton` with Core-clock phase,
fixed radius4, primary variant, no explicit reduced-motion argument and continuous
legacy invalidation. Python `__init__.py:1247`, Node `index.cjs:315`, TypeScript
`index.d.ts:114`, Go `native.go:2674`, Rust `lib.rs:830` expose constructors;
generic visual sizing supplies dimensions. Their `tested_subset` rows are
legitimate legacy construction coverage, not source-motion/color/secondary/
responsive-style authoring. `bindings/node/test/extended.test.cjs` and Go
`native_test.go:481` check sizing/construction, not these omitted features.
Do not silently migrate old bridge kind35 or its repaint cadence.

Gallery `web/gallery.c:5242` uses ex with width computed from external preview,
height36/radius6, clock/reduced motion and host wake; it demonstrates a single
rectangle, not default source fill, secondary variant or layout compositions.
Source metadata `docs/component-usage.json` Skeleton still indexes only legacy
APIs/phase and no state, unlike actual ex implementation. This is a discoverability
gap to repair with the eventual verified slice, not evidence ex is absent.

## Proposed independently reviewable work

1. **Recommended first: additive owned palette/style values and direct C paint.**
   Before production, specify exact version/size/reserved and atomic failure
   contract for a new source-specific palette resolver plus additive Skeleton
   presentation entrypoint; leave `cui_theme`, existing options and legacy APIs
   unchanged. Resolve all4 builtins+38presets from pinned schema, explicit token
   versus secondary fallback, and custom theme from supplied values without
   preset inference. Owned color is sufficient for this Hsla source family;
   preserve alpha and documented byte conversion, no brush/gradient architecture.
   Reuse the existing sampler and root ID. This fixes actual wrong colors while
   keeping responsive sizing as explicitly outstanding, not falsely closing family.
2. **Responsive own-root presentation.** Review a borrowed submission descriptor
   that applies existing Core box sizing/corners/style before paint, with explicit
   defaults full width/16px and application rem policy. Investigate ordinary flow
   and percent sizing first; no new dispatcher/layout architecture. Reject any
   unsupported registered-scroll combination explicitly rather than expanding
   Core admission without review. Preserve fixed-route command compatibility.
3. **Genuine authored foreign UI and gallery.** Add optional bridge-owned motion/
   options with bounded copied records and five equivalent builders; exact ABI,
   owner-thread, wake and lifecycle policy need their own checkpoint. Keep legacy
   constructors unchanged. Gallery should show secondary/color/shape, full/fraction
   width on narrow/wide resize and application loading→content transition with
   copyable UI code. Update human/agent metadata only after actual proof.

## Required proof before implementation claims

* Independent frozen-data palette oracle for all presets/builtins/custom fallback;
  do not calculate expected output through candidate helper. Verify token absence,
  transparent authored colors, failed indices/config and unchanged outputs.
* Exact source pulse oracle at0,.25,.5,.75,1, interior/random/boundary/repeated
  cycles, backwards clock, independent states, invalid ease and reduced motion;
  fieldwise comparisons or byte snapshots for atomic unchanged outputs, never
  struct-copy padding equality.
* Actual software pixels/rounded bounds for normal/secondary translucent colors,
  circles/squares/rectangles, clip/inherited opacity; first-frame and narrow/wide
  resize fill/fraction geometry and stable root IDs. Test source border/custom
  paint precedence if exposed. Keep legacy cosine/border commands frozen.
* No Skeleton focus/busy/role, application busy/status transition and omission
  reset/resume; scheduler wakes only while needed, reduced-motion idle stable.
* Actual five-runtime authored UI, options/status/old-library omissions, immutable
  copied wire data and optional-symbol fallback once bindings are implemented.
  Strict C11, selective/umbrella C/C++, WASM compile, private-Xvfb native and
  headless browser tests under a separately granted serial lease.

No tests were added merely to assert current known gaps, and none were executed
for this audit. No status promotion or all78 ledger edit belongs to this checkpoint.

## Exact additive palette/presentation proposal (review required)

This is a proposed next slice, not shipped declarations. Keep the existing
`cui_theme`, `cui_skeleton_options`, phase API, ex API and shared sampler ABI and
behavior untouched. No version/size/reserved fields or new retained object are
needed for a new plain owned C value. No common brush/style framework is justified
by this single-color family.

Proposed new portable header `include/cui/cui_skeleton_palette.h` includes only
`cui_theme.h`; C/C++ linkage guards follow existing headers:

```c
typedef struct {
    cui_color background;
} cui_skeleton_palette;

/* Fallback adaptation, never infer identity from equal theme bytes. */
bool cui_skeleton_palette_from_theme(const cui_theme *, cui_skeleton_palette *out);
/* Index and lifetime/order authority: existing cui_theme_preset_count/name. */
bool cui_skeleton_palette_preset_get(size_t index, cui_skeleton_palette *out);
```

Proposed additive declaration in `cui_content.h` (includes the new value header):

```c
bool cui_skeleton_with_palette(cui_context *ui, cui_id id,
                              const cui_skeleton_palette *palette,
                              cui_loading_motion *state,
                              cui_skeleton_options options,
                              cui_loading_motion_result *out);
```

### Pure resolution, source interpretation and ownership

* `cui_skeleton_palette` owns one encoded RGBA8 CUI paint color; no pointer,
  context, preset index, color identity tag, hidden allocation or source lifetime.
  Its layout is the layout of its sole `cui_color` member (target ABI tests, not a
  new hardcoded platform-width assertion). All channel combinations are valid.
* `from_theme` copies exactly `theme->secondary`, including alpha and hidden RGB
  at alpha0. Null theme/output returns false without changing output. Other
  theme fields are irrelevant and need not be validated. Custom theme changes
  do not discover a preset; no registry search, name guessing or matching bytes.
* Custom authored color uses an explicit value, for example
  `cui_skeleton_palette p = {.background = {21, 34, 55, 0}};`.
  There is no "unset" zero sentinel: alpha0 is authored transparent paint, never
  replaced by theme fallback. A separate bool/override mask is unnecessary when
  the whole supplied palette is active. Callers wanting fallback explicitly call
  `from_theme` first, then optionally overwrite the owned member.
* All four CUI builtins are covered by `from_theme(&builtin, &out)` using the
  actual existing light/dark/high-contrast/terminal constructors. They are CUI
  adaptations, not aliases of similarly named upstream presets. No new builtin
  enum or four duplicated selection functions.
* `preset_get` uses precisely the existing38-preset `cui_theme_preset_get` index
  order; out-of-range/null output returns false unchanged. Resolve pinned raw
  `palettes.json` values: `skeleton.background` if present, else
  `secondary.background` (schema's `tokens.secondary`), then named-color lookup.
  Inspection of the exact retained source found secondary.background in all38
  and only solid hex values for the selected Skeleton/fallback tokens. Preserve
  explicit alpha; #RRGGBB means255, #RRGGBBAA copies all four bytes. Do not use
  current theme adaptation output as the source oracle, synthesize border blends,
  or silently accept a future unsupported token/brush syntax. Generator must fail
  on changed pin/order/missing fallback or unreviewed syntax. No gradient renderer
  requirement is introduced by this pinned source.
* "Exact source token" here means exact selected token/channel bytes and fallback,
  not GPUI shader floating-Hsla/linear compositing bit identity. This entrypoint
  deliberately uses existing CUI RGBA8 rectangle rendering and inherited-opacity
  conversion. It must not claim source GPU pixels or introduce a new color-space
  transport. A future float pipeline would be a separate reviewed API.
* Pure resolver writes only after resolution succeeds; no allocation, locale,
  global mutation, context diagnostic or renderer access. Output is immediately
  copyable; no loader/library record pointers are returned.

### Presentation and compatibility

The new painter snapshots `palette->background` and reuses the existing Skeleton
validation, sampler and box emission implementation through a private color-value
helper. Existing ex still supplies `theme->secondary`; legacy phase API still
supplies border/cosine/invalidation exactly. No public old record gains fields.
No global theme.secondary replacement, switch to source preset from theme bytes,
Core query, dispatch, native transport or bridge ABI change.

* Mandatory nonnull ui/palette/state/out and nonzero id. Dimensions/radius/duration,
  clock/state/easing validity and reduced-motion resolution are exactly existing
  `cui_skeleton_ex` rules: positive width/height<=4096, radius0..4096,
  duration0 selects2 else finite0..60, sampler now0..1e12. Reuse its actual
  validation; no new divergent reducer. All options remain by-value and easing
  userdata is borrowed only during synchronous sampling, never retained.
* Palette color is multiplied by `(options.secondary ? .5f : 1)` and sampler
  opacity using the existing float calculation/order, then a single uint8_t
  truncation as current ex. Do not quantize secondary first. Alpha0 remains0,
  RGB is preserved. Source component's Hsla secondary opacity multiplies alpha;
  inherited parent Core opacity remains a separate existing layer, not a palette
  rewrite. Out result describes motion, not effective post-parent alpha.
* Same caller ID, single box, width/height/radius, clip/ancestor opacity and draw
  ordering as existing ex; no semantic node, busy region or extra focus stop.
  A transparent rectangle may be culled by existing rendering; do not invent a
  visible paint/presence guarantee. Custom palette is active even when invisible.
* Prevalidation/sampler failure returns false with state/out and draw stream
  unchanged (easing callback side effects cannot be rolled back). Success means
  accepted component submission as with existing ex, not completed Core layout,
  renderer success, native presentation or physical accessibility delivery. A
  later Core/frame failure does not roll back sampling or already performed
  application effects. Caller still checks existing Core error/frame result.
* Caller owns persistent motion state; state/out must be distinct valid writable
  objects, not overlapping each other or borrowed inputs. No general alias/in-place
  support is promised. Palette need live only through this call; snapshots/copies
  may be kept indefinitely. Start/reset/omission/reinsertion remain explicit caller
  policy; the new palette does not create a new animation owner.
* Caller schedules absolute `out->next_wake_seconds` in the same clock units as
  options.now_seconds. Reduced motion suppresses easing and returns static/no wake
  exactly as existing Skeleton sampler. No implicit perpetual invalidation.

### Implementation, proof and deliberate boundaries

Proposed ownership after review: new header/source `src/theme/cui_skeleton_palette.c`,
pinned generated color table and narrow generator/data oracle; additive content
header/Components helper+painter; focused new tests/docs and canonical metadata
when granted. Theme resolver remains portable data; Components depends on that
value and Base sampler, never Base→Components/platform. Reuse registry ordering
and pinned source manifests; avoid a second independently sorted registry.

Required proof: independent raw JSON/name resolution for all38, byte-exact selected
colors plus explicit Default Light/Aurora Light/Default Dark counterexamples;
all4 builtin fallbacks, custom themes with identical preset-looking bytes but
no inferred identity, alpha0 hidden RGB, invalid indices/nulls with output byte
snapshots. Painter: actual software pixels for translucent/transparent/secondary
colors at quarter/interior/boundary times, clipping/inherited opacity and corner
geometry; fieldwise command equivalence of existing ex versus new painter using
`from_theme` under fixed clocks, independent states and callback traces. Freeze
legacy phase output separately before helper extraction. Invalid options/ease
and zero id leave outputs/state/commands unchanged; reduced and omission/reset
scheduling tested without native desktop windows. Verify strict C11, standalone/
selective/umbrella C/C++ and WASM compilation under a later bounded lease.

This step fixes source color selection and explicit custom paint only. It does
NOT deliver source full-parent/fraction/flex root sizing, own Styled border/layout
refinement, richer gallery compositions or any modern Skeleton binding options.
All five legacy constructors remain unchanged and their omissions remain visible.
Family completion, full renderer/platform parity and all78 closure remain open.
