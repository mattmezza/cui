# Frozen Badge family completion audit

Source-only checkpoint against CUI `b4cd6a8f`; no build, runtime, browser,
generation or status change was performed for this audit. Existing tests below
are inspected evidence, not a fresh execution receipt. The family remains partial.

## Authority

The baseline is GPUI Kit v0.7.1, commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, as recorded in
`docs/upstream-baseline.md`. Primary files were fetched individually from
`https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/`;
no repository clone or dependency installation was used.

| Path below pinned repository | SHA256 |
| --- | --- |
| `crates/component/src/badge.rs` | `f4debc66e913a6fb4450beeebc3acd19d2b882b891a6e9fbe65ca4421497a691` |
| `website/component/badge.md` | `87c7eca64a16f293ab94bcc8377b25a281960ae58f1709f92ef7e8b05e750c14` |
| `crates/component/src/icon.rs` | `f8f9355f51bed7e16634c9052454c2649ba1b800dd7932b1094997a967eebf02` |
| `crates/component/src/lib.rs` | `81b2e61f0dcd6b5a083cfb1b4c603a26aff0a30d2ddcb583b9c130f975acfabd` |
| `crates/component/src/sizing.rs` | `379fa0e1ffd05c5a1711647f183ba3e5f1fb297887ea0929f229d22d32142251` |
| `crates/component/src/component_traits.rs` | `f95ca555f338c8a61a74dca5a9ad1da82499b6e7a2c198c83bb1be777339bdc4` |

The website digest exactly matches the Badge row in `docs/gpui-parity.json`.
Its three feature labels are an index, not the family definition. That row has
partial platform evidence and no last-verified commit. Its sample points to
`gallery/main.c`, which demonstrates the legacy pill; the richer demo is in
`web/gallery.c`. Neither reference establishes complete family coverage.

## Source behavior and coverage matrix

C authority: `include/cui/cui_components.h`, `src/components/cui_components.c`,
`docs/BADGE.md`. Existing direct tests: `test_badge_variants` and design-size
coverage in `tests/unit/test_components.c`. P means implemented with specific
inspected assertions; U means no sufficient independent proof located here.

