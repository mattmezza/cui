# CUI beta roadmap

Updated: **2026-10-31** · Status: **working alpha; beta scope unfinished**.

**Goal:** complete and verify all **78 frozen component families**, plus the
requested native, language-binding, documentation and website capabilities.
No completion percentage or beta date is asserted.

## Current delivery status

| Stage | Status |
| --- | --- |
| Released library | [v0.1.0-alpha.3](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.3). Ten verified assets; hidden windows, initial named shadows and component improvements included. |
| Public website | Interactive C → WASM gallery with all 78 entries, docs and API index. Last independently verified deployment: `c8e222bb50f79ad6faa3195295bb6d4dc604e384`. |
| Merged development work | Additional component-part and Button state shadows are merged and deployed, but absent from alpha.3 library packages. |
| Work in integration | Current-frame scrollbar layout, a typed C-only Slider model, versioned background transport, private software preparation/validation and counted text recording. Focused checks exist; renderer painting, binding coverage and combined validation remain unfinished. |
| Package registries | Go alpha.3 tag/proxy verified. PyPI, npm and crates.io publication remains open; GitHub assets are available. |

**Implemented ≠ tested ≠ merged ≠ released ≠ deployed.** A gallery entry or green
feature test does not establish complete component parity.

## Immediate priorities

| Priority | Work | Next acceptance checkpoint |
| --- | --- | --- |
| 1 | Finish scroll layout and Dialog sizing | Core geometry integrated; 15 focused regressions and two Inter-font checks pass. Finish Dialog composition/input and wider acceptance. |
| 1 | Finish Slider behavior | C-only typed Base Single/Range interaction passes all four combined source/distribution/legacy checks. Finish Styled presentation, host delivery and bindings. |
| 1 | Finish backgrounds and gradients | Core transport, bounded preparation and private software validation are integrated; implement painting in software, SDL_Renderer, SDL_GPU, Sokol and Canvas support. Core-only tests establish no renderer support. |
| 1 | Resolve current validation failures | Fix hosted source-package configuration, Windows alignment and WASM32 warning failures; Spinner scheduling now passes its hosted check; obtain a clean full run. |
| 2 | Close component and binding gaps | Reconcile every family with frozen source behavior and explicit exceptions. |
| 3 | Deliver the next verified release | Matching packages, docs, installed examples and exact-revision website checks. |

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
| [ ] | CI, performance and resources | Reduce redundant builds/gate cost without weakening coverage; measure performance/memory. Package tests now default to one compiler job. Keep laptop load and build artifacts bounded. |

### Component gaps to close

| Families / area | Outstanding acceptance scope |
| --- | --- |
| Controls | Button state/style refinement, Styled Slider ranges/themes, native numeric delivery and five builders (the typed C Base model is integrated); consistent size, text alignment, icons, rounding and shadows across controls. |
| Date/time/color | Remaining parsing, formatting, bounds, keyboard, locale and accessibility contracts. Preserve existing locale/year navigation. |
| Overlays | Dialog independent header/title slots and separate Content/Body flex sizing; Popover focus/relationships/placement and HoverCard lifecycle/dismissal. Explicit collapsed semantics require a reviewed contract. |
| Tables | Table selection/sorting/resizing/themed cells/keyboard; DataTable virtualization, remote callbacks, semantics and consistent scrolling. |
| Charts/plots | Scales, axes/ticks, series/legend, layout/motion, tooltips, selection and accessibility; verify existing pan/zoom/touch. |
| App structure | Carousel intrinsic layout, Sidebar clipping/scrolling, Settings, Command lifecycle/width and Root/window services; verify composed apps. |
| Text/editing | TextView parser/plugins/selection/typography, Textarea behavior and Questionnaire model/navigation/semantics. Editor expansion remains lower priority. |
| Video/audio/speech | Verify reusable VideoPlayer controls, visible seek icons, video-only fullscreen and audio on advertised hosts; finish native provider/lifecycle gaps. Browser decoding proves no native codec/device support. |
| Every other family | Audit current code, frozen behavior and limitations even when recorded feature rows are green. Fix outstanding reported demo regressions; do not infer whole-family completion. |

