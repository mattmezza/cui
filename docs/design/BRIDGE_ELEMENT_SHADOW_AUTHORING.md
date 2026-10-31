# Foreign-builder element shadow authoring proposal

Audit base: `37eecd1f` (2026-10-31). Proposal only: no bridge declaration,
implementation, facade, generated resource or coverage status changes. The
current Core named-box APIs remain C-only. This document requires API review
before production work, especially before extending the mapping to component
parts or detached surfaces. See `../ELEMENT_SHADOWS.md` and
`ELEMENT_SHADOW_STYLING.md` for the implemented Core seam and renderer limits.

## Smallest compatible first slice

Offer an owned, typed list of at most eight mixed DROP/INSET layers and explicit
corner radii on six bridge node kinds: ROW, COLUMN, LABEL, BUTTON, INPUT and
BADGE. These are two authoring containers plus four catalog families; they are
not six of 78 equivalent component contracts, nor all-family/part support.
Reject other node kinds immediately at the authoring setter. Expose identical
scope through Python, JavaScript/TypeScript, Go and Rust. Presets expand through
canonical C preset logic. An explicit CSS constructor converts blur radius to
sigma; ordinary layer constructors continue to accept sigma.

This first slice needs no new part registry, public Core paint-node query,
wrapper box or generic fallback from a semantic node to an arbitrary rectangle.
Add explicit post-submission hooks to the existing dispatch branches. Expanding
beyond this allowlist requires an owner-specific mapping and tests, reviewed
separately. A future component-part addressing API must not be smuggled into a
universal `node.id` fallback.

## Audited submission and identity

The opaque bridge handle is slot+generation. `bridge_node` validates both and
live membership (`src/bridge/cui_bridge.c:341`); node creation assigns a private
Core ID independently of the key/handle (`:1201`). Removal recursively frees
resources and marks nodes nonlive (`:1345`); reused slots receive a new
generation. Keep all existing app provenance and one-thread/serialized-app rules.

| Proposed kind | Current submission / target | Attachment point and scope |
| --- | --- | --- |
| ROW, COLUMN | `bridge_render_node` opens/closes `node->id` (`src/bridge/cui_bridge.c:4700`) | After its matching end, after all children. Includes a row/column app root, excluding the bridge's separate background wrapper ID1. No implicit background or new hit/semantic node. |
| LABEL | Explicit named box containing text (`src/bridge/cui_bridge.c:4436`) | After that box closes. Shadows follow its rectangular layout bounds, not glyph outlines; transparent boxes still have rectangular caster semantics. |
| BUTTON | Fallback branch calls `cui_button_ex(node->id)` (`src/bridge/cui_bridge.c:4664`); `cui_button_paint_state` opens root ID (`src/components/cui_components.c:609`) | After the complete Button helper returns. Guard specifically BUTTON, rather than assuming every fallback kind is a Button forever. Style only its root, excluding text, caret, icons and tooltip. |
| INPUT | Branch calls `cui_input_ex(node->id)` (`src/bridge/cui_bridge.c:4577`); `cui_input_common` paints through `cui_input_paint`, which opens ID (`src/components/cui_components.c:2579`) | After the complete Input helper returns. Root includes its input frame; selection/caret/text are separate content. Preserve its existing clip/input behavior. |
| BADGE | `cui_badge(node->id)` (`src/bridge/cui_bridge.c:4575`) opens/closes that ID (`src/components/cui_components.c:1914`) | After the Badge helper returns. Root only. |

Source line references identify the audited base; inspect the named functions
when rebasing. Styling success must come from Core current-frame resolution,
never previous-frame `cui_bounds` or semantic presence. Button's bool result is
activation, not paint admission. Input has an error result (which does not itself prove root absence). Preserve
existing error propagation and honor component/Core
failure paths; a missing expected root must report a frame error, not silently
claim successful rendering or patch the previous frame.

### Why a universal epilogue is insufficient

`bridge_render_node` returns immediately after dispatching non-row/column nodes.
A hook only after its row/column end misses those nodes. A hook at the bottom of
`bridge_render_node_other` misses the TextView allocation early return (`:4325`)
and cannot establish that each helper actually admitted its paint root. That
return also bypasses the existing disabled restoration; this audit does not fix
that unrelated behavior. Helper-local failures also return without materializing
a root, for example group scratch allocation (`bridge_render_group:3147`).