| Frozen behavior or state | Current C route and inspected evidence | Remaining gate |
| --- | --- | --- |
| Default Number, count 0, maximum 99, Medium | `cui_badge_options_default`; count-zero keeps child wrapper and removes marker, P | Default full geometry and empty-child oracle |
| Positive count, maximum equality, decimal overflow `maximum+` | `size_t` formatting; count100→99+ P | 1/9/10/99/100, equality, custom maximum, maximum0, SIZE_MAX without truncation |
| Dot always visible independently of count | DOT P, 6px square | Count/max changes must not affect dot; zero/alpha/clipping witnesses |
| Icon always visible independently of count | ICON/check image P | Custom SVG and source icon refinement below |
| Builder variant ordering | Source count() does not reset dot/icon, despite its comment; C uses explicit enum | Document declarative equivalent; test transitions without stale marker |
| Arbitrary child content; marker appended after children | Callback once, fixed80×40 child and text, P | Empty, multiple children, avatars, nested badges, responsive child, independent child input/state |
| Zero-flow absolute corner placement | First-frame outer and child80×40 retained for all 4 sizes ×3 variants, P | Real-font narrow/wide resize, nested clip, overlap and software pixels |
| Count top/right offsets | Large +2/−N, Medium −3/−3N, Small/XSmall −4/−4N; P | Multi-digit/custom maximum widths and transform/clip cases |
| Count pill geometry | Text10, line10, padding2, minimum14; measured width, P | Actual font pixels/baseline and non-default rem policy |
| Source sizes | Medium default; Large24, Medium/custom Size(_)16, Small/XSmall10 for icon shell; dot6/count10 unchanged | C explicit design-size is an extension, not proof of source arbitrary Size(_) mapping; publish distinction |
| Icon bottom-right shell | 10/16/24, rounded, border1 theme background; image command P | Border pixels, transparent/custom background, clipped shell and artwork |
| Supplied icon remains an Icon object | Source retains path/data, own size/style/color/rotation; icon.rs uses inherited size/color only as fallback | C reconstructs named icon at extent−2 and white. Add compatible icon-content/refinement route or explicitly keep this gap; current configurable name is insufficient |
| Default color and custom color | Source theme.red; C theme.danger/custom RGBA, opaque custom RECT P | Prove semantic token mapping in light/dark themes; alpha0/partial alpha and exact colors |
| Rounded marker, white foreground, stacking | C radius1024, white text/icon, absolute overlay | Pixel/culling/shadow ownership witnesses, not command presence alone |
| Outer style refinement | Source stores/refines StyleRefinement but badge.rs has no Styled implementation; Icon does | Do not claim arbitrary Badge style builders merely from imports/private field. Distinguish reachable parent composition from any proven direct authoring; verify actual examples compile before adding API scope |
| Different positions/complex nesting examples | Parent layout and caller composition, not a Badge position setter in pinned implementation | Copyable recipes and real geometry tests; no invented source position API |
| Passive semantics | Source Badge has no own focus, activation or event handler. C adds STATUS/name, parent wrapper, no focus/tab; P | Dynamic names, empty/custom names, absent marker references, screen-reader/AccessKit evidence still separate |
| Child interactivity | Gallery button child increments count; browser test checks 7→8 | Pointer/touch/key child activation exactly once with overlay marker; marker must not capture or steal focus |
| Lifetime and state | Source consumes builder/children each render; C caller owns count and child state, synchronous callback; no Badge retained state | Copied frame text/name and SVG lifetime/resource recovery tests, omission/reappearance and ID reuse |
| Invalid input/failure | Invalid variant prevents callback/submission P; invalid design-size P | All invalid records, malformed strings, derived-ID/collision boundary, resource exhaustion; no partial callback or leaked frame resources |

The frozen Badge itself does not require drag, selection, keyboard navigation,
focus rings, disabled paint, close handling, animation, or value callbacks. These
are justified noninteractive exceptions for the marker only. Arbitrary interactive
children retain their own full input/lifetime contracts. Count updates must still
be visible and accessible; the exception does not excuse stale content or blocking
child input. Native GPU and browser visual equivalence are not inferred from C
command assertions.

## Five binding coverage

All facades reach `src/bridge/cui_bridge.c`'s CUI_BRIDGE_BADGE branch, which calls
legacy `cui_badge(text)`, not `cui_badge_ex`. This is the standalone text pill,
with primary/on-primary styling; it is not the frozen count-overlay family.

| Facade | Actual authoring path | Coverage and missing surface |
| --- | --- | --- |
| Python | `bindings/python/cui_bridge/__init__.py`: badge(key,text)→Widget("badge") | Legacy label; component smoke test. No typed count/max/variant/icon/size/color or composed Badge child API |
| JavaScript | `bindings/node/index.cjs`: badge(key,text)→Node(11,text) | Same legacy bridge route; generic node styling does not add overlay semantics |
| TypeScript | `bindings/typescript/index.d.ts`: badge→Label with shadow traits; Node declarations re-export it | Typed legacy surface only; no frozen Badge options or owned icon content |
| Go | `bindings/go/cui/native.go`: Badge(text)→Node kind11 | Legacy smoke in native_test; no extended Badge record or nested marker ownership |
| Rust | `bindings/rust/src/lib.rs`: Kind::Badge11, App root/child/build APIs | Generic typed node construction reaches legacy render path; no Badge-specific options |

Existing root/part shadow facilities must be checked against their actual
allowlists. A shadow trait is not evidence that the extended marker or arbitrary
nested child is exposed. Bridge node lifetime/rollback and copied-string ownership
must be preserved when adding the family; no arbitrary child support should be
advertised unless the actual Badge painter submits those children.

## Gallery and browser evidence boundary

