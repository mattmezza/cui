# CUI beta roadmap

Updated: **2026-11-25** · Status: **working alpha; beta scope unfinished**.

**Goal:** complete and verify all **78 frozen component families**, plus the
requested native, language-binding, documentation and website capabilities.
No completion percentage or beta date is asserted. Start with **Next milestones**;
use the checklists below to track acceptance and delivery.

## At a glance

| Track | Current status |
| --- | --- |
| Public release | **v0.1.0-alpha.4**, including native starter and language assets; [release](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.4). |
| Component coverage | **78 families: 77 tested subsets, 1 implemented, 0 fully verified.** These counts are not a completion percentage. |
| Public website | Interactive WASM gallery, docs and API index; alpha.4 source at `17533b89`; [documentation deployment](https://github.com/mattmezza/cui/actions/runs/41308009221) at `4752885b` passed. |
| Working source | Alpha.4 released from `17533b89`. Exact-source hosted CI/native bindings/Pages pass. Installed C/Python/JS/TS/Go/Rust, real-font text and native starter checks pass. Broader platform/parity acceptance remains open. |
| Packages | Alpha.4 GitHub assets and verified Go tag/proxy; PyPI, npm and crates.io publication remains open. |

**Implemented ≠ tested ≠ merged ≠ released ≠ deployed.** A gallery entry or green
feature test does not establish complete component parity.

## How to read this roadmap

- **Integrated:** accepted on the working integration branch; may be unreleased.
- **Test-only / drafted:** exploratory or worker changes, not supported production APIs.
- **Open checkbox:** acceptance or delivery is still required, even if a subset works.
- **Released / deployed:** verified published artifacts, tracked separately from source.

## Release wrap-up: alpha.4 complete

Alpha.4 source, native starter, library/language assets and website are published
and verified. Older releases remain immutable. Disposable worktrees/builds were
cleaned while retaining unfinished source and evidence. Parity work resumed on
2026-11-24.

## Next milestones, in order

1. **Finish active slices.** Prepare configured Icon and Badge packages for publication. The inbox examples pass from offline-installed artifacts across the five language bindings. Responsive Skeleton examples are authored in all five languages. Localized TimeField facades have focused dynamic and bundled-source static acceptance. Keep SDK, amalgamation and skill resources aligned. SDL V2 and Python have focused root acceptance. Broader regressions, lifecycle and delivery remain open.
2. **Close component and ecosystem gaps:** reconcile every family with the frozen
   baseline and the reported regressions below; finish bindings and documentation.
3. **Validate the integrated revision:** compatibility, headless regressions,
   platform evidence, generated files and exact-revision CI/browser checks.
4. **Ship matching artifacts:** library/packages, website and reproducible examples.

### Active work and acceptance gaps

| Work | Landed evidence / next requirement |
| --- | --- |
| Dialog and scrolling | Six-slot C composition, intrinsic first-open sizing, long-body scrolling with a fixed Footer and idle quiescence pass focused native checks and the full headless browser gate (`f728908e` artifact). New headless regressions cover held mouse/touch/Space and stale accessibility activation on reopen. Table touch-drop is integrated. Broader lifecycle, foreign builders and deployment remain open. |
| TimeField | Localized C widget, Base/Styled example and pure text helpers are integrated. Source/distribution, C++/WASM, focused native and real Chromium keyboard/H12/Arabic/read-only/bounds checks pass; the full catalog browser gate also passes. The localized C bridge is integrated (`f6a3bf37`): ten root checks plus three shared/static-example/frozen-legacy tests pass. Python authoring passes sixteen configured root checks, including actual current/old/partial-provider compatibility and the headless UI example; package/native-interactive acceptance remains open. Go/Rust configured root dynamic acceptance passes with current/old/partial providers and both authored examples; bundled-source static acceptance also passes three root checks; installed-package/MSRV/native-interactive acceptance remains open. JS/TS strict root build and all 81 configured Node checks pass, including 20 localized checks and strict TypeScript. Package/native-interactive acceptance remains open. One-root Tab, IME and broader platform acceptance remain open. Contract: `docs/design/TIME_FIELD_BINDING_CHECKPOINT.md`. |
| SDL rendering | V1 baseline and pure source-range design are recorded. Private preflight source passed initial focused checks, including 216 frozen-validator comparisons; private extraction is integrated as `16827ec9`. Its exact-limit boundary fixtures and frozen-validator comparison now pass three combined headless checks (0.16 seconds); broader rendering regressions remain open. Compile-only native/WASM guard measurements now preserve all 37 existing offsets and alignment; native object cost is +8 bytes, WASM +0. SDL V2 is now integrated (`e05071c1`): strict root build and three focused private-Xvfb tests pass, including actual software BGRA transfer, failure/reentry/restoration checks and the legacy ABI. Initial capability-query failures are retained; native format/limit handling was corrected. Generated-consumer checks passed for that slice; newer binding changes require regeneration. Broader regression and platform acceptance remain open; unreleased. SDL_Renderer software acceptance does not establish SDL_GPU, Sokol or hardware support. |
| Icon and Badge | Alpha.4 includes the C bridge SVG resource API. Working source adds retained Icon and Badge in C, Python, JavaScript, TypeScript, Go and Rust. Current and older providers are covered. Python and Node examples passed from offline-installed packages and in hidden SDL. The installed Go example passed headlessly and the source example in hidden SDL. Rust passed seven retained CTest groups, including source-built and shared libraries, hidden SDL, and actual alpha.4, pre-resource and incomplete Badge providers. The inbox passed in an independent offline project using the packaged Rust crate and its bundled C source. Publication remains open. See the [Python](docs/PYTHON_ICON_BADGE.md), [Node](docs/NODE_ICON_BADGE.md), [Go](docs/GO_ICON_BADGE.md) and [Rust](docs/RUST_ICON_BADGE.md) inboxes. |
| Skeleton | Alpha.4 ships the owned palette and responsive C presentation, with a 38-preset oracle and headless C geometry tests. The subsequent profile gallery passes headless Chromium checks for desktop/mobile layout, palette/secondary/accent pixels and loading-to-content transitions. The optional retained C bridge passes software tests for responsive width, paint, motion, omission and recovery. Python, JavaScript, TypeScript, Go and Rust have responsive builders and runnable profile examples. Publishing those builders and the updated gallery remains open. See [the presentation guide](docs/SKELETON_RESPONSIVE.md) and [bridge guide](docs/BRIDGE_SKELETON.md). |
| Native starter | Working source switches from a pinned Sidebar to a compact Sheet below MD. Private X11 checks verify intermediate rendering and focused page input across six widths. Headless checks cover navigation focus handoff, pinned account controls, clipped links, held mouse/touch and unsaved edits. Short Settings pages scroll by wheel, touch and form focus. Projects has a resizable list/detail pair and bounded tabs, verified with 48 private-X11 presentations across six widths and headless interaction tests. Physical drag, other hosts and publication remain open. See the [starter guide](docs/NATIVE_APP_STARTER.md) and [Linux evidence](docs/LINUX_LIVE_RESIZE.md). |
| Typed Slider | C/Python/JS/TS/Go/Rust authoring is integrated with focused checks. Rust current/legacy, 43 old/provider, four compile-fail and matching default-static UI/example checks pass; minimum-compiler acceptance remains open. Python/JS/TS/Go copied-wire evidence checks passed focused acceptance; the broader inactive-field matrix remains open. Alpha.4 contains the typed model and foreign bridge subsets. The subsequent opt-in Linux C adapter passes native AT-SPI Set Value and headless numeric mailbox checks. Foreign host opt-in, other OS adapters and physical screen-reader acceptance remain open. |
| Backgrounds, shadows and text | Public C software V2 query/render is integrated with 20 focused headless/freshness checks; released in alpha.4. Shared CPU helpers passed focused acceptance (including a corrected include-order fixture); owned-frame extraction (integrated; 24 headless checks and actual WASM layout compilation pass), SDL integration is now covered by focused checks; the other three adapters and foreign authoring APIs remain open. V2 transforms/owned text support up to 64×; the existing scaled-font API retains its separate 16× limit. |
| Validation | Exact alpha.4 hosted matrix/native bindings/Pages pass; installed consumers and starter pass. Earlier local 606/607 run had one documentation mismatch, repaired with 6/6 affected checks; 19 current skill tests pass. See [release evidence](docs/validation/alpha4-release.json). |

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
| [ ] | Responsive layout and speed | Native starter and dashboard pass continuous resize on private X11. Verify physical drag and other native backends, and measure hover through a long inbox. Finish intrinsic sizing, overflow and current-frame caret/selection/IME projection. |
| [ ] | Shadows and backgrounds | Complete root, nested-part and popup shadow ownership across components/bindings; finish gradients and renderer support, clipping, opacity, resource limits and failure behavior. Preserve legacy rendering. |
| [ ] | Typography and design tokens | Complete font-family/provider, weights/stretch/axes, tracking, line-height, wrapping and alignment coverage; consistent sizes, spacing, rounding and gradient text across advertised hosts/bindings. |
| [ ] | Events and async UX | Finish the event coverage/exception matrix, typed contract and binding support; preserve legacy timing/mutations. Verify offline async fetch, wakeup, loading, retry, stale responses, cancellation and pending-work shutdown. |
| [ ] | Touch, states and scrollbars | Verify pointer/keyboard/touch, cancellation, nested scrolling, optional scrollbar styles, pan/pinch, custom cursors, disabled/loading/inert and hover/pressed/focus states wherever applicable. |
| [ ] | Accessibility and preferences | Complete semantic/adapter gaps and native system theme/contrast/reduced-motion observation. Real assistive-technology acceptance remains separate from semantic snapshot tests. |
| [ ] | Native windows and platforms | Broader hidden-window acceptance; preserve Retina/static-linking work. Verify Linux/macOS/Windows host lifecycle, DPI, renderer recovery and installed consumers. Explicitly identify unavailable physical-device checks. |
| [ ] | Language authoring | Finish component/style/event coverage in Python, JS/TS, Go and Rust; provide working authored UIs, callbacks and explicit omissions, rather than launcher-only examples. |
| [ ] | Package-manager delivery | Finish PyPI/npm/crates.io ownership/authentication/publication gates and clean installation checks; maintain verified Go module delivery and native dependency guidance. |
| [ ] | Human and agent docs | Keep build/compiler/linker prerequisites, quickstarts, state/events, immediate/retained distinctions, language examples and professional app recipes reproducible. Update metadata and generated skill together; keep discovery concise. |
| [ ] | Website and media | Keep gallery behavior, system/theme selection, search, copyable code, navigation scroll preservation, docs, version display, README media and OG coverage aligned with shipped APIs. |
| [ ] | CI, performance and resources | Eleven offline Chromium probes now fetch their local WASM as binary. The same-artifact Popover browser phase fell from 70.9 s to 2.1 s locally with all eight cases. Timestamp-preserving generators are integrated (`3088338a`) and verified across 1,170 files. Rust’s embedded CMake build honors Cargo NUM_JOBS with a one-job fallback. Portable hosted CI allows two test processes and bounded nested builds. Hosted timings and rebuild savings remain to be measured. [Measurements](docs/PERFORMANCE.md#browser-probe-transport) |

### Next component slices

The native starter now combines Resizable, Sidebar, Sheet and Tabs in Projects.
HoverCard remeasures changing content while open and settles after the placement
follow-up. Popover has an optional C trigger link and a keyboard focus example.
The gallery also exercises opt-in focus preservation on outside dismissal and
right-click toggling with keyboard and touch support.
Check reported regressions against the current gallery as each slice lands.
Ship the verified builders with the next release.

| Order | Component | Next outcome |
| --- | --- | --- |
| 1 | Icon / Badge / Skeleton | Publish the verified SVG and Badge builders and responsive Skeleton example. Check custom artwork, an interactive Badge child, and the loading-to-content transition from installed consumers and the deployed gallery. |
| 2 | VideoPlayer / Video | The C player and WASM gallery have capability-gated mute and volume, with Chromium proof for keyboard and touch, decoded audio and player-only fullscreen. The optional native FFV1/PCM example applies gain after rendering and passes all 22 private-display/SDL dummy audio checks. Deployment, physical audio and broader codecs remain open. [Player guide](docs/VIDEO_PLAYER.md) · [native sound example](examples/video_player_audio/README.md) |
| 3 | DataTable | Opt-in C scroll anchors preserve the first visible row and its offset on rebind. The 1,000- and 10,000-row private-X11 inbox checks preserve selection, position and focus, with same-frame hover and Down navigation after a prepend. Chromium verifies the **Live updates** gallery example with copyable C code, focus and selection during a timed arrival, and cancellation when switching modes. The offline **Paged inbox** checks near-end requests, one pending load, retained selection/focus and position after append, cancellation and end of data. The retained C bridge now supports transactional keyed replacements on up to 32 rows. Headless checks cover exact viewport offset, selection, keyboard continuation, invalid-key rollback and legacy reset. Python exposes keyed updates and a runnable inbox. Legacy calls still work against alpha.4. JS/TS, Go and Rust, remote adapters and publication remain open. [Python inbox](docs/BINDING_BUILDERS.md#keep-a-python-datatables-scroll-position-during-updates) · [Keyed bridge](docs/BRIDGE_COMPONENTS.md#replace-datatable-rows-without-losing-the-viewport) · [Live update API](docs/TABLE.md#keep-the-visible-row-during-a-live-update) |
| 3 | VirtualList | The WASM gallery has a variable-height **Live inbox** example with copyable C usage. Prepending and resizing above the viewport preserve the first visible item and its offset. Removing it moves the replacement to the top where scroll bounds allow. Release and foreign builder support remain open. [Live update guide](docs/VIRTUAL_LIST.md#preserve-position-during-live-updates) |
| 3 | Table | Deploy the independent header background and alternating-row controls. Headless Chromium already verifies both themes and DataTable's stripes, keyed selection, sorting and scrolling. [Table colors](docs/TABLE.md#row-and-header-colors) |
| 4 | Sidebar / Sheet | Starter Projects and Settings use the same pinned account controls. Headless tests exercise navigation and focus across layout changes. Verify the published starter and physical host behavior. |
| 4 | Resizable | Projects remembers separate pane sizes for desktop and stacked layouts. Headless tests verify drag during resize, compact keyboard resizing and cancellation across axes. Private X11 confirms the seam and pane paint across all six host widths. Publication and physical devices remain open. |
| 4 | Tabs | Projects verifies bounded scrolling, horizontal touch pan, overflow selection and focus handoff when resize hides the trigger. Private X11 confirms the selected tab stays visible and focused. Existing library focus behavior stays available. Publication remains open. |
| 4 | TitleBar | Exercise decorated Cocoa traffic lights and Retina placement on a Mac. Close documented native drag and system-window menu gaps by host capability. [TitleBar guide](docs/CHROME.md#title-bar) |
| 5 | Dialog / AlertDialog | Composed Base and Styled surfaces drop held mouse, touch and Space on reopen, with focus restored and fresh activation verified. Intrinsic sizing and fixed-Footer scrolling have focused checks. Finish root ownership and foreign slot builders. |
| 5 | Popover / HoverCard | HoverCard has compact, wrapped and near-edge WASM profiles with copyable C, verified live reflow and narrow touch placement. Popover has admitted-only C trigger relationships, explicit focus entry and Escape return, opt-in outside focus handoff, and a right-click toggle that keeps keyboard and tap activation. Deployment, foreign builders and native accessibility remain open. |
| 5 | Select / Combobox / Command | Command grow-width row paint and hit targets follow the parent from the first frame, with a follow-up for width-dependent custom heights. Headless Chromium verifies Select trigger centering and full-width rows, keyboard selection and edge taps across Select, Combobox and Command. Deployment and physical device checks remain open. |
| 6 | Button / Checkbox / Radio / Switch / Slider | Check the shared size scale and hover, pressed, focus, disabled and loading states where applicable. Extend typed activation and value coverage in the bindings while preserving existing C callback timing. |
| 6 | Input / Textarea | Verify word movement, line movement, caret/selection visibility and IME placement across wrapped or scrolled content. Keep new Editor features lower priority. |
| 7 | DatePicker / TimeField | Exercise direct year selection, localized formatting and range bounds with keyboard and touch. Finish installed TimeField examples and IME checks. |
| 7 | Chart / Plot | Verify that thin curves remain continuous across scales and that existing pan/pinch/zoom works without hijacking the parent scroll region when the plot cannot use the gesture. |

### Component gaps to close

| Families / area | Outstanding acceptance scope |
| --- | --- |
| Controls | Configured Badge artwork has a direct C route and a retained C container. The strict gallery WASM build and full headless browser gate passed on artifact `f728908e`, including configured artwork and child activation. All 42 representative touch cases also pass. Python, JavaScript, TypeScript, Go and Rust expose configured authoring in working source. Deployment remains open. Button state/style refinement, foreign Slider host accessibility and expanded builder styling remain open. Linux C Slider Value transport is verified with an independent AT-SPI client. Consistent size, text alignment, icons, rounding and shadows across controls remain open. |
| Date/time/color | Remaining parsing, formatting, bounds, keyboard, locale and accessibility contracts. Preserve existing locale/year navigation. |
| Overlays | Finish Dialog root and host lifecycle; finish Popover focus/relationships/placement and HoverCard lifecycle/dismissal. Explicit collapsed semantics require a reviewed contract. |
| Tables | Table selection/sorting/resizing/themed cells/keyboard; DataTable virtualization, remote callbacks, semantics and consistent scrolling. |
| Charts/plots | Scales, axes/ticks, series/legend, layout/motion, tooltips, selection and accessibility; verify existing pan/zoom/touch. |
| App structure | Carousel intrinsic layout, Sidebar clipping/scrolling, Settings, Command lifecycle/width and Root/window services; verify composed apps. |
| Text/editing | TextView parser/plugins/selection/typography, Textarea behavior and Questionnaire model/navigation/semantics. Editor expansion remains lower priority. |
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

Accepted source evidence (now on `main`; not yet released):

| Slice | Evidence | Still required |
| --- | --- | --- |
| Combined Linux regression | Alpha.4 `8597916f`: strict 1,214-step build; 606/607 checks pass, zero CTest skips (619.03s); sole generated-summary repair passes six affected checks | Preserve the original failed run; do not report a single607/607 run. Exact-revision hosted/platform/delivery checks remain. |
| Dialog composition | `ef893a34` idle fix; focused native checks and full browser gate on artifact `f728908e` pass | Broader lifecycle and deployment; nested-touch checks pass. |
| Localized TimeField | Integrated `49267cc4`; native/source/ABI checks and focused plus full browser gates pass | Broader touch/IME and platform coverage; C bridge/Python have focused root acceptance, Go/Rust bundled-source static acceptance passes; broader package/native acceptance remains open. |

Receipts travel with their source changes. Worker-only evidence remains distinct
from root acceptance and does not establish the final alpha.4 release revision.

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
