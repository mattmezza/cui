# CUI beta roadmap

Updated: **2026-11-01** · Status: **working alpha; beta scope unfinished**.

**Goal:** complete and verify all **78 frozen component families**, plus the
requested native, language-binding, documentation and website capabilities.
No completion percentage or beta date is asserted. Start with **Next milestones**;
use the checklists below to track acceptance and delivery.

## At a glance

| Track | Current status |
| --- | --- |
| Public release | **v0.1.0-alpha.3**; [release assets](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.3) available. |
| Component coverage | **78 families: 77 tested subsets, 1 implemented, 0 fully verified.** These counts are not a completion percentage. |
| Public website | Interactive WASM gallery, docs and API index. Last independently verified deployment: `c8e222bb50f79ad6faa3195295bb6d4dc604e384`. |
| Integration branch | New Slider, scroll geometry, background/text preparation and software V2 rendering work is **unreleased**. The combined Linux build and 541 headless tests pass; final browser/platform acceptance and publication remain unfinished. |
| Packages | Go alpha.3 tag/proxy verified; PyPI, npm and crates.io publication remains open. |

**Implemented ≠ tested ≠ merged ≠ released ≠ deployed.** A gallery entry or green
feature test does not establish complete component parity.

## How to read this roadmap

- **Integrated:** accepted on the working integration branch; may be unreleased.
- **Test-only / drafted:** exploratory or worker changes, not supported production APIs.
- **Open checkbox:** acceptance or delivery is still required, even if a subset works.
- **Released / deployed:** verified published artifacts, tracked separately from source.

## Next milestones, in order

1. **Finish active slices:** validate Dialog/AlertDialog gallery behavior, localized TimeField editing and the private SDL source-range preflight. Table touch and pure time-text helpers are integrated with focused checks; full regression and delivery remain open.
2. **Close component and ecosystem gaps:** reconcile every family with the frozen
   baseline and the reported regressions below; finish bindings and documentation.
3. **Validate the integrated revision:** compatibility, headless regressions,
   platform evidence, generated files and exact-revision CI/browser checks.
4. **Ship matching artifacts:** library/packages, website and reproducible examples.

### Active work and acceptance gaps

| Work | Landed evidence / next requirement |
| --- | --- |
| Dialog and scrolling | Table touch-drop admission is integrated with source/distribution runtime evidence and skill18 passing. Dialog/AlertDialog first-open sizing, six-slot composition and gallery migration are integrated at `b234adeb`. Combined native build and eight focused checks pass; worker C++/WASM and scrollbar checks pass. The first-frame browser check found 43 pixels of unintended short-Body overflow. The gallery status slot now explicitly fits its content (`a9938d04`), preserving source defaults; the real-control headless regression passes. The rebuilt browser run passes first-open sizing and fixed-Footer scrolling, then exposes continuous idle redraw; that regression and the remaining browser scenarios are open. Long-body and nested-touch acceptance remain open. Held-input reopening, broader cancellation and gallery acceptance remain open; earlier lifecycle prototypes are not production support. |
| TimeField | Pure locale-aware parsing/formatting and bounds helpers passed strict source/distribution, five headless, C++ header and WASM layout checks. Six legacy-consumer tests, usage6 and skill18 also pass in the worker tree. Integrated as `173e5d48`; combined-revision five headless tests and skill18 pass. Full regression and hosted validation remain open. Localized widget and the complete Base/Styled example now pass strict worker builds and focused runtime checks, including batching/focus, rejection, inactive phases, generation reset and recovery. Worker C++/WASM/distribution checks, seven new tests, six legacy tests, usage6 and skill18 now pass (signed handoff `7c0d68e7`). Localized widget source is now integrated (`49267cc4`); combined build and ten focused checks pass (49.25 seconds). Full WASM gallery linking now passes with warnings treated as errors (`0fdaad43`, Zig 0.16.0). An independent headless Chromium/WASM gate now passes keyboard edits, H12/Arabic locale, read-only, rejected bounds and validation reset. The full catalog run remains blocked by Dialog idle redraw; broader touch/IME/foreign acceptance remains open. Source one-root Tab behavior, IME and foreign-language bindings remain open. |
| SDL rendering | V1 baseline and pure source-range design are recorded. Private preflight source passed initial focused checks, including 216 frozen-validator comparisons; private extraction is integrated as `16827ec9`. Its exact-limit boundary fixtures and frozen-validator comparison now pass three combined headless checks (0.16 seconds); broader rendering regressions remain open. SDL V2 production rendering, compatibility measurements and failure validation remain open. Software V2 support does not imply SDL support. |
| Typed Slider | C/Python/JS/TS/Go/Rust authoring is integrated with focused checks. Rust current/legacy, 43 old/provider, four compile-fail and matching default-static UI/example checks pass; minimum-compiler acceptance remains open. Python/JS/TS/Go copied-wire evidence checks passed focused acceptance; the broader inactive-field matrix and native numeric delivery remain unfinished. Unreleased. |
| Backgrounds, shadows and text | Public C software V2 query/render is integrated with 20 focused headless/freshness checks; it is unreleased. Shared CPU helpers passed focused acceptance (including a corrected include-order fixture); owned-frame extraction (integrated; 24 headless checks and actual WASM layout compilation pass), the other four adapters and foreign authoring APIs remain open. V2 transforms/owned text support up to 64×; the existing scaled-font API retains its separate 16× limit. |
| Validation | Current combined Linux build and all **541/541 headless tests pass, zero skips**, in 291.67 seconds. The three consumer fixture closure fixes are verified; real font and pinned theme tests now run. Worker drafts, browser/platform acceptance and publication remain open. Evidence: integration commit `21acfca8` and `docs/validation/beta-combined-native-541.json` on the integration branch. |