`web/gallery.c` case8 demonstrates count7, overflow125, dot, check icon, and zero
marker with clickable children; control sizes also exercise CUI design metrics.
`tests/web/test_browser.py` checks semantic names, 7→8 and four design-size marker
extents. These are useful partial tests, not pixel/refinement/clip/asset/lifetime
or all-five-runtime proof. Avatar/nested/color/position examples from the source
document are not equivalent to the current five-button demo.

## Next cohesive implementation and verification slice

1. Preserve legacy `cui_badge` and all existing foreign calls. Settle an additive
   extended C icon-content/options route supporting a supplied configured icon
   without resizing published records. Retain named-SVG convenience. Record
   source-red versus danger mapping, arbitrary source Size(_) versus CUI design
   scale, and only genuinely reachable outer refinements. No generic style graph.
2. Add owned bounded bridge Badge options and explicit child-composition painting,
   with default absence preserving legacy pill. Expose the same typed family in
   Python/JS/TS/Go/Rust, copied values, atomic invalid replacement, clear/reset and
   node reuse/destruction. Specify marker IDs separately from wrapper IDs; no
   silent compound-part or portal ownership claim.
3. Add table-driven source numerical oracles for the matrix, real-font first-frame
   and resize tests, software pixels for color/alpha/border/clip/stacking, and
   passive-marker child input tests. Exercise maximum edges, variant transitions,
   omissions, nested content, asset lifetime and resource failures. Keep an
   independent legacy pill/foreign default control to detect compatibility drift.
4. Run all five actual runtime suites and TypeScript checks; migrate/add narrow
   gallery examples for nested/avatar/custom icon/color cases and headless browser
   coverage. Record exact platform/renderer exceptions instead of replacing them
   with a generic parity claim.
5. Update authoritative headers, BADGE guide, component usage/metadata, examples,
   SDK/amalgamation and consumer skill through canonical generators. Only after
   fresh configured native/browser/binding evidence covers the agreed matrix may
   the integrator consider family verification. This checkpoint changes no status.

The first review should settle the small icon/options and bridge ownership contract;
there is no justification for an input dispatcher or new global state. The C
geometry foundation is substantial, but foreign authoring and configured-icon
fidelity prevent a complete-family claim today.

## Proposed additive authoring contract (source-only, not declarations)

This checkpoint proposes APIs; none is implemented or measured. Preserve both
`cui_badge` and `cui_badge_ex`, including their existing named-icon painter.
Do not enlarge `cui_badge_options`. Existing Icon already supports owned SVG
state, path/name loaders, affine rotation, explicit/inherited size/color,
opacity, box decoration and numeric constraints (`include/cui/cui_icon.h`).
Reuse that implementation instead of adding Badge-specific rasterization.

```c
/* Proposed in cui_components.h; names subject to review. */
typedef struct {
    cui_icon_state *state;
    cui_icon_options options;
} cui_badge_icon_content;
typedef struct {
    bool submitted;
    cui_icon_error icon_error;
} cui_badge_result;
cui_badge_result cui_badge_with_icon(
    cui_context *, cui_id, const cui_theme *, cui_badge_options,
    const cui_badge_icon_content *, cui_button_content_fn, void *);
```

The configured route requires ICON variant and nonnull initialized state and
content descriptor. Reject incompatible variants before invoking child content.
Shell geometry still follows Badge size; artwork is a separate ordinary Icon
child, with its own configured dimensions and transform. No extent-minus-two
rewrite. Supply source fallback inherited text size 14/10/8 for Large/Medium/
Small-or-XSmall, inherited white, while explicit icon size/color remain
unchanged. `Size::Size(_)` on source Badge maps to Medium shell/fallback; explicit
CUI design scaling remains separately documented. An inherited-size descriptor
can use these Badge defaults; authored Icon overrides take precedence. Copy
options locally before applying fallbacks; never mutate caller records. Force
artwork decorative so Badge STATUS/name remains the sole marker semantic;
child content keeps its own semantics. Rotation affects artwork paint, not
shell dimensions, anchoring or input.