ITEM is intentionally not painted by its own dispatch branch; its owner renders
it. NOTIFICATION is a no-op there and the shared notification stack is submitted
later in `cui_bridge_step` (`:4768`). Styling either at ordinary dispatch end would
invent a target. A child authored under a composite is not proof that its own
bridge dispatch runs, or that an owner does not measure/omit/repeat that child.
First-slice tests must include nesting and protected/layout-only submissions;
retain Core mutation guards and never make them an excuse to ignore failures.
Use the existing `cui_frame_paint_enabled(ui)` predicate before applying an
owned override. It is false in host measurement/layout-only work but true in
painted inert ancestry. `cui_frame_sequence` is an input-observation predicate
and returns zero in inert ancestry, so it is the wrong paint guard. Skipping a
nonpainting pass retains authored values for the subsequent real paint pass;
an unexpected Core setter failure during an ordinary painted pass is a frame
error.

### Explicitly deferred mappings

| Family/path | Why the first slice rejects it |
| --- | --- |
| Dialog, AlertDialog, Sheet, Popover | Bridge initializes `surface_state.overlay.id` from `node->id | (1<<62)` (`:1331`). `cui_surface` paints its panel at the overlay ID, with separate backdrop, arrow and resize IDs. Root node ID is not a panel contract. |
| Tooltip, HoverCard | The content portal uses `node->id`, but exists only after trigger/placement/admission and balanced begin/end. This is a promising second slice with explicit presence results and popup-only semantics, not automatic trigger styling. |
| Select, Combobox, Menu, DropdownButton, DatePicker, Command | Triggers and conditionally live popups have separate owners and derived identities; audit each. |
| Checkbox, Radio, Switch, Slider, Stepper and other compound controls | A semantic/layout root does not imply the visible indicator, thumb, track, connector or repeated item is styled. Anonymous and repeated parts need owner-approved APIs. |
| Notification, ITEM, Settings/Questionnaire-owned fields, virtualized/repeated children | Deferred or owner-controlled rendering may bypass direct node dispatch. No inferred ownership or wildcard style application. |
| Root View, Focus Trap, Dock and remaining unlisted kinds | Structural behavior or helper submission differs; absent from first-slice support even if a named rectangle could be found today. |

Keep the bridge background wrapper, application root, popup panel, trigger and
backdrop independently scoped. No traversal that paints all descendants and no
new wrapper that changes layout, clipping, capacity, hit tests or semantics.

## Proposed additive ABI and owned state

Names below are review candidates, not declarations available today. Preserve
ABI_VERSION1, existing public layouts and all old entry points. Add a fixed,
pointer-free `cui_bridge_box_shadows_v1` input record and a size probe, following
the existing versioned notification precedent. The setter takes explicit
`struct_size`, checks exact size/schema, and copies before returning. Define
field order/types in the header; do not alias `cui_box_shadow`, C enums or Clay.

A V1 record contains `uint32_t` size/schema/count/reserved, four named float radii
(`top_left`, `top_right`, `bottom_left`, `bottom_right`, matching Core), and eight
fixed layer slots. Each slot contains four floats (offset_x/y, sigma, spread),
`uint32_t kind` and `uint32_t rgba` explicitly packed `0xRRGGBBAA`. No pointer,
`size_t`, bool, implicit CSS corner order or host-endian byte-array reinterpretation
crosses the ABI. Count controls the active prefix; reserved fields must be zero.
Publish/test the size and offsets in all FFIs. Exact-size V1 can coexist with a
future V2 without resizing old options or requiring old callers to know V2.

Candidate operations:

- `cui_bridge_set_box_shadows_v1`: replace the owned normalized list atomically.
- `cui_bridge_set_shadow_preset_v1`: explicit preset+radii, expanded by canonical
  preset code to the same normalized list. NONE is an explicit empty override.
- `cui_bridge_reset_box_shadows`: remove the override and restore normal component
  defaults on subsequent submissions.
- `cui_bridge_box_shadow_from_css_v1`: checked factory for one typed layer;
  output unchanged on rejection. Reuse Core conversion, not five divergent
  copies of the conversion/preset tables.

The exact function signatures, optional capability query and error precedence
need approval. Existing unsupported-kind setters return INVALID_ARGUMENT; use
that precedent, STALE_HANDLE for stale handles and INVALID_STATE while stepping.
Validate the entire record, allowed kind, finite/domain values, count<=8 and
radii for nonempty lists before modifying owned state. Keep the existing
empty-list radii validation policy, including NONE, rather than introducing a
clear-only mismatch. Reuse Core-supported domains (sigma[0,256],
offsets/spread[-4096,4096], CSS radius[0,512]); do not silently clamp NaN, truncate
nine layers, coerce invalid colors or reinterpret sigma as blur radius. Check
float conversion overflow in the facade before conversion where necessary, and
validate again in C. A rejected replacement preserves the previous list.