## Immediate priorities

| Priority | Deliverable | Completion condition |
| --- | --- | --- |
| 1 | Validate active component drafts | Combined Linux build and 541 headless tests pass. Dialog now passes eight combined focused checks; run the prepared Dialog and TimeField gallery scenarios. TimeField also passes ten combined focused checks; The 541-test baseline predates both changes. |
| 2 | Finish active component slices | Finish first-frame Dialog/AlertDialog and localized TimeField gallery runtime acceptance; validate the private SDL preflight boundary tests. Table touch is integrated; broader lifecycle, native Slider and minimum-compiler gates remain open. |
| 3 | Complete advertised rendering | Extend V2 support beyond software, then reconcile component and foreign-language styling coverage. A shared helper alone is not adapter support. |
| 4 | Consolidate and publish | Run final integrated checks, reconcile the 78-family ledger, and publish matching source, packages and website artifacts. |

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
| [ ] | Responsive layout and speed | Verify continuous live resize and hover responsiveness on Linux/macOS; finish intrinsic sizing, overflow and current-frame caret/selection/IME projection. Keep APIs simple and document composed-app usage. |
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
| [ ] | CI, performance and resources | Reduce redundant builds/gate cost without weakening coverage; measure performance/memory. Consumer test harnesses request one compiler job. Rust’s embedded CMake build now follows Cargo NUM_JOBS with a conservative fallback of one; three offline job-control tests pass. Portable hosted CI now allows two test processes and bounded nested builds; savings remain unmeasured. Keep laptop load and artifacts bounded. |

### Component gaps to close

| Families / area | Outstanding acceptance scope |
| --- | --- |
| Controls | Button state/style refinement, native numeric Slider delivery and five builders (typed C Base/Styled models and controlled gallery are integrated but unreleased); consistent size, text alignment, icons, rounding and shadows across controls. |
| Date/time/color | Remaining parsing, formatting, bounds, keyboard, locale and accessibility contracts. Preserve existing locale/year navigation. |
| Overlays | Accept the integrated Dialog six-slot composition in the browser and close held-input reopening gaps; finish Popover focus/relationships/placement and HoverCard lifecycle/dismissal. Explicit collapsed semantics require a reviewed contract. |
| Tables | Table selection/sorting/resizing/themed cells/keyboard; DataTable virtualization, remote callbacks, semantics and consistent scrolling. |
| Charts/plots | Scales, axes/ticks, series/legend, layout/motion, tooltips, selection and accessibility; verify existing pan/zoom/touch. |
| App structure | Carousel intrinsic layout, Sidebar clipping/scrolling, Settings, Command lifecycle/width and Root/window services; verify composed apps. |
| Text/editing | TextView parser/plugins/selection/typography, Textarea behavior and Questionnaire model/navigation/semantics. Editor expansion remains lower priority. |
| Video/audio/speech | Verify reusable VideoPlayer controls, visible seek icons, video-only fullscreen and audio on advertised hosts; finish native provider/lifecycle gaps. Browser decoding proves no native codec/device support. |
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

Current integration evidence (not yet on `main`):

| Slice | Evidence | Still required |
| --- | --- | --- |
| Combined Linux baseline | `21acfca8`: 541/541 headless tests, zero skips | Repeat on the final integrated revision; this run predates Dialog and localized TimeField. |
| Dialog composition | `8cffe75e`: combined build and eight focused checks pass | Prepared first-frame, long-body and nested-touch browser checks. |
| Localized TimeField | Integrated `49267cc4`: combined build and ten focused checks pass; worker C++/WASM and legacy evidence retained | Full WASM build and focused browser checks pass; broader coverage and full catalog gate remain open. |

Integration-only receipts must travel with their source changes; worker evidence
must not be presented as validation of `main`.

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
