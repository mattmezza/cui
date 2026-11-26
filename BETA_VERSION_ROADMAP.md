# CUI beta roadmap

Updated: **2026-11-26** · Status: **working alpha; beta scope unfinished**.

**Goal:** complete and verify all **78 frozen component families**, plus the
requested native, language-binding, documentation and website capabilities.
No completion percentage or beta date is asserted. Start with **Next milestones**;
use the checklists below to track acceptance and delivery.

## At a glance

| Track | Current status |
| --- | --- |
| Public release | **v0.1.0-alpha.11**, with native starter and language assets. [Release](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.11) |
| Component coverage | **All 78 families have tested subsets. Zero families are fully verified.** |
| Public website | Interactive WASM gallery and docs at alpha.11. The Pages deployment from `85df489c` served the exact release starter at publication. [Pages verification](https://github.com/mattmezza/cui/actions/runs/45888310574) |
| Latest integrated work | Retained Carousel supports both orientations, optional edge wrapping, arrows and slide markers with customizable content in C, Python, JavaScript, TypeScript, Go and Rust. Included in the next release batch. |
| Packages | Alpha.11 GitHub assets and a verified public Go module. PyPI, npm and crates.io publication remain open. |

**Implemented ≠ tested ≠ merged ≠ released ≠ deployed.** A gallery entry or green
feature test does not establish complete component parity.

## How to read this roadmap

- **Integrated:** accepted on the working integration branch; may be unreleased.
- **Test-only / drafted:** exploratory or worker changes, not supported production APIs.
- **Open checkbox:** acceptance or delivery is still required, even if a subset works.
- **Released / deployed:** verified published artifacts, tracked separately from source.

## Release wrap-up

Alpha.11 ships Accordion and Collapsible title states, retained Attachment
actions and slots, and viewport-relative Carousel slides. Installed examples
exercise the retained APIs in all five language builders. The release and
gallery serve the same native starter.
[Release evidence](docs/validation/alpha11-release.json)

Alpha.10 ships responsive Accordion and Collapsible sizing and paint,
the joined Accordion layout and styled headings and dividers. The installed
workspace preferences examples and starter match the qualified source.
[Release evidence](docs/validation/alpha10-release.json)

Alpha.9 ships configurable Radio indicators and controlled C selection, new
Rating marks, responsive Stepper Fill and retained VirtualList live rows. The
starter and installed examples passed against the same source revision.
[Release evidence](docs/validation/alpha9-release.json)

Alpha.8 ships Radio and RadioGroup motion, rich option content, tooltips and
signed Tab order. Installed language examples exercise all five bindings.
The public gallery and GitHub serve the same native starter. [Release evidence](docs/validation/alpha8-release.json)

Alpha.7 ships the Pagination, Notification, Attachment and Sidebar slices.
The native starter, installed language examples and public Go module were
verified against the release revision. [Release evidence](docs/validation/alpha7-release.json)

Alpha.6 shipped the ordered C TimeField with IME and accessible segment focus,
live-resize Input geometry, Textarea word navigation, Chart and Plot refinements,
and Carousel and Calendar fixes. The responsive native starter and language
packages match the validated source. [Release evidence](docs/validation/alpha6-release.json)

## Next milestones, in order

1. **Finish active slices.** Qualify TimeField IME and the localized examples on physical hosts, broaden SDL V2 lifecycle and renderer evidence, and exercise the responsive starter on physical devices. Keep SDK, amalgamation and skill resources aligned.
2. **Close component and ecosystem gaps:** reconcile every family with the frozen
   baseline and the reported regressions below; finish bindings and documentation.
3. **Validate the integrated revision:** compatibility, headless regressions,
   platform evidence, generated files and exact-revision CI/browser checks.
4. **Ship matching artifacts:** library/packages, website and reproducible examples.

### Active work and acceptance gaps

| Work | Landed evidence / next requirement |
| --- | --- |
| Dialog and scrolling | Six-slot C composition, intrinsic first-open sizing, long-body scrolling with a fixed Footer and idle quiescence pass focused native checks and the full headless browser gate (`f728908e` artifact). New headless regressions cover held mouse/touch/Space and stale accessibility activation on reopen. Table touch-drop is integrated. Alpha.5 deploys these changes. Broader lifecycle and foreign builders remain open. |
| TimeField | Localized examples pass headlessly from the alpha.5 Python wheel, npm tarball, Go module and Rust crate. Working source covers ordered IME in C, injected SDL3 and Chromium. Chromium AX and private Linux AT-SPI tests verify the committed time, segment values and delegated focus. Platform accessibility editing, physical IME and screen-reader checks, and interactive native binding examples remain open. [C contract](docs/TIME_FIELD_LOCALIZED.md) · [Binding evidence](docs/binding-coverage.json) |
| SDL rendering | V1 baseline and pure source-range design are recorded. Private preflight source passed initial focused checks, including 216 frozen-validator comparisons; private extraction is integrated as `16827ec9`. Its exact-limit boundary fixtures and frozen-validator comparison now pass three combined headless checks (0.16 seconds); broader rendering regressions remain open. Compile-only native/WASM guard measurements now preserve all 37 existing offsets and alignment; native object cost is +8 bytes, WASM +0. SDL V2 is now integrated (`e05071c1`): strict root build and three focused private-Xvfb tests pass, including actual software BGRA transfer, failure/reentry/restoration checks and the legacy ABI. Initial capability-query failures are retained; native format/limit handling was corrected. The C SDL_Renderer V2 route shipped in alpha.4. Broader renderer and platform acceptance remain open. SDL_Renderer software acceptance does not establish SDL_GPU, Sokol or hardware support. |
| Icon and Badge | Alpha.4 includes the C bridge SVG resource API. Working source adds retained Icon and Badge in C, Python, JavaScript, TypeScript, Go and Rust. Current and older providers are covered. Python and Node examples passed from offline-installed packages and in hidden SDL. The installed Go example passed headlessly and the source example in hidden SDL. Rust passed seven retained CTest groups, including source-built and shared libraries, hidden SDL, and actual alpha.4, pre-resource and incomplete Badge providers. The inbox passed in an independent offline project using the packaged Rust crate and its bundled C source. The retained builders ship in alpha.5. See the [Python](docs/PYTHON_ICON_BADGE.md), [Node](docs/NODE_ICON_BADGE.md), [Go](docs/GO_ICON_BADGE.md) and [Rust](docs/RUST_ICON_BADGE.md) inboxes. |
| Skeleton | Alpha.4 ships the owned palette and responsive C presentation, with a 38-preset oracle and headless C geometry tests. The subsequent profile gallery passes headless Chromium checks for desktop/mobile layout, palette/secondary/accent pixels and loading-to-content transitions. The optional retained C bridge passes software tests for responsive width, paint, motion, omission and recovery. Python, JavaScript, TypeScript, Go and Rust have responsive builders and runnable profile examples. Alpha.5 publishes those builders and the updated gallery. See [the presentation guide](docs/SKELETON_RESPONSIVE.md) and [bridge guide](docs/BRIDGE_SKELETON.md). |
| Native starter | Working source switches from a pinned Sidebar to a compact Sheet below MD. Private X11 checks verify intermediate rendering and focused page input across six widths. Headless checks cover navigation focus handoff, pinned account controls, clipped links, held mouse/touch and unsaved edits. Short Settings pages scroll by wheel, touch and form focus. Projects has a resizable list/detail pair and bounded tabs, verified with 48 private-X11 presentations across six widths and headless interaction tests. The extracted alpha.5 starter passes installed checks. Physical drag and other hosts remain open. See the [starter guide](docs/NATIVE_APP_STARTER.md) and [Linux evidence](docs/LINUX_LIVE_RESIZE.md). |
| Typed Slider | C/Python/JS/TS/Go/Rust authoring is integrated with focused checks. Rust current/legacy, 43 old/provider, four compile-fail and matching default-static UI/example checks pass; minimum-compiler acceptance remains open. Python/JS/TS/Go copied-wire evidence checks passed focused acceptance; the broader inactive-field matrix remains open. Alpha.4 contains the typed model and foreign bridge subsets. The subsequent opt-in Linux C adapter passes native AT-SPI Set Value and headless numeric mailbox checks. Foreign host opt-in, other OS adapters and physical screen-reader acceptance remain open. |
| Backgrounds, shadows and text | Public C software V2 query/render is integrated with 20 focused headless/freshness checks; released in alpha.4. Shared CPU helpers passed focused acceptance (including a corrected include-order fixture); owned-frame extraction (integrated; 24 headless checks and actual WASM layout compilation pass), SDL integration is now covered by focused checks; the other three adapters and foreign authoring APIs remain open. V2 transforms/owned text support up to 64×; the existing scaled-font API retains its separate 16× limit. |
| Validation | Alpha.11 passed hosted CI, native binding callbacks, all three browser shards and all six installed checks. [Release evidence](docs/validation/alpha11-release.json) |

**Resource rule:** reuse existing build directories, serialize heavy builds, and keep automated windows off the desktop. Remove only confirmed disposable artifacts; preserve source and independently landed work.

**Maintainer/external gates:** package-registry ownership/authentication and actual
macOS/Windows/GPU/assistive-technology acceptance need their own evidence. They do
not block source work that can be validated headlessly on Linux.

## Beta exit criteria

Beta is ready only when all five outcomes below have evidence:

1. **Components:** all 78 families audited; remaining behavior and reported demo regressions resolved or explicitly justified.
2. **Usable APIs:** consistent events, responsive layout, styling, typography and supported language bindings documented and tested.
3. **Reliable hosts:** native/window/media behavior and supported renderer/platform coverage verified, with deferred hardware checks labeled.
4. **Easy adoption:** reproducible app examples, concise human/agent docs and installable packages.
5. **Matching delivery:** integrated regressions pass and published library, packages and website match the validated source.

## Remaining beta scope

Each unchecked row is an **unfinished scope**, potentially containing landed
subsets. Detailed ledgers and headers remain authoritative.

| Done | Area | Remaining work |
| --- | --- | --- |
| [ ] | Component parity | Audit all 78 families against the frozen baseline; complete missing behavior, styling, lifecycle and documented exceptions. See the family breakdown below. |
| [ ] | Responsive layout and speed | Native starter and dashboard pass continuous resize on private X11. The SDL software inbox verifies same-frame hover in 1,000 and 10,000 rows. Physical input-to-display latency, resize on other hosts, intrinsic sizing, overflow and multiline rewrap remain open. Single-line Input follows each resolved width for caret, selection and IME. [Inbox measurements](docs/PERFORMANCE.md#native-datatable-inbox) |
| [ ] | Shadows and backgrounds | Complete root, nested-part and popup shadow ownership across components/bindings; finish gradients and renderer support, clipping, opacity, resource limits and failure behavior. Preserve legacy rendering. |
| [ ] | Typography and design tokens | Complete font-family/provider, weights/stretch/axes, tracking, line-height, wrapping and alignment coverage; consistent sizes, spacing, rounding and gradient text across advertised hosts/bindings. |
| [ ] | Events and async UX | All 78 C families have a documented event disposition and compatibility contract. The optional Linux async example has offline worker and transport tests. Extend foreign bindings and platform/accessibility proof within the documented contracts. [Event coverage](docs/design/COMPONENT_EVENTS_REMAINING.md) · [Async example](examples/async_fetch/README.md) |
| [ ] | Touch, states and scrollbars | Verify pointer/keyboard/touch, cancellation, nested scrolling, optional scrollbar styles, pan/pinch, custom cursors, disabled/loading/inert and hover/pressed/focus states wherever applicable. |
| [ ] | Accessibility and preferences | Complete semantic/adapter gaps and native system theme/contrast/reduced-motion observation. Real assistive-technology acceptance remains separate from semantic snapshot tests. |
| [ ] | Native windows and platforms | Broader hidden-window acceptance; preserve Retina/static-linking work. Verify Linux/macOS/Windows host lifecycle, DPI, renderer recovery and installed consumers. Explicitly identify unavailable physical-device checks. |
| [ ] | Language authoring | Finish component/style/event coverage in Python, JS/TS, Go and Rust; provide working authored UIs, callbacks and explicit omissions, rather than launcher-only examples. |
| [ ] | Package-manager delivery | Finish PyPI/npm/crates.io ownership/authentication/publication gates and clean installation checks; maintain verified Go module delivery and native dependency guidance. |
| [ ] | Human and agent docs | Keep build/compiler/linker prerequisites, quickstarts, state/events, immediate/retained distinctions, language examples and professional app recipes reproducible. Update metadata and generated skill together; keep discovery concise. |
| [ ] | Website and media | Keep gallery behavior, system/theme selection, search, copyable code, navigation scroll preservation, docs, version display, README media and OG coverage aligned with shipped APIs. Rendered docs and search summaries use CUI terminology with descriptive provenance links. |
| [ ] | CI, performance and resources | Eleven offline Chromium probes now fetch their local WASM as binary. The same-artifact Popover browser phase fell from 70.9 s to 2.1 s locally with all eight cases. Timestamp-preserving generators are integrated (`3088338a`) and verified across 1,170 files. Rust’s embedded CMake build honors Cargo NUM_JOBS with a one-job fallback. Portable hosted CI allows two test processes and bounded nested builds. Alpha.7 recorded zero extra push runs across both release tags after the branch-only filter. Alpha.6 had six duplicate runs across those two tags. Installed-package CI exercises both Go variants and runs the shared Rust crate once. Hosted test and rebuild timing comparisons remain open. [Measurements](docs/PERFORMANCE.md#hosted-check-scheduling) |

### Next component slices

The native starter now combines Resizable, Sidebar, Sheet and Tabs in Projects.
HoverCard remeasures changing content while open and settles after the placement
follow-up. Popover has an optional C trigger link and a keyboard focus example.
The gallery also exercises opt-in focus preservation on outside dismissal and
right-click toggling with keyboard and touch support.
Check reported regressions against the current gallery as each slice lands.

| Order | Component | Next outcome |
| --- | --- | --- |
| 1 | Icon / Badge / Skeleton | Alpha.5 ships the SVG and Badge builders and responsive Skeleton example. Keep targeting custom artwork, interactive Badge children and loading-to-content transitions in compatibility checks. |
| 2 | VideoPlayer / Video | The C player and WASM gallery have capability-gated mute and volume, with Chromium proof for keyboard and touch, decoded audio and player-only fullscreen. The optional native FFV1/PCM example applies gain after rendering and passes all 22 private-display/SDL dummy audio checks. Alpha.5 deploys the player. Physical audio and broader codecs remain open. [Player guide](docs/VIDEO_PLAYER.md) · [native sound example](examples/video_player_audio/README.md) |
| 3 | DataTable | Live updates preserve row position and selection. Private-X11 and headless Chromium cover scrolling, paging and keyboard continuation. C and all five language bindings support opt-in 10,000-row snapshots within a 32 MiB native app budget and runnable 40-row inboxes. Ordinary setters accept 32 rows. Alpha.5 ships the copied snapshots and inbox examples. Remote adapters remain open. [Capacity guide](docs/BRIDGE_COMPONENTS.md#retain-more-than-32-datatable-rows-in-c) |
| 3 | VirtualList | The WASM gallery has a variable-height **Live inbox** example with copyable C usage. Prepending and resizing above the viewport preserve the first visible item and its offset. Removing it moves the replacement to the top where scroll bounds allow. Alpha.5 deploys the C example. Alpha.9 adds retained C and five-language support for variable heights and order updates with opt-in anchoring. Headless checks include runnable inboxes and the alpha.7 fallback. Retained horizontal layouts, intrinsic measurement and physical accessibility remain open. [Live update guide](docs/VIRTUAL_LIST.md#preserve-position-during-live-updates) |
| 3 | Table | Alpha.5 deploys independent header backgrounds and alternating rows. Headless Chromium already verifies both themes and DataTable's stripes, keyed selection, sorting and scrolling. [Table colors](docs/TABLE.md#row-and-header-colors) |
| 4 | Sidebar / Sheet | Starter Projects and Settings use the same pinned account controls. Headless tests exercise navigation and focus across layout changes. The extracted alpha.5 starter passes. Physical host behavior remains to be verified. |
| 4 | Resizable | Projects remembers separate pane sizes for desktop and stacked layouts. Headless tests verify drag during resize, compact keyboard resizing and cancellation across axes. Private X11 confirms the seam and pane paint across all six host widths. Alpha.5 ships the starter. Physical-device checks remain open. |
| 4 | Tabs | Projects verifies bounded scrolling, horizontal touch pan, overflow selection and focus handoff when resize hides the trigger. Private X11 confirms the selected tab stays visible and focused. Alpha.5 ships the bounded-tabs starter example. |
| 4 | TitleBar | Exercise decorated Cocoa traffic lights and Retina placement on a Mac. Close documented native drag and system-window menu gaps by host capability. [TitleBar guide](docs/CHROME.md#title-bar) |
| 5 | Dialog / AlertDialog | Composed Base and Styled surfaces drop held mouse, touch and Space on reopen, with focus restored and fresh activation verified. Intrinsic sizing and fixed-Footer scrolling have focused checks. Finish root ownership and foreign slot builders. |
| 5 | Popover / HoverCard | HoverCard has compact, wrapped and near-edge WASM profiles with copyable C, verified live reflow and narrow touch placement. Popover has admitted-only C trigger relationships, explicit focus entry and Escape return, opt-in outside focus handoff, and a right-click toggle that keeps keyboard and tap activation. The gallery maintains a stable canvas while an outside touch closes an overflowing Popover. Alpha.5 deploys these fixes. Foreign builders and native accessibility remain open. |
| 5 | Select / Combobox / Command | Command grow-width row paint and hit targets follow the parent from the first frame, with a follow-up for width-dependent custom heights. Headless Chromium verifies Select trigger centering and full-width rows, keyboard selection and edge taps across Select, Combobox and Command. Alpha.5 deploys these fixes. Physical-device checks remain open. |
| 6 | Button / Checkbox / Radio / Switch / Slider | Check the shared size scale and hover, pressed, focus, disabled and loading states where applicable. Extend typed activation and value coverage in the bindings while preserving existing C callback timing. |
| 6 | Accordion / Collapsible | Immediate and retained C presenters support shared sizes, centered SVG indicators, responsive widths and per-trigger background, focus and cursor styles. The WASM Accordion uses a joined card with dividers after each section. Headless bridge tests and the workspace preferences example exercise copied styling, legacy queued events and opt-in single-open/collapse behavior. Python, JS/TS, Go and Rust expose the retained presentation features and queued callbacks with runnable preferences examples. Immediate C, retained C and the gallery include shared and per-item heading typography, expanded and disabled row boxes, responsive title wrapping and inset dividers. Retained C, Python, JS/TS, Go and Rust examples exercise those styles and queued events. Immediate C, retained C, the five language bindings and the gallery support hover, keyboard-focus and press title styles. Physical accessibility remains open. [Disclosure guide](docs/DISCLOSURE.md#joined-groups-and-trigger-styles) · [Retained C and bindings](docs/BRIDGE_COMPONENTS.md#style-retained-accordion-and-collapsible-from-c) |
| 6 | RadioGroup / Radio | The opt-in C group presenter and WASM gallery crossfade marks while selection updates synchronously. Standalone C Radio keeps its shared-integer contract. The retained C bridge and all five language bindings expose both fades with existing queued events. Headless C and Chromium cover motion, reduced motion and settling. Immediate C supports rich descriptions and decorative content. Retained Item descriptions and the gallery expose the supporting text semantically. The C bridge and all five bindings support opt-in passive decorative children. Tests cover current rendering and the alpha.7 fallback. Immediate C, the C bridge, all five language bindings and the gallery support option tooltips with focused tests. Immediate C, the gallery, retained C and all five bindings support signed tab ordering with one stop per group. Headless tests verify the earlier-provider fallback. Alpha.8 includes these Radio additions. Alpha.9 adds shared C and WASM paint callbacks and standalone controlled select-only requests. Retained C and all five language builders add copied paint layers. The delivery forms use them, and headless checks verify selected, hovered, pressed, focused and disabled appearance, geometry, and older-provider fallback. Synchronous requests in the bindings and native preference observation remain open. [C guide](docs/RADIO_GROUP.md#style-the-group-and-its-options) |
| 6 | Stepper | The alpha.9 WASM example defaults to fill connectors and switches axis as the preview shrinks or its type grows. It demonstrates validation, indicator feedback, keyboard and touch, and copyable C. Retained C, Python, JS/TS, Go and Rust add Fill with an explicit axis and extents while keeping queued changes. Fixed spacers and button layouts remain available. Intrinsic compression and native accessibility remain open. [Stepper guide](docs/STEPPER_NAVIGATION.md#wasm-gallery) |
| 6 | Rating | Alpha.9 C and WASM add shared sizes, custom SVG marks and colors, and hover preview that preserves the committed rating. Retained C and all five language builders expose copied configuration and shared SVG mask ownership while keeping queued scalar changes. Headless tests cover half steps, step-down, keyboard, touch and earlier providers. The feedback forms show how to update a label from the change callback. Platform accessibility remains open. [Rating guide](docs/NUMERIC.md#rating) |
| 6 | Pagination | C and the WASM gallery exercise shared sizes, responsive rows, a centered counter, configurable page budgets, icon-only arrows and scrolling ellipsis jump menus. C, the gallery and the retained bridge support localized hover hints on Previous/Next chevrons. Headless tests cover delayed mouse and pen hover, bounds, dismissal, and no hint on tap. Native and Chromium tests cover focus return, stale choices, the 100-page bound and mobile touch. The retained C bridge, Python, JS/TS, Go and Rust offer the extended options with the existing scalar event. Python, Node, Go and Rust headless checks also cover ordinary paging with the published alpha.6 bridge. Immediate C, retained C, the gallery and all five language builders support fractional custom heights with named-size restoration. Headless tests cover older and incomplete providers. Extended Pagination uses shared Button paint for hover, press, keyboard focus and open menus, with a current-page outline and pointer cursor. Chromium checks light and dark canvas pixels. These changes ship in alpha.7. Physical-device acceptance remains open. [Frame guide](docs/NUMERIC.md#pagination) · [retained C guide](docs/BRIDGE_COMPONENTS.md#configure-a-retained-pager-from-c) |
| 6 | Input / Textarea | Native regressions cover the caret and IME anchor at a full-width Textarea wrap. Headless Chromium checks Alt+Arrow word movement and selection across wraps. Single-line Input retains its existing Up/Down policy. Growing Input and InputGroup place the caret and selection in the current width. Chromium exercises resize with active preedit. Multiline rewrap and physical IME acceptance remain open. Keep new Editor features lower priority. |
| 7 | Notification | The opt-in C presenter and gallery provide wrapped text, a semibold title and a dedicated Close lane. Long messages can stack the action below. Passive persistent notices support keyboard and touch dismissal before hover. Headless C and Chromium exercise narrow layouts and isolated Review/Close activation, with the original C renderer covered by a legacy geometry test. Automatic Root presentation, foreign authoring, built-in OS delivery and physical accessibility acceptance remain open. [Notification guide](docs/STATUS_COMPONENTS.md#notification) |
| 7 | Attachment | The opt-in C presenter and gallery fit long filenames and descriptions with one-line grapheme ellipsis, full semantic labels and independent typography. The card reserves space for progress and retry. Headless C verifies text geometry, action dispatch and the original entry points. Chromium verifies both orientations, small cards, real previews and independent actions. Retained C and all five language facades support copied metadata styles, lifecycle overrides, shared sizes and real RGBA previews. Retained C and the five language builders queue Open, Remove and Retry with input evidence. Language action examples use manual polling. Retained C and all five language builders support preview fallback, overlays, metadata and action slots with headless geometry and child-event checks. The WASM gallery includes a copyable C card demonstrating all four slots. Physical accessibility remains open. [Attachment guide](docs/MEDIA.md#fit-file-names-and-descriptions) · [Retained events](docs/ATTACHMENT_EVENTS.md#retained-actions) |
| 7 | DatePicker / TimeField | Chromium covers localized historical date selection by keyboard and touch, including batches across year pages and popup focus return. The ordered TimeField gallery covers composition, preview-edge navigation and named accessible values. Physical native input and broader accessibility actions remain open. [DatePicker guide](docs/DATE_LOCALE_YEAR.md) · [Focus routing](docs/design/TIME_FIELD_FOCUS_ROUTING.md) |
| 7 | Chart / Plot | Chromium verifies continuous strokes and optional circular Line markers at desktop/mobile widths and 1×/2× display scales. Plot mouse/touch zoom and slow touch pan pass. Chromium verifies page handoff over disabled, horizontal-only and vertically bounded Plots. Linux headless tests exercise dispatch evidence and the legacy capture policy. Physical touch and native ancestor handoff remain open. |

### Component gaps to close

| Families / area | Outstanding acceptance scope |
| --- | --- |
| Controls | Configured Badge artwork has a direct C route and a retained C container. The strict gallery WASM build and full headless browser gate passed on artifact `f728908e`, including configured artwork and child activation. All 42 representative touch cases also pass. Python, JavaScript, TypeScript, Go and Rust expose configured authoring in alpha.5. Button state/style refinement, foreign Slider host accessibility and expanded builder styling remain open. Linux C Slider Value transport is verified with an independent AT-SPI client. Consistent size, text alignment, icons, rounding and shadows across controls remain open. |
| Date/time/color | Remaining parsing, formatting, bounds, keyboard, locale and accessibility contracts. Preserve existing locale/year navigation. |
| Overlays | Finish Dialog root and host lifecycle; finish Popover focus/relationships/placement and HoverCard lifecycle/dismissal. Explicit collapsed semantics require a reviewed contract. |
| Tables | Table selection/sorting/resizing/themed cells/keyboard; DataTable virtualization, remote callbacks, semantics and consistent scrolling. |
| Charts/plots | Scales, axes/ticks, series/legend, layout/motion, tooltips, selection and accessibility; verify existing pan/zoom/touch. |
| App structure | Carousel arrows and pagination honor Disabled in native and WASM tests. Horizontal touch yields vertical page pans. Sidebar has native and Chromium proof for pinned clipping, proportional scroll/drag and focus reveal on mounted rows. Carousel has viewport-relative slide sizing and optional wrapping on both axes in immediate C, retained C and language builders, and the WASM gallery. Retained arrows and wrapping slide markers share Carousel selection events and accept passive custom content. Intrinsic content sizing, custom retained control placement, broader Sidebar composition and platform accessibility, Settings, Command lifecycle/width and Root/window services remain open. Verify composed apps. |
| Text/editing | TextView parser/plugins/selection/typography and Textarea behavior. Questionnaire batched input and confirmation have native regressions. Its remaining work includes styling, localization, nested-root integration and accessibility. Editor expansion remains lower priority. |
| Video/audio/speech | Verify physical audio and media lifecycle on advertised hosts, and expand native codec coverage. Keep browser decoding, native dummy audio and physical audio output as separate checks. |
| Every other family | Audit current code, frozen behavior and limitations even when recorded feature rows are green. Fix outstanding reported demo regressions; do not infer whole-family completion. |

For each family, verify applicable state ownership, callbacks/results,
keyboard/pointer/touch, focus/accessibility, visual states, responsive bounds,
omission/removal and resource lifetime. Record justified nonvisual/host exceptions.

### Reported regressions: closure checklist

These groups remain open until existing fixes are reconciled with regression
coverage and the deployed gallery. This is an acceptance checklist, not a claim
that every listed defect still reproduces.

- [ ] **Alignment and sizing:** button/control text, Accordion caret, Avatar initials,
  Pagination counter, Select label, Tooltip, DatePicker, AlertDialog and HoverCard
  content sizing; all applicable size tokens, including Attachment layout.
- [ ] **Images and presentation:** real Attachment/Image previews, avatar stack,
  semantic Alert/Bubble variants, reusable Message demo, Notification action/close
  spacing and Markdown code blocks.
- [ ] **Input and selection:** Radio exclusivity, Rating stars, Stepper behavior,
  Calendar icons, locale/year selection, full-width Select/Combobox/Command,
  InputGroup wrapping and text navigation/scrolling regressions.
- [ ] **Layout and navigation:** working Resizable/Dock/VirtualList demos, Sidebar
  clipping, bounded Tabs scrolling/overflow button, meaningful Toolbar find and
  Theme demos, native TitleBar capability/fallback, gallery scroll preservation.
- [ ] **Data and motion:** Table/DataTable sorting and consistent scrolling,
  alternate/header backgrounds, Chart line quality, Plot pan/zoom/pinch,
  Progress and Sheet animation, optional scrollbar appearance only where useful.
- [ ] **Media:** reusable player plus base video surface; visible seek controls,
  video-only fullscreen, audible playback and documented host support.

## Validation and delivery gates

- [ ] All 78 families reconciled with [the feature ledger](docs/gpui-parity.json),
  current limitations and [binding coverage](docs/binding-coverage.json).
- [ ] Warnings, meaningful regressions, legacy compatibility, capacity/failure/
  lifetime tests and applicable CI checks pass on the integrated revision.
- [ ] Generated distributions, source metadata and consumer skill are fresh.
- [ ] Native Linux tests use `python scripts/test_local.py --build-dir build`;
  browsers run headless. No automated windows on the maintainer’s desktop.
- [ ] [Platform evidence](docs/PLATFORM_MATRIX.md) distinguishes compile, runtime
  and physical GPU/OS/assistive-technology acceptance; unsupported checks stay deferred.
- [ ] Performance and memory claims have reproducible evidence and limitations.
- [ ] Exact release artifacts, provenance, checksums, installed examples and
  deployed website revision verified before declaring delivery complete.

Historical source evidence and remaining acceptance:

| Slice | Evidence | Still required |
| --- | --- | --- |
| Combined Linux regression | Alpha.4 `8597916f`: strict 1,214-step build; 606/607 checks pass, zero CTest skips (619.03s); sole generated-summary repair passes six affected checks | Preserve the original failed run; do not report a single607/607 run. Exact-revision hosted/platform/delivery checks remain. |
| Dialog composition | `ef893a34` idle fix; focused native checks and full browser gate on artifact `f728908e` pass | Broader lifecycle. Alpha.5 deploys the focused behavior and nested-touch checks pass. |
| Localized TimeField | Integrated `49267cc4`; native/source/ABI checks and keyboard gallery gates pass. Working source covers ordered IME and formatted semantic values. Chromium AX and Linux AT-SPI verify the current time, discrete values and segment focus. Released language artifacts pass headless examples. | Physical IME, assistive-technology editing and screen-reader qualification, and interactive native binding acceptance. |

Receipts travel with their source changes. Use the alpha.11 receipt above for
exact-source acceptance and delivery.

Detailed validation receipts live in [docs/validation](docs/validation).
Older green runs do not cover newer source; worker checks do not establish
integrated, released or deployed support.

## Deferred or lower priority

- **Incremental repainting:** separate exploration in [issue #1](https://github.com/mattmezza/cui/issues/1), not an added beta blocker.
- **Editor expansion and ColorPicker polish:** lower priority; existing regressions still require fixes.
- **v1.0:** follows beta acceptance; see [v1 readiness](docs/V1_READINESS.md).

## Maintenance rule for humans and agents

This is the repository’s running beta checklist, not a replacement for the detailed
API contracts or validation receipts. Keep status summaries short and link evidence.

Update this document **in the same change** that completes a slice or discovers a
new gap. Update the corresponding detailed ledger/guide too. Check a row only when
its entire scope is complete; attach concise test/commit/release evidence and
retain unresolved exceptions. Refresh the date and delivery facts only when they
change. Keep all 78 manifest entries and preserve independently landed main work.

Detailed references: [component ledger](docs/gpui-parity.json) ·
[binding coverage](docs/binding-coverage.json) ·
[platform matrix](docs/PLATFORM_MATRIX.md) ·
[release gates](docs/RELEASING.md) · [v1 readiness](docs/V1_READINESS.md).
