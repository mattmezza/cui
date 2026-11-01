# CUI beta roadmap

Updated: **2026-11-01** · Status: **working alpha; beta scope unfinished**.

**Goal:** complete and verify all **78 frozen component families**, plus the
requested native, language-binding, documentation and website capabilities.
No completion percentage or beta date is asserted.

## Current delivery status

| Stage | Status |
| --- | --- |
| Released library | [v0.1.0-alpha.3](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.3). Ten verified assets; hidden windows, initial named shadows and component improvements included. |
| Public website | Interactive C → WASM gallery with all 78 entries, docs and API index. Last independently verified deployment: `c8e222bb50f79ad6faa3195295bb6d4dc604e384`. |
| Merged development work | Additional component-part and Button state shadows are merged and deployed, but absent from alpha.3 library packages. |
| Work in integration | Current-frame scroll-region sizing, typed Base/Styled C Sliders, a controlled Slider gallery, versioned background transport, private software painting/validation and counted text recording. Focused checks pass; these slices are unreleased. Public renderer support, bindings and final combined validation remain unfinished. |
| Package registries | Go alpha.3 tag/proxy verified. PyPI, npm and crates.io publication remains open; GitHub assets are available. |

**Implemented ≠ tested ≠ merged ≠ released ≠ deployed.** A gallery entry or green
feature test does not establish complete component parity.

## Immediate priorities

| Priority | Work | Next acceptance checkpoint |
| --- | --- | --- |
| 1 | Finish scroll layout and Dialog sizing | Core geometry and additive region sizing integrated. Current-size placement and small-viewport overflow proofs pass. Finish Dialog composition/input and lifecycle acceptance. |
| 1 | Finish Slider behavior | Typed Base/Styled C interaction and presentation pass focused integrated checks; controlled gallery passes focused headless checks. Finish bridge, five language builders, native numeric delivery, vertical registered scrolling and wider acceptance. |
| 1 | Finish backgrounds and gradients | Core transport, bounded preparation and private software validation are integrated; implement painting in software, SDL_Renderer, SDL_GPU, Sokol and Canvas support. Core-only tests establish no renderer support. |
| 1 | Resolve current validation failures | Source-package, Windows alignment and WASM32 fixes are integrated. Candidate `00aa1f99` passes CI and native bindings, including Windows. Confirm Pages and run the newer integrated source through final checks. |
| 2 | Close component and binding gaps | Reconcile every family with frozen source behavior and explicit exceptions. |
| 3 | Deliver the next verified release | Matching packages, docs, installed examples and exact-revision website checks. |

## Remaining beta scope

Each unchecked row is an **unfinished scope**, potentially containing landed
subsets. Detailed ledgers and headers remain authoritative.

| Done | Area | Remaining work |
| --- | --- | --- |
| [ ] | Component parity | Audit all 78 families against the frozen baseline; complete missing behavior, styling, lifecycle and documented exceptions. See the family breakdown below. |
| [ ] | Responsive layout and speed | Verify continuous live resize and hover responsiveness on Linux/macOS; finish intrinsic sizing, overflow and current-frame caret/selection/IME projection. Keep APIs simple and document composed-app usage. |
| [ ] | Shadows and backgrounds | Complete root, nested-part and popup shadow ownership across components/bindings; finish gradients and renderer support, clipping, opacity, resource limits and failure behavior. Resolve unequal-scale shadow quality before enabling the proposed software path; preserve legacy rendering. |
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

The deployed styling revision passed [CI](https://github.com/mattmezza/cui/actions/runs/41236071825),
[native bindings](https://github.com/mattmezza/cui/actions/runs/41236071845) and
[Pages](https://github.com/mattmezza/cui/actions/runs/41236071976).
Integration candidate `3636bfbc` passed all browser shards and native binding
runners; its only CI failure was a Windows Release struct-padding comparison.
The fieldwise correction passes locally. Submitted candidate `00aa1f99` has successful
[CI](https://github.com/mattmezza/cui/actions/runs/41251863047),
[native bindings](https://github.com/mattmezza/cui/actions/runs/41251863050) runs. Its
[Pages](https://github.com/mattmezza/cui/actions/runs/41251862625) result still requires terminal
confirmation. Branch checks do not deploy the public website. Older green results
do not cover newer code.

### Current integration evidence

These are **unreleased slices**, not completed beta areas:

| Slice | Evidence / remaining gate |
| --- | --- |
| Scrollbar layout | Focused geometry, layout, resize and input-policy checks pass; Dialog integration remains open. |
| Typed Base/Styled Slider | Integrated source/distribution/legacy/example checks pass (16/16), alongside strict WASM compilation, usage6 and skill18. Owned presentation, motion and 176 software pixel proofs are covered; see [receipt](docs/validation/styled-slider-facade-integration.json). Bridge, five builders and native numeric delivery remain open. |
| Controlled Slider gallery | Integrated at `259e298a`; focused headless acceptance covers owner acceptance/rejection, Single/Range pairs, pixels, pointer/touch/keys and passive idle. Full headless browser regression also passes, including 80 compiling usage excerpts and all 78 baseline entries. Deployment and hosted checks of this newer revision remain open. |
| Reduced-motion scheduling | Local viewport/DPI and repeated preference-change checks pass; all browser shards passed for candidate `3636bfbc`. Confirm the newest candidate separately. |
| Background rendering | Transport, preparation and private validator checks pass. Counted text recording passes the repository Release O3 build, seven runtime checks, mutation rejection and skill18. The negative-fixture warning and no-Bidi admission gap are fixed. Wider suite/platform acceptance remains open. This does **not** establish renderer painting support. |
| Scroll regions and private painter | Both integrated. Focused private checks pass (10/10), alongside skill18. Large-coordinate regression reproduced and corrected. See [receipt](docs/validation/core-paint-integration.json). Private painting does not establish public adapter support. |
| Dialog sizing | Current-size placement, frozen compatibility and small-viewport proofs pass. Worker actual-font geometry/contracts pass; input testing found a late scrollbar-registration error after accepted close. Worker fix advances past close/veto/focus checks, but held-touch testing reproduces old-contact scrolling after reopen. Lifetime isolation needs a reviewed fix and regression coverage. No family closure claimed. |
| Native Slider delivery | [Source audit](docs/validation/native-slider-delivery-audit.json) confirms native adapter routes only focus/click and publishes legacy float values. Additive numeric mode, owned double payload/authority, host opt-ins and real AT delivery remain open. |
| Shadow quality | Test-only profile/footprint diagnostics preserve 512 legacy RGBA comparisons. Independent positive-blur area reference is integrated (`86aadbed`): worker and integrated strict/headless checks pass (1/1), 512 legacy RGBA comparisons remain exact; see [receipt](docs/validation/shadow-area-integration.json). Fractional inset/source-kernel differences persist; production geometry/rendering decisions remain open. |
| Portability fixes | Combined local build and five runtime checks pass; pinned WASM32 compilation, skill18 and generated freshness pass. Hosted CI/native bindings pass for `00aa1f99`; newer slices still require exact-revision hosted/browser confirmation. |

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