The descriptor/options/style/constraints and callback userdata are borrowed
synchronously. State masks must outlive rendering of every returned draw list;
collect/destroy follows the existing Icon contract. SVG replacement preserves
older masks. Validate descriptor fields before submission; missing/parse/limit
errors from icon preparation are reported in `icon_error`, with the valid shell
and ordinary child content retained (not falsely reported as atomic UI rollback).
`submitted` distinguishes invalid call from admitted Badge with failed artwork.
The implementation must preflight preparation before invoking content so error
ordering is deterministic. Exact native/WASM sizes and offsets require a later
compiler lease; no constants are published by this checkpoint.

### Bounded owned bridge proposal

Add a separate optional Badge configuration to the existing legacy Badge node.
Absent configuration keeps its current legacy text painter and child policy.
Configured Badge explicitly owns an ordered list of ordinary child nodes;
submit each exactly once inside the Badge content callback. This admits nested
badges, avatar nodes and custom child rows/buttons without a synthetic text-only
wrapper. Apply the existing graph's parent, mount rollback, destruction and
cycle checks; no second ownership graph or portal reparenting. A marker is an
internal part, never an independently destroyable child handle.

Proposed pointer-free bridge record and explicit copied source setter:

```c
/* Proposed bridge ABI V1; source tag NONE/NAME/SVG. */
typedef struct {
    uint32_t version, variant, size, flags;
    uint64_t count, maximum;
    uint32_t rgba, reserved;
    float icon_width, icon_height;
    float icon_rgba_r, icon_rgba_g, icon_rgba_b, icon_rgba_a;
    float transform[6];
    uint32_t icon_flags, reserved_icon;
} cui_bridge_badge_options_v1;
/* Proposed signatures use existing app/node/error types. */
cui_bridge_status cui_bridge_badge_set_options(
    cui_bridge_app *, cui_bridge_node, const cui_bridge_badge_options_v1 *);
cui_bridge_status cui_bridge_badge_set_icon_source(
    cui_bridge_app *, cui_bridge_node, uint32_t source_kind,
    const char *bytes, size_t length);
cui_bridge_status cui_bridge_badge_reset(cui_bridge_app *, cui_bridge_node);
```

This proposed record is 96 bytes by field arithmetic, NOT an ABI measurement.
Flags express custom background, artwork dimensions/color/transform; unused
fields/reserved bytes must be zero. Colors are straight RGBA; background uses
0xRRGGBBAA. Float32 values must be finite and accepted by Icon's existing domain;
facades reject overflowing conversions. Counts uint64 must fit native size_t
before commit, including WASM32. A separate name setter should reuse the existing
copied node text/name mechanism, not overload SVG bytes or infer accessibility
from paths. Version/flags/numeric/source validation and allocations finish before
replacing old values; failures preserve old configuration and child ownership.

Proposal: lazily allocate one per-configured-node payload, capped at 256 bytes
excluding the existing Icon state/cache and copied strings; measure actual
payload before approval. Cap source bytes at 64KiB, name/path at 1KiB and
accessible name at 1KiB; additionally apply existing Icon pixel/cache/version
limits, not an unlimited per-node raster cache. Allocation counts/bytes and
retained old draw-list versions must be disclosed separately. Zero configured
Badge nodes incur no payload allocation. Clearing source means empty artwork
while keeping the ICON shell; resetting configuration restores the legacy pill.
Removing a child does not reset options. Node destruction frees sources/state
only after outstanding rendering borrows end under the existing bridge frame
lifetime. Replacement/destruction and slot reuse need actual tests.

### Five-language surface and required review decisions

All five facades should expose a typed `BadgeOptions` and `BadgeIcon` value,
count/dot/icon constructors, optional content children, and explicit reset.
Python dataclasses, JS frozen/plain validated records, TS discriminated unions,
Go value structs and Rust enums/builders map to the same owned bridge contract.
Unmounted builders retain a request locally; mount validates against their owning
app/library. Never normalize through another global library. Mounted setters
commit facade state only after C success; absent new symbols preserve old loads
and return feature-unavailable on new authoring. No facade duplicates Icon math.
Source byte ownership is copied; arbitrary native loaders/callback pointers are
not exposed as cross-language records.

