# Configured Badge authoring: Node/TypeScript checkpoint

Design only, against root `956a853c`; no bridge/Node production change or executed
validation. Existing `badge(key,text)` remains the standalone kind11 text pill.
All 78 entries remain; configured foreign Badge and complete Icon authoring are
pending in every language until their individual gates pass.

## Actual authority and gaps

There is no `include/cui/cui_badge.h` at this revision. Badge authority is
`include/cui/cui_components.h:470–515`, Icon is `include/cui/cui_icon.h`.
`src/components/cui_components.c:cui_badge_compose` owns corner placement,
content-before-indicator order and configured artwork fallback.
`src/components/cui_icon.c:cui_icon` owns Icon option validation/layout/paint;
`src/base/cui_icon.c` owns source copies, immutable masks and bounded versions.

Current bridge `src/bridge/cui_bridge.c` renders BADGE with `cui_badge()` and
ICON with named artwork plus positive visual width/height only. Its parent
admission does not make BADGE an arbitrary-content container. Node mirrors those
limitations (`badge()` kind11, `icon()` kind50). Adding JS methods alone cannot
reach `cui_badge_ex` or `cui_badge_with_icon`.

| C surface | Required foreign mapping; no reduced substitute |
| --- | --- |
| Badge defaults/options | COUNT/DOT/ICON, full `size_t` count/maximum, four control sizes, design size, optional explicit RGBA including alpha0, copied name and named-icon string |
| `cui_badge_ex` | Arbitrary ordered child subtree; count0 omits only indicator; count>maximum displays maximum+; maximum0 retained |
| `cui_badge_with_icon` | Owned Icon resource + complete Icon options; `submitted` separate from `icon_error`; structural rejection invokes no child callback |
| `cui_badge_indicator_id` | Existing indicator target, not wrapper/artwork; absent on count0; pure ID is no presence guarantee |
| Icon source/state | name/path/raw SVG/empty, source replacement/reload, limits/defaults, explicit owner lifetime, counters/errors, collect after render consumption |
| Icon geometry | All six sizes, custom/inherited text size, explicit width/height presence including0, design size/rem, affine six floats and identity/rotation helpers |
| Icon paint | Explicit/inherited tint presence, opacity presence including0, debug placeholder, complete applicable box style and numeric constraints |
| Icon semantics | Standalone named image/decorative contract; configured Badge forcibly decorative, sole Badge STATUS remains |
| Icon utilities | Catalog count/name/path/default, query/embedded asset copied bytes, headless prepare, copied bitmap/paint bounds; named/spinner convenience routes tracked separately |
| Custom C loader | Cannot pass JS function/user pointer through a record or invoke it during Core composition; explicit policy decision below |

## Recommended narrow bridge prerequisite (proposal, not exported API)

Use one reusable app-owned Icon resource abstraction, then configured Badge node
composition referencing it. Do not invent a Badge-only record containing merely
SVG/size/color. Do not put `cui_icon_state*`, `cui_box*`, callbacks or pointer-sized
fields into foreign payloads. Root must approve the C bridge schema/signatures
before implementation; the sketch below does not reserve symbols or enum values.

```
const art = app.icon_resource({source:{kind:'name',value:'check'}, limits:{...}});
const badge = configured_badge('unread', {
  variant:'icon', count:123n, maximum:99n, size:'medium', designSize:'inherit',
  color:[220,30,40,255], name:'Unread',
  artwork:{resource:art, options:icon_options({size:'custom',customSize:20,
    transform:icon_rotation(Math.PI/4), color:[240,120,30,255],
    style:{...}, constraints:{...}})}
}).children(button('open').label('Inbox').on_click(openInbox));
// After successful or failed step: immutable current-frame result, never replay.
const result = badge.latest_result(); // frame, submitted, iconError, frameStatus
```

Proposed bridge operations: complete-group/version/layout support query;
app-local generation-tagged resource create/destroy/source/limits/reload/get-info;
full copied Icon options configure/get; dedicated configured Badge create/configure/
get-result; copied catalog/asset/bitmap utilities. An atomic full replacement setter
must retain the previous resource/options/strings on invalid input or allocation
failure. Exact native descriptor sizes, reserved words, active-field rules and
optional-symbol list require a separate reviewed C checkpoint, not guesswork here.