For each family, verify applicable state ownership, callbacks/results,
keyboard/pointer/touch, focus/accessibility, visual states, responsive bounds,
omission/removal and resource lifetime. Record justified nonvisual/host exceptions.

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

Latest reliable delivery evidence: [CI](https://github.com/mattmezza/cui/actions/runs/41236071825),
[native bindings](https://github.com/mattmezza/cui/actions/runs/41236071845) and
[browser/deployment](https://github.com/mattmezza/cui/actions/runs/41236071976)
passed for the merged styling revision. New integration work has focused passes, including 18/18 combined skill tests,
6/6 usage tests and generated freshness after background preparation integration
(`2cfe490a`). Fresh strict C11/headless background checks passed 107 grids and
8,500 frozen-sampler comparisons; this establishes preparation, not renderer support.
Typed Base Slider `5afc739f` passed a warnings-clean combined build and 4/4
headless source, selective/umbrella distribution and legacy-event checks. Updated
consumer skill tests passed 18/18. Styled and foreign authoring remain pending.

Candidate `b2a4db93` passed [full hosted CI](https://github.com/mattmezza/cui/actions/runs/41244554704)
and [native bindings](https://github.com/mattmezza/cui/actions/runs/41244554789).
Newer Core integration `6fb1357d` passed a 157-step warnings-clean build, all 15
focused layout/background/legacy-pixel tests and two Inter-font checks. Private
software validation `0625580a` also passed against that rebuilt Core. The combined
consumer skill passed 18/18 and pinned formatting checks passed. Painting and
font/shadow resource preparation remain unfinished. Spinner fix `638d9d13` passed
four local viewport/DPI cases and 24 instrumented preference transitions with zero
extra frames; hosted confirmation remains pending. No clean **full**
regression run or browser gate covers the complete newer integration yet.
Older green results do not cover newer code.

Current candidate `c66c6f5f` exposed three hosted integration gaps: source-package
configuration incorrectly required generated `dist` headers; Windows lacks the
private validator’s `max_align_t`; a Slider event-count guard warns on WASM32.
The package configuration fix passes actual staged-source configuration and all
five full-tree import fixtures locally. Portability fixes and hosted confirmation
remain pending; there is no new public deployment.

### Current integration evidence

These are **unreleased slices**, not completed beta areas:

| Slice | Evidence / remaining gate |
| --- | --- |
| Scrollbar layout | Focused geometry, layout, resize and input-policy checks pass; Dialog integration remains open. |
| Typed Base Slider | Source/selective/umbrella and legacy tests pass (4/4). C-only support; styled component and other languages remain open. |
| Reduced-motion scheduling | Four local viewport/DPI cases and repeated preference changes pass; the named hosted check also passed in Pages run `41247866200`. Whole workflow remains failed for WASM32 compilation. |
| Background rendering | Transport, preparation and private validator checks pass. Counted text recording is integrated locally; Release O3 integration exposed a negative-fixture warning and a no-Bidi contextual admission gap. Corrections and runtime confirmation remain open. This does **not** establish renderer painting support. |
| Portability fixes | Combined local build and five runtime checks pass; pinned WASM32 compilation, skill18 and generated freshness pass. New hosted Linux/macOS/Windows/browser confirmation remains open. |

Keep detailed receipts with their implementation changes; refresh this summary
when the integrated revision passes, merges or ships.

## Deferred or lower priority

- **Incremental repainting:** separate exploration in [issue #1](https://github.com/mattmezza/cui/issues/1), not an added beta blocker.
- **Editor expansion and ColorPicker polish:** lower priority; existing regressions still require fixes.
- **v1.0:** follows beta acceptance; see [v1 readiness](docs/V1_READINESS.md).

## Maintenance rule for humans and agents

Update this document **in the same change** that completes a slice or discovers a
new gap. Update the corresponding detailed ledger/guide too. Check a row only when
its entire scope is complete; attach concise test/commit/release evidence and
retain unresolved exceptions. Refresh the date and delivery facts only when they
change. Keep all 78 manifest entries and preserve independently landed main work.

Detailed references: [component ledger](docs/gpui-parity.json) ·
[binding coverage](docs/binding-coverage.json) ·
[platform matrix](docs/PLATFORM_MATRIX.md) ·
[release gates](docs/RELEASING.md) · [v1 readiness](docs/V1_READINESS.md).
