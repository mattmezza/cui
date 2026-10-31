# CUI beta roadmap

Updated: **2026-10-31**. Status: **working alpha; full parity is unfinished**.

**Goal:** finish and verify the frozen GPUI Kit 0.7.1 scope across all **78 catalog
families**, together with the requested native, binding, documentation and website
work. A beta label must not conceal missing scope. No completion percentage or
release date is asserted.

## At a glance

| State | What it means today |
| --- | --- |
| Live | Interactive C → WebAssembly gallery, all 78 catalog entries, browsable docs and API index. Impeccable artifacts removed. |
| Verified deployment | Website revision `ab8fddbbbca4b78e3b2cc1b14752dd0c4ac96b30`; public docs, catalog, headers and native archive checked on 2026-10-31. |
| Pending integration/release | Spinner browser improvements; Stepper visual builders and C presentation; HoverCard outside-release policy; initially hidden SDL3/bridge windows across five language paths; named-element C shadow setters, six typed builder roots, and opt-in Checkbox/Radio indicator IDs. Focused checks exist; combined validation/publication is still pending. |
| In progress | Shadow component-part and language-binding coverage; completion of remaining component and cross-platform gaps. |
| Deprioritized | Editor expansion/polish and color-picker polish. Existing regressions still need fixes. |

**Status rule:** implemented ≠ tested ≠ merged ≠ released ≠ deployed. A tested
feature or gallery example does not verify an entire family. Older ledger rows
can lag implementation: inspect current code and evidence before scheduling work.

## Next deliveries

- [ ] **Publish the pending component batch.** Finish combined browser/native and
  binding checks; merge reviewed commits; deploy and verify the actual revision.
  Preserve existing callbacks, defaults, Retina fixes and static-linking support.
- [ ] **Initially hidden windows.** Ship creation with the native hidden flag,
  avoiding show-then-hide flashing; document explicit show/hide and daemon usage.
  SDL3 and Python/JS/TS/Go/Rust bridge paths have focused Linux private-Xvfb
  evidence for actual hidden creation and first presentation. Integration,
  publication and broader native-platform acceptance remain pending. Caller-owned
  window hosts need their own policy.
- [ ] **Consistent shadows.** Ship a clear typed API for presets and custom
  offset/blur/spread/color, mixed drop/inset layers and clearing. Existing shadow
  renderers must be reused. Initial named-box C setters have focused tests; full
  component coverage is still pending. The [78-family ownership audit](docs/design/ELEMENT_SHADOW_COVERAGE.md)
  records public, private, anonymous and nonvisual targets. Finish component-root,
  nested-part and popup ownership, language bindings, copyable examples and
  browser/native proof. CSS-like blur
  convenience must not reinterpret existing sigma-based APIs or presets.
  The six-kind owned bridge/facade override passes focused portable software and
  all five facade checks. Eight C-only Checkbox/Radio/group facades now accept
  explicit indicator IDs while preserving legacy anonymous routes; their focused
  tests pass against captured old-library fixtures. Broader part/portal coverage,
  corresponding indicator bindings, browser/GPU proof and integration remain pending.
- [ ] **Release the completed major batch.** Freeze one source revision; validate
  matching C, Python/npm, Go and Rust artifacts; publish release notes/checksums
  and matching website docs. `0.1.0-alpha.3` is prepared, **not yet released**.

## Latest validation evidence

- Combined production source `6e75b603`: warning-enabled focused Release build and
  **10/10** headless C/Python tests passed. JS/Go/Rust runtime and actual older-library
  compatibility checks passed **5/5**; TypeScript strict checks passed.
- **18/18** consumer-skill tests, formatting and generated SDK/resource freshness
  passed; workflow regressions passed **9/9** (bindings) and **5/5** (Pages).
- Earlier gallery source `c52ce72f` passed headless HoverCard, Stepper and Spinner
  probes. The combined candidate passed all three hosted Pages browser shards at
  `b9c2f3d1`; final sanitizer/Windows checks remain pending after test-fixture fixes.
- Hosted native binding gate `b9c2f3d1` passed ten static/dynamic callback cases
  and hidden-window checks in all five language paths on the combined candidate.
- Earlier guide/build/artifact invocation errors are corrected. GCC sanitizer CI
  caught temporary bounds storage in the new indicator test; its lifetime is now
  fixed, and a focused instrumented Core/test reproduction plus ordinary regression
  pass locally. Windows also exposed oversized shadow-test stack workspaces;
  bounded heap oracles and a passing 1 MB stack regression address that pressure,
  pending Windows confirmation. All three Pages browser shards passed at `b9c2f3d1`.
  Final sanitizer/Windows gates, release and deployment remain pending; results
  prove only their stated subsets.

## Component parity: verify and close the gaps

This is a work list, not a declaration that everything below is absent. Some
features already have partial implementations or tests. The linked guides and
[feature ledger](docs/gpui-parity.json) retain the detailed scope and exceptions.