Store `has_shadow_override`, normalized count, radii and eight copied Core layers
in the private node record. This bounds memory per node and avoids a new heap
resource lifetime; document/check the actual per-node memory increase. No layer
array is retained by reference. Initialize to no override. Remove/reset stale
slot state, and rely on creation's whole-record initialization on slot reuse.
Unattached builder objects also copy their typed values; mounted objects update
C first and commit their local copy only on success. Removal/app destruction
invalidates subsequent mutations under existing handle rules.

There are three distinct states: absent override (normal component defaults),
explicit empty override (suppress both DROP and INSET), and explicit nonempty
replacement. Clearing and resetting cannot be synonyms because components can
have default shadows now or later. `[]` and preset NONE must remain empty on every
frame until replaced/reset. They must not merely skip the Core setter and allow
default shadows to reappear.

Core styles last only for the submitted frame. The bridge retains authored style
and reapplies it after the owner's complete submission every frame. Component
styles/defaults run first; the explicit override is last. Begin/end components
must finish their owner work before styling, if later approved. For example,
`cui_surface` installs its own shadows while the panel is open
(`src/components/cui_surface.c:188`); a pre-submission or trigger-level setter
would not provide panel override semantics.

Explicit radii are part of the style value; first-slice authoring must not promise
automatic matching of theme/state-dependent component radii. A preset supplies
layers, not target geometry. Any future automatic-radius feature needs a separate
owner contract and theme/state transition tests.

## Facade work and compatibility

| Facade | Concrete integration points and owned authoring shape |
| --- | --- |
| Python | ctypes `_API`, immutable typed layer/radii values, `Widget` retained fields and `App.mount` application in `bindings/python/cui_bridge/__init__.py`. Copy user sequences immediately and apply both pre-mount and mounted updates. |
| JS and TypeScript | Add N-API marshalling/exports in `bindings/node/addon.c`, owned validated immutable records in `Node` and mount replay in `index.cjs`, typed declarations in `index.d.cts`. JS and TS share runtime; TS declarations alone are not runtime coverage. |
| Go | Add dynamic/static bridge function plumbing in `bindings/go/cui/native.go`, typed value structs/enums and a copied owned slice/fixed array on `Node`, plus mount replay. Preserve fluent error behavior, app checks and pointer lifetime through the copying call. |
| Rust | Add V1 `repr(C)` wire structs, separate safe value types and validated `App` mutation methods in `bindings/rust/src/lib.rs`; retain existing Node app identity and Result errors. Copy borrowed slice data through a local fixed record; no borrowed slice retained by C. |

Typed examples should use a DROP layer, an INSET layer, a preset, empty-list clear
and reset on an allowed node kind in each language. Keep sigma and CSS factory
names distinct. Do not expose raw private paint IDs, numeric node-kind constants
or Core pointers as user authoring parameters. Missing newer symbols in an older
shared library must produce an explicit upgrade/matching-artifact error, never
silently drop styles. If adding optional symbol lookup, review that as a separate
loader decision; ABI1 equality by itself does not prove symbol availability.

## Focused acceptance plan after review

Reuse the existing portable private bridge-render fixture pattern in
`tests/unit/test_bridge_avatar_style.c`; it captures the real bridge draw list
without exporting a public renderer/test API or opening a window. Add a targeted
bridge shadow fixture linked only against needed portable libraries. Test:

1. Each of the six allowed kinds actually emits the requested root layers and
   unchanged bounds, hit activation, focus and semantics. Compare no-opt-in legacy
   commands/pixels and explicit reset to normal default behavior.
2. All eight mixed layers, copy-after-call, same-frame replace, next-frame retained
   replay, empty/NONE clear, reset, invalid ninth/NaN/enum/radii/size/schema/color
   atomic rejection. Canonical preset expansion and CSS constructor boundaries
   must match C exactly; no duplicate facade-specific preset tables.
3. Nested row/column and two independently styled nodes, subtree omission/removal,
   stale handle/slot reuse, failed mount rollback and app destruction. A child
   must not inherit its parent's layer list or overwrite its decoration.
4. Unsupported popup/ITEM/Notification/composite setters reject before render;
   closed/missing trigger states do not start styling some other target. Protect
   measurement/layout-only and mutation-during-step boundaries; reject missing
   expected roots after component capacity/error paths and recover cleanly.
5. One focused contract suite per facade: pre-mount ownership/replay where that
   facade supports unattached builders, immediate-construction ownership for
   Rust, mounted update,
   replace/clear/reset, invalid-input atomicity and propagated native errors.
   Check JS runtime and TS compile-time surface, Go both loader paths and Rust
   FFI layout, while avoiding package suites that rebuild unrelated targets.