A new explicit configured Badge kind is preferable to silently changing kind11
child acceptance/rendering. Only it is a content container. C bridge recursion is
the `cui_button_content_fn`: it renders each child once, never invokes JS inside
Core, and ordinary child actions dispatch synchronously after stepping. Mount
preflight rejects absent/partial capability before creating any nodes.

## Complete options, ownership and numeric rules

Represent every Icon option field with its actual presence flag; do not equate
zero with absent. Copy style and constraints into stable bridge-owned storage and
reconstruct private native pointers only during synchronous submission. Include
all `cui_box` scalar/color/edge/corner/flag fields in the shared style mapping;
document which Icon deliberately overrides: width/height/fixed/percent/max and
paint padding are resolved by `cui_icon`, not reapplied by JS. Constraints include
explicit maximum0; minima win, then padding/border floor. Do not implement layout
or SVG parsing/rasterization in JS. Preserve native normalization versus rejection
for opacity/style exactly; do not reject inactive poison differently without an
explicit binding-level contract and tests.

Use BigInt for count/maximum and resource/frame generations. Query native size_t
width; accept the entire target domain, reject overflow before conversion. All
floats require checked double→float conversion under the corresponding C active
field rules; enum/boolean/presence/reserved validation is explicit. Strings are
owned strict UTF8 with NUL policy and published byte bounds; never silently truncate.
Native C records are native-endian; any internal LE Buffers require fieldwise
conversion, including size_t/u64, as with the localized TimeField facade.

Each app owns resources; handles reject cross-app/stale use. Node configuration
retains referenced resources. Destruction while referenced rejects or marks a
retired handle whose storage survives final render; choose one precise policy in
C review (recommend reject while referenced). Replacement/omission/removal cannot
collect old masks before the synchronous renderer finishes its borrowed list.
Close/error/destroy must release exactly once; poststep callbacks may mutate next
frame state, never invalidate the just-rendering list. No implicit file/network I/O.

For host-provided sources, prefer copied path→SVG asset maps with revision/reload,
served by a C-resident loader. This covers deterministic application assets without
foreign callbacks during Core. It is **not** equivalent to arbitrary deferred
`cui_icon_loader_fn`; do not advertise that callback API as supported. If full
callback equivalence is required, review a pre-frame host preparation service
separately. Eager JS loader calls at setter time change callback/error timing and
are not a transparent implementation of C's next-prepare/reload semantics.

## Results, alternatives and acceptance slices

Structural invalidity submits nothing and renders no child. Artwork invalidity,
NOT_FOUND/PARSE/LIMIT/ALLOCATION occur after content and shell and preserve them;
report `submitted=true` plus exact Icon error, independent from Core/frame failure.
Do not translate every Icon error into a failed whole frame or swallow it.
Badge overrides decorative and missing inherited size/color only; explicit artwork
options win. No extra image semantic, focus stop, click target or JS reducer.

A JS-only wrapper of ordinary `icon()` loses styles/state/errors and cannot preserve
Badge zero-flow placement. A Badge-specific SVG wire repeats an incomplete Icon
API. Direct JS rendering callbacks inside Core violate the existing host model.
Recommended slices: (1) reviewed reusable owned Icon bridge/options contract;
(2) its resource/old-consumer tests; (3) configured Badge C recursion/results;
(4) Node/TS complete authoring, examples and root acceptance. Catalog/spinner/custom
loader omissions must stay explicit until implemented; none closes source parity.

Meaningful gates: frozen scalar Badge/Icon ABI/pixels/callback counts; complete
old/partial optional group refusal; every size/variant/count0/max0/full size_t;
all option presence/zero/constraints/style/transform/opacity combinations against
independent ordinary C Icon trees; exactly-one STATUS and child activation;
first-frame/resize/nested/clip/omission/reappearance; malformed SVG/missing assets/
resource cap and allocation injection with shell retained; copied-source replacement
and old-list masks alive; cross-app/stale/refcount/destruction/reentry/exception;
strict TS typed configs/results and actual English UI with count/dot/custom-SVG
badges around buttons. All native execution uses private Xvfb; browser headless.
No public implementation, generation, build or tests ran for this checkpoint.