| Area | Work still required |
| --- | --- |
| Controls | Finish Button state/style refinement; remaining Slider/theme behavior; consistent sizes, typography, rounding, shadows and state styling across controls. |
| Date/time and color | Complete remaining parsing, formatting, bounds, keyboard, localization and accessibility contracts for DatePicker/TimeField/ColorPicker. Preserve existing locale/year navigation and civil-time work. |
| Overlays | Finish Dialog slots, Popover focus/relationship and remaining source placement behavior; verify HoverCard lifecycle/dismissal. Native explicit collapsed state needs a separate reviewed semantic-presence contract. |
| Tables | Complete Table selection/sorting/resizing, themed cells and keyboard behavior; DataTable virtualization/remote callbacks and header/footer/table semantics. Preserve consistent scrolling and existing styling. |
| Charts and plots | Close remaining scales, axes/ticks, series/legend, source layout/motion, tooltips, selection and accessibility gaps. Verify existing pan/zoom/touch behavior rather than reimplementing it blindly. |
| App structure | Complete Carousel intrinsic layout/host behavior, Sidebar, Settings, Command host-action lifecycle, and Root View/window services. Verify composed-app resize, scrolling and interaction behavior. |
| Rich text/editing | Complete TextView parser/plugins/shared selection/typography and Textarea source behavior. Audit Questionnaire model/navigation/compound semantics against landed work. Editor language services remain lower priority. |
| Speech and media | Finish documented native speech-provider gaps and host lifecycle; validate video/audio/player controls on each advertised host. Browser decoding is not proof of native codec/device support. |
| Remaining families | Audit **every other catalog family**, including those with all recorded feature rows marked tested, against frozen source behavior and styling. Close their documented limitations; do not infer completion from a green family badge. |

Every family needs applicable evidence for value/state ownership, disabled/inert/
loading behavior, pointer/keyboard/touch, focus/accessibility, visual states,
responsive bounds, omission/removal and resource lifetime. Conceptual host or
behavior families need justified exceptions rather than invented paint roots.

## Shared capabilities and developer experience

- [ ] **Responsive layout and performance:** close remaining live-resize and
  hover-latency issues with reproducible Linux/macOS evidence; verify narrow/wide
  composed apps, overflow and current-frame caret/selection/IME projection.
  Keep the responsive C API and language guidance clear.
- [ ] **Typography and design:** finish authoring/provider gaps for font families,
  numeric weights/stretch/variable axes, tracking, line height, wrapping and
  alignment. Preserve existing typography tokens and gradient text; document
  Canvas/GPU/binding limitations and complete the promised coverage.
- [ ] **Themes, motion and input:** complete native system preference/contrast/
  reduced-motion observation and component integration. Verify touch cancellation,
  nested scrolling, optional scrollbar styles, pan/pinch opt-ins, cursors and
  focus/hover/pressed states across applicable components.
- [ ] **Events and asynchronous UX:** finish the coverage/exception audit for the
  shared event contract, legacy timing/mutation compatibility and binding support.
  Validate the worker-fetch example's cancellation, stale responses, wakeup,
  failure/retry and shutdown with deterministic offline tests. Existing event
  facades/examples must be assessed before adding replacements.
- [ ] **Native accessibility:** complete optional-state representation and adapter
  mapping where missing; validate supported platform bridges with real assistive
  technology. Semantic snapshots alone do not establish screen-reader acceptance.
- [ ] **Platforms/renderers:** maintain Linux, macOS, Windows and browser support
  with explicit compile/runtime/physical-device evidence. Verify DPI/Retina,
  clipping, shadows, text, lifetime/recovery and static/relocated consumers.
  Unsupported hardware checks stay explicitly deferred; do not advertise untested
  renderers or additional operating systems as complete.

## Bindings, packages and public documentation

- [ ] **Bindings:** finish promised component/style/event APIs in Python, JS/TS,
  Go and Rust. Keep supported APIs and omissions explicit; test real UIs and
  runtime behavior, not just `.run()` or successful compilation.
- [ ] **Package-manager delivery:** verify clean installed consumers and native
  dependencies; finish PyPI/npm/crates.io ownership, publisher/authentication and
  publication gates, plus the correctly prefixed public Go module tag. Local
  archives and a GitHub release do not establish registry availability.
- [ ] **Docs and website:** provide matching quickstarts, dependencies/build/linker
  flags, callbacks/state ownership, immediate/retained distinctions, language
  examples and professional composed-app recipes. Keep search, copyable snippets,
  themes, media/README capture and sharing/OG metadata accurate as APIs change.
- [ ] **Agent guidance:** update source metadata, generated consumer skill and
  agent-readable docs together; keep discovery concise and revision-aware.
  Distinguish shipped declarations from proposals/pseudocode.

## Completion and release checks

- [ ] Reconcile all 78 families against the frozen baseline, current limitations
  and binding/platform evidence. Record justified exceptions; leave no required
  behavior silently omitted.
- [ ] Pass warnings, focused meaningful regressions, legacy compatibility,
  generated-resource freshness and applicable repository/CI checks. Add failure,
  capacity, lifetime and teardown coverage where behavior requires it.
- [ ] Run native Linux automation only through
  `python scripts/test_local.py --build-dir build`; browsers headless. Physical
  GPU/OS/assistive-technology acceptance remains separately identified.
- [ ] Record reproducible performance/memory results and material limitations;
  avoid claiming broad speedups from isolated timings.
- [ ] Validate exact release artifacts, provenance, checksums, installed examples
  and website revision before declaring delivery complete.

## Keeping this roadmap current

For every completed slice or newly discovered gap:

1. Update the matching checklist/table row and detailed source ledger/docs.
2. Attach concise evidence (test, commit, release or deployment); only check an
   item when its **whole stated scope** is complete.
3. Regenerate consumer resources when applicable and preserve all 78 entries.
4. Update the date and deployment/version facts when they actually change.

Maintainers/agents must preserve scope, keep worktrees/artifacts bounded, use
low-priority single-job builds while the maintainer works, and retain small
validation receipts instead of accumulating duplicate build trees.

Authoritative detail: [component ledger](docs/gpui-parity.json),
[binding coverage](docs/binding-coverage.json),
[platform matrix](docs/PLATFORM_MATRIX.md),
[release gates](docs/RELEASING.md), [v1 readiness](docs/V1_READINESS.md).
Incremental repainting remains a separate deferred exploration
([issue #1](https://github.com/mattmezza/cui/issues/1)); it is not a new beta blocker.