Existing Core/renderer shadow tests remain authoritative for raster algorithms.
Only add necessary bridge-level pixels/order checks rather than duplicating that
suite. Browser proof, if a builder host is actually used, must execute the matching
artifact headlessly; the C gallery cannot prove Python/Node/Go/Rust authoring.
Native tests use `scripts/test_local.py` with private Xvfb. Unavailable GPU/live AT
checks remain deferred. Coordinate one nice19, -j1 build slot and reuse the
existing focused build directory; this audit ran no builds or tests.

On implementation, update bridge docs, `binding-coverage.json`, examples and source
metadata for exactly the tested allowlist; regenerate SDK/amalgamation/skill
resources and run required focused freshness/offline checks. Keep all78 catalog
entries and the beta roadmap shadow item pending until its stated broader work
is complete. This proposal changes no actual coverage, so the existing unchecked
roadmap and C-only consumer limitation remain correct.

## Approved V1 checkpoint before production

The six-kind scope and owned override are approved. Freeze these additive names
and signatures (all return `cui_bridge_status` except the two uint32 queries):

```c
uint32_t cui_bridge_box_shadows_v1_size(void);
uint32_t cui_bridge_box_shadows_supported_v1(uint32_t kind);
cui_bridge_status cui_bridge_set_box_shadows_v1(cui_bridge_app *, cui_bridge_node,
    uint32_t struct_size, const cui_bridge_box_shadows_v1 *);
cui_bridge_status cui_bridge_reset_box_shadows(cui_bridge_app *, cui_bridge_node);
cui_bridge_status cui_bridge_box_shadow_from_css_v1(float x, float y, float blur_radius,
    float spread, uint32_t rgba, uint32_t kind, uint32_t layer_size,
    cui_bridge_box_shadow_v1 *out);
cui_bridge_status cui_bridge_shadow_preset_v1(uint32_t preset,
    float top_left, float top_right, float bottom_left, float bottom_right,
    uint32_t struct_size, cui_bridge_box_shadows_v1 *out);
```

`cui_bridge_box_shadow_v1` is 24 bytes: float offset_x/y/sigma/spread at offsets
0/4/8/12, uint32 kind/rgba at16/20. `cui_bridge_box_shadows_v1` is224 bytes:
uint32 struct_size/schema_version/count/reserved at0/4/8/12, four float radii in
Core named order at16/20/24/28, and eight layer records at32 with stride24.
Schema is1; reserved is0. Size/offset assertions and each facade's layout tests
are required. Packed color is numerically0xRRGGBBAA, independent of byte order.
Factories reuse Core preset/CSS functions and commit outputs only on success.
`preset_v1` constructs a complete owned style, so every facade can use precisely
the same value setter for explicit, preset and empty lists.

Private node storage is one inline224-byte V1 value; schema0 means no override,
schema1/count0 means explicit empty, and schema1/count1..8 means replacement.
No new allocation or retained pointer is introduced. All224 bytes are copied on
successful set. Runtime application converts at most eight wire layers to a
stack-local Core list, which Core itself copies. Existing whole-node creation
initialization clears inherited slot state. Verify/document the actual node-size
delta on the supported build ABI rather than assuming host struct packing.

App setter/reset validation order is existing `bridge_valid_node` first
(app lifecycle then live slot/generation), supported node kind next
(INVALID_ARGUMENT), then exact record size/schema/reserved and complete active
values. Invalid records preserve the old override. No new callback or mutation
phase policy: follow existing setters' step/run-dispatch rules. Capability query
returns exactly0 or1 for the six approved kinds. Pure factories have no app or
handle; invalid size/output/value returns INVALID_ARGUMENT with output unchanged.
NONE produces an explicit empty record with the same empty-list radii policy as
Core. Reset returns the node to schema0, restoring normal defaults next frame.

Facade numeric validation must reject values that overflow conversion to finite
float32; C validates again. Python/Go optional loading must preserve old default
library loads, with explicit unsupported-feature errors only when new authoring
is requested. Static/dynamic facade layout checks must agree on224/24 and fixed
offsets. Reapply only when `cui_frame_paint_enabled` is true, after complete owner
submission; an unexpected missing named target in an ordinary painted pass
becomes the existing bridge frame CORE_ERROR. Inert ancestry still paints.

This signed checkpoint changes no production or coverage status. The pending
beta shadow item remains unchecked; implementation and focused all-facade proof
must follow under the coordinated build slot.