Before production settle: whether complete Icon decoration/constraints belong
in this first bridge record or an additive owned Icon descriptor shared with
standalone Icon (preferred for full family fidelity); exact Icon inherited versus
authored-size flags; source-red token mapping; and reachable outer Badge style.
The small record above covers required rotation/color/dimensions but must NOT be
claimed to represent all Icon refinements until those remaining fields have a
reviewed shared route. Source Badge has no Styled implementation; its private
style field alone does not justify inventing public Badge refinement methods.

### Meaningful acceptance tests

| Gate | Independent witness |
| --- | --- |
| Configured artwork | Same asymmetric SVG through standalone Icon and Badge: rotation, custom color/alpha, width/height, inherited fallback; compare software pixels and shell bounds |
| All variants | Count/max boundaries, zero hiding, dot and icon transitions, every source size; existing legacy painter frozen control |
| Composition | Empty/multiple children, avatar, nested Badge, interactive child; callback once, wrapper intrinsic first-frame/resize, marker zero-flow and clipped pixels |
| Semantics | One passive marker STATUS, explicit/default name, no icon duplicate/focus; child keyboard/touch activation retained |
| Ownership | Source replacement with old draw list alive, failed parse/allocation/oversize replacement, reset/clear, node removal/reuse and bounded cache collection |
| Five runtimes | Actual configured count/dot/icon/custom SVG and nested child rendering in Python/JS/Go/Rust, TS typechecks; old-library load and failed mount rollback |
| ABI and resources | Native/WASM sizeof/offsetof, exact source caps, uint64→size_t and float32 overflow, invalid flags/reserved fields, zero-idle allocation and payload size |

No production or generation is authorized by this proposed-record checkpoint.
Full family verification still requires the preceding matrix, not just these
new route tests or a legacy text-pill facade.


## Integrator decision: first C slice

Approve additive configured-icon C implementation and prepared meaningful tests,
preserving `cui_badge_options`, `cui_badge` and `cui_badge_ex`. Reuse full existing
Icon options/state, including decoration and constraints; do not duplicate SVG,
raster or dimension-resolution logic. Force only decorative semantics and
unprovided inherited fallbacks, preserving explicit authored settings.

Validate the new call's structural requirements before composing content. The
proposal's prepare-before-content ordering is not required if it would duplicate
Icon's layout logic: a documented content-before-artwork order consistent with
existing Badge composition is acceptable. Valid shell/content may remain when
artwork reports failure; no atomic UI rollback is promised. Borrowed options and
userdata last through synchronous submission; prepared masks follow Icon's draw-
list lifetime. Preserve source/default color mapping explicitly.

The illustrative foreign record remains proposed, not approved or implemented:
it must use actual bridge status types and cover complete shared Icon styling
before full-fidelity authoring is advertised. C source work does not close five-
language, ABI, pixel, lifecycle or gallery acceptance. Worker builds and generation
remain held while the localized TimeField bridge owns the serial validation slot.

### Unvalidated additive C source checkpoint

Prepared `cui_badge_with_icon`, descriptor/result declarations and a standalone
`tests/unit/test_badge_configured_icon.c`. Both old public routes use the same
unchanged Badge painter branch; only the new configured path invokes ordinary
`cui_icon` with copied options/decorative and missing inherited fallbacks.
Content-before-artwork validation is explicit. No raster/layout math duplicated.
Prepared assertions cover first-frame shell/content/artwork dimensions with
constraints/style/rotation/color, sole STATUS, caller descriptor immutability,
structural callback isolation and later artwork-invalid error ordering.

UNEXECUTED: compile/ABI, prepared test, software pixels versus standalone Icon,
SVG replacement/draw-list lifetime, invalid-resource cases and frozen old-route
regressions. Test registration, canonical generations, metadata and all five
binding work remain pending; no runtime, freshness or full-family claim.
