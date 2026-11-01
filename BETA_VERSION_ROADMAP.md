# CUI beta roadmap

Updated: **2026-11-01** · Status: **working alpha; beta scope unfinished**.

**Goal:** complete and verify all **78 frozen component families**, plus the
requested native, language-binding, documentation and website capabilities.
No completion percentage or beta date is asserted.

## At a glance

| Track | Current status |
| --- | --- |
| Public release | **v0.1.0-alpha.3**; [release assets](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.3) available. |
| Component coverage | **78 families: 77 tested subsets, 1 implemented, 0 fully verified.** These counts are not a completion percentage. |
| Public website | Interactive WASM gallery, docs and API index. Last independently verified deployment: `c8e222bb50f79ad6faa3195295bb6d4dc604e384`. |
| Integration branch | New Slider, scroll geometry, background/text preparation and private software rendering work is **unreleased**. Combined validation is unfinished. |
| Packages | Go alpha.3 tag/proxy verified; PyPI, npm and crates.io publication remains open. |

**Implemented ≠ tested ≠ merged ≠ released ≠ deployed.** A gallery entry or green
feature test does not establish complete component parity.

## Next milestones, in order

1. **Finish active implementation:** Dialog sizing/lifecycle, typed Slider bridge
   and language builders, then complete background/shadow/text rendering support.
2. **Close component and ecosystem gaps:** reconcile every family with the frozen
   baseline and the reported regressions below; finish bindings and documentation.
3. **Validate the integrated revision:** compatibility, headless regressions,
   platform evidence, generated files and exact-revision CI/browser checks.
4. **Ship matching artifacts:** library/packages, website and reproducible examples.

### Active work and known blockers

| Work | Landed evidence / next requirement |
| --- | --- |
| Dialog and scrolling | Current-frame geometry, frozen compatibility and small-viewport proofs pass. Nested Dialog close/reopen can still retain an old routed touch scroll; finish and verify lifecycle cancellation before integration. |
| Typed Slider | C Base/Styled models and controlled gallery are integrated. Full browser regression, 42 representative touch checks and 19 mobile navigation checks pass locally. C bridge is under review, including failed-frame authority and metadata; five language builders and native numeric delivery remain open. |
| Backgrounds, shadows and text | Transport, counted text preparation and private ordinary software painter are integrated. Private shadow profiles pass frozen comparisons. Transactional private shadow painting is integrated; worker checks pass (3/3, 48 mixed frames plus legacy comparisons). Combined-source acceptance remains open. Text painting and public support in software, SDL_Renderer, SDL_GPU, Sokol and Canvas remain open. |
| Validation | Candidate `00aa1f99` passed hosted CI/native bindings; Pages still needs terminal confirmation. Those results do not cover newer integration work. A fresh combined build/test gate remains required. |

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
| [ ] | CI, performance and resources | Reduce redundant builds/gate cost without weakening coverage; measure performance/memory. Package tests default to one local compiler job. Portable hosted CI now allows two test processes and bounded nested builds; savings remain unmeasured. Keep laptop load and artifacts bounded. |

### Component gaps to close

| Families / area | Outstanding acceptance scope |
| --- | --- |
| Controls | Button state/style refinement, native numeric Slider delivery and five builders (typed C Base/Styled models and controlled gallery are integrated but unreleased); consistent size, text alignment, icons, rounding and shadows across controls. |
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

Detailed validation receipts live in [docs/validation](docs/validation).
Older green runs do not cover newer source; worker checks do not establish
integrated, released or deployed support.

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
