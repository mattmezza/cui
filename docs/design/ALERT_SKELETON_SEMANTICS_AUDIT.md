# Alert announcements and Skeleton loading semantics: source audit

Status: design checkpoint, no production changes. Reference GPUI Kit
`87d10ae5e1299d1be18670a37570c5402f952e34`, verified against the reference checkout;
CUI audit baseline `9d4914830f29bcdd8b12517f21841fbdae17f631`. The two retained
`not_started` feature rows describe different situations and must not be treated
as equivalent missing component implementations. Preserve all 78 manifest entries.

## Frozen source requirements

[Alert source](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/alert.rs)
implements `RenderOnce` at line 173. An invisible Alert returns `Empty`; a visible
Alert's root explicitly calls `.role(Role::Alert)` at line 192. It composes icon,
optional non-banner title, message and optional pointer close handler. This is
positive evidence for an alert-role requirement. It does not establish that a
particular OS or screen reader speaks the content, announces the initial mount,
repeats identical text, or interrupts speech. The component source has no explicit
announcement queue, delivery acknowledgement, utterance ID, or repeat policy.
Its role call alone cannot prove the behavior of GPUI's downstream accessibility
implementation. This audit makes no such upstream AT claim.

[Skeleton source](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/skeleton.rs)
contains a style refinement and `secondary` flag. `render` creates a `div`, applies
size/background/style and a repeating two-second opacity animation. There is no
loading boolean, role, accessible name, busy flag, content relationship, focus
behavior or completion announcement in this component. Neither this source nor
the associated frozen `website/component/skeleton.md` provides an explicit
loading-semantics API. Decorative placeholders are therefore the demonstrated
component scope. A meaningful loading region is an application composition,
not an inferred missing upstream Skeleton feature.

## Existing CUI evidence and limits

| Layer | Alert | Skeleton/loading |
| --- | --- | --- |
| Component | `src/components/cui_content.c:26` emits `CUI_ROLE_ALERT`, no tab stop; invisible alerts emit nothing. Name precedence is explicit accessible name, then message, then title. | Legacy `cui_skeleton` and extended `cui_skeleton_ex` paint boxes without semantic nodes. Extended API comment explicitly promises no focus stop or busy status. |
| Core snapshot | `cui_semantics` exposes role, name, bounds and details. It is state, not a speech request or delivery receipt. | Existing `cui_interact` plus `cui_set_semantic_details` allow the application to expose a meaningful named region/status and its `busy` detail. A paint-only Skeleton cannot identify the actual loading operation. |
| Native adapter | AccessKit mapping sets ALERT/ALERTDIALOG live assertive, STATUS/LOG live polite, and maps `details.busy`. | The same generic busy mapping is available for application-owned semantic nodes; no Skeleton-specific mapping is needed. |
| Browser gallery | `web/gallery.js:670` maps role enum to DOM roles and `details.busy` to `aria-busy`. Stable IDs reuse DOM nodes. | `tests/web/test_status_motion.py` checks extended decorative motion emits no status/busy nodes. |
| Live AT | Alert announcement delivery is not established by component snapshots, DOM assertions or existing native action tests. | No Skeleton utterance is required by the audited source. Application loading announcements still need transport and AT validation if claimed. |

`include/cui/cui_content.h` exposes `cui_alert_options.accessible_name` and rich
slots. For custom slot contents, a meaningful explicit name matters: the fallback
message/title does not automatically summarize arbitrary children. The bounded
`cui_bubble_announcement` enum selects GROUP/STATUS/ALERT semantic roles; despite
its name it is not a Core announcement queue. Alert has no corresponding
polite/assertive selector: its role matches the frozen Alert root.

`tests/unit/test_alert_bubble_slots.c` checks Alert role, nonfocusable semantics,
custom children and Bubble role selection. `tests/web/test_alert_bubble_slots.py`
checks browser composition and interaction, not spoken announcements. Existing
`tests/unit/test_status_motion.c` explicitly checks zero semantic nodes for
extended Spinner/Skeleton, and preserves legacy Spinner's separate STATUS
behavior. That legacy Spinner behavior must not be mistaken for Skeleton output.

The AccessKit implementation at `backends/accesskit/cui_accessibility.c:192`
already sets live attributes. It copies bounded snapshot strings, retains tokens
for stable ID/role pairs, rejects invalid/capacity-overflow updates and suppresses
unchanged snapshots. These are useful lifetime and idle guarantees. They also
mean an unchanged Alert is not an event that must be resent every frame. A
remove/reinsert or changed name is a tree transition; whether the platform emits
an announcement, and its contents, requires evidence. There is no public
announcement acknowledgement or guarantee that an unchanged identical message
will be spoken again.

The optional package is pinned AccessKit C 0.23.0 at
`0824c4a1e3a4d13ce5582df20e394fba49485a15` (`third_party/accesskit/pin.json`).
`backends/accesskit/root_bridge.cmake` explicitly supports Linux only and rejects
cross compilation. `tests/integration/test_accessibility_native.c` verifies real
private-session AT-SPI button/checkbox roles, bounds, focus/actions, relations,
replacement and teardown. It does not create Alert/STATUS nodes or verify live
region event delivery. Existing native tests cannot be cited as announcement
proof. macOS/Windows adapters and their announcement delivery remain outside
this package's demonstrated support.

The gallery DOM mirror creates noninteractive alert/status `div`s with role and
`aria-label`; it does not add a dedicated persistent live-text transport or
explicit `aria-live` in `syncSemantics`. Role exposure and accessible naming can
be tested headlessly. Initial insertion, label-only mutation and repeated
identical content must not be advertised as reliably spoken without browser/AT
evidence. The gallery mirror is also a host implementation, not a universal
accessibility adapter installed for every CUI browser consumer.

## Actual remaining scope

For Alert, the source-level role and Core representation already exist. The
remaining reviewable work is transport-level evidence for Alert insertion,
content change, removal and idle behavior; it is not automatically a new Core
announcement API. Native live-attribute mapping exists but its platform event
behavior is untested here. A complete live-AT claim additionally requires an
opt-in screen-reader matrix, including initial mount versus update and naming of
rich content. Do not promote the manifest row merely because role assertions pass.

For Skeleton, retain the existing row as an explicit source-scope clarification.
Do not add a busy flag or live announcement to every animated placeholder. A
screen with twenty skeleton rows should have one application-owned loading
region/status, not twenty competing utterances. The application owns transition
from loading to loaded/failed, the text, and which region is busy. Existing Core
semantic details can represent this without changing Skeleton or public structs.

## Bounded next slice for review

Start with tests and an application-composition example using existing APIs;
keep public APIs, Core dispatch, Skeleton and adapter ownership unchanged.

1. Add deterministic Core fixtures for visible Alert, unchanged rerender, changed
   explicit name, hide/reinsert, rich-content explicit naming and independent
   close/action descendants. Assert roles, names, no extra tab stop and omitted
   hidden subtree. Compose multiple decorative skeletons inside one named
   application region with `details.busy`, plus one application-owned STATUS
   message. Assert exactly the intended semantic nodes through loading,
   completion and failure, and unchanged output during animation frames.
2. Extend adapter-focused fixtures to inspect actual AccessKit node live/busy
   properties and token/diff behavior. If inspection needs a seam, keep it private
   and testing-only, consistent with current adapter counter hooks. Exercise
   bounded string/node failure without partial replacement and idle suppression.
3. Extend the existing private DBus/Xvfb AT-SPI integration harness to query Alert
   role and busy state and collect whichever live/text/property events the pinned
   adapter actually emits. First inspect the pinned AccessKit Unix source to
   establish supported event mapping; do not assume an announcement event exists.
   Assert only supported, observed event types and payloads, with bounded waits
   and no idle duplicate emissions. An unsupported event path is a documented
   adapter gap requiring a separate design, not a reason to invent a Core queue.
4. Add browser DOM tests for stable identity, role, name, busy transition, removal
   and absence of decorative placeholder semantics. If a browser transport change
   is necessary, review it separately with persistent live-text region behavior
   and bounded deduplication/repeat semantics; snapshots alone cannot certify it.

All native tests use `scripts/test_local.py` and the adapter's private-session
runner, never the maintainer's desktop or accessibility bus. Browser tests remain
headless. Use strict C11 warnings and focused CTest targets; no dependency install
or live screen reader is part of automated tests. Keep actual speech perception,
interruption/coalescing, user preferences, browser/screen-reader combinations and
macOS/Windows delivery explicitly deferred until opt-in validation exists.

If later requirements include repeating identical announcements or delivery
acknowledgement, propose an independent bounded host transport with copied UTF-8,
explicit lifetime/overflow/coalescing rules and platform support evidence before
adding any public API. Those requirements are not proved by the frozen component
sources and are outside this next slice. Metadata/skill updates and any row
classification remain integrator-owned after the chosen evidence is reviewed.

## Bounded proof implementation (after this audit)

`examples/loading_region` now provides compiling application composition, Core
snapshot output and a focused external CMake harness; corresponding new Core and
headless DOM fixtures exercise transitions without changing public APIs.
`test_native.c` uses the existing verified optional cache and private DBus/Xvfb
runner to receive real AT-SPI announcement events, not synthetic callback events.
Core/native focused tests passed 2/2 in normal and ASan/UBSan builds; the
production-gallery-translator DOM fixture
passed in headless Chromium. No native window or screen reader was launched.

Inspection of the lockfile-selected `accesskit_atspi_common` 0.20.0 found named
live-node insertion announcements in `adapter.rs:add_node`, changed-name
announcements in `node.rs:notify_property_changes`, and live politeness mapping
in `node.rs:live`. `accesskit_unix` 0.23.0 `atspi/bus.rs` serializes message and
politeness in `object:announcement`. The real fixture verifies Alert insertion,
changed name, hide/reinsert and idle deduplication, completion STATUS politeness,
and busy true/false. The pinned Unix role mapping exposes AccessKit Alert as
AT-SPI Notification (AlertDialog maps to AT-SPI Alert); this mapping is asserted
without changing production code. Existing client cache-signature warnings do
not prevent the direct queries or event assertions, but remain a compatibility
observation. See the example README for commands and remaining live-AT/platform
limits. No manifest status promotion follows automatically from these results.
