# Changelog

## Unreleased

- C RadioGroup can crossfade selection marks with the theme spring while
  updating selection and accessibility in the same frame. The WASM example
  shows the animation with reduced-motion support.

## 0.1.0-alpha.7

- Pagination wraps its controls at the available width and keeps the page
  counter on one centered line. It supports shared size presets, fractional
  custom heights, configurable page budgets, localized arrow hints, and
  ellipsis jump menus. Retained C, Python, JS/TS, Go and Rust expose the
  extended configuration through the existing page-change event.
- Responsive Notification cards wrap titles and messages and reserve a
  separate Close target. Persistent notices support Tab and touch dismissal,
  and a long notice can place its action on a separate row. The new presenter
  is available in C and the gallery.
- C Attachment cards have an opt-in presenter for independently styled
  filenames and descriptions. Long text ellipsizes at grapheme boundaries
  while accessibility retains the full name. The gallery shows the same
  behavior with images and independent actions.
- Sidebar reveals newly focused rows within its scrollable body. The header
  and footer stay pinned, and the body reserves the configured scrollbar
  width only when a visible bar can overflow.
- Release qualification runs on branch pushes. Library and Go version tags
  reference the already-qualified commit without scheduling duplicate builds.

All 78 catalog families have tested subsets. Full-family verification,
physical device checks and registry publication remain open.

## 0.1.0-alpha.6

- The localized TimeField offers an opt-in single Tab stop with internal
  segment navigation, IME composition and size and appearance options in C.
  Chromium and the Linux accessibility adapter expose the committed value and
  the active segment. The language packages retain their localized builder
  APIs and include reproducible examples. Ordered IME stays a direct C API.
- Growing Input and InputGroup controls place the caret, selection and IME
  anchor using the current frame's width during live resize. Textarea keeps
  carets at wrapped line edges visible and supports Alt+Arrow word navigation.
- Line charts render continuous strokes across display scales and can show
  circular points at a logical size. Plot retains the full initial touch
  displacement and can hand a declined vertical drag to the browser page.
- Carousel arrows and pagination inherit the viewport's disabled state.
  Calendar and DatePicker keep queued keyboard navigation across year pages
  and require a fresh activation to choose the month.

All 78 catalog families have tested subsets. Full-family verification, physical
platform checks and publication to PyPI, npm and crates.io remain open.

## 0.1.0-alpha.5 — 2026-11-25

- DataTable keeps the visible row and selection across keyed updates. C, Python,
  JavaScript, TypeScript, Go and Rust support opt-in near-end paging and copied
  snapshots of up to 10,000 rows within a 32 MiB native app budget. Ordinary
  setters accept 32 rows. Applications own page transport and request lifetime.
- Retained Icon and Badge builders share owned SVG artwork across all five
  language bindings. Configured Badges preserve the actions of their children.
  Responsive Skeleton builders include runnable loading-to-content examples.
- The native starter switches between pinned Sidebar and compact Sheet
  navigation as the window resizes. Projects combines resizable panes and
  bounded tabs. Short Settings pages stay accessible through scrolling and
  keyboard focus.
- VideoPlayer adds mute and volume controls with host confirmation. Its browser
  example uses decoded audio and player-only fullscreen. The optional native
  example uses FFV1/PCM with SDL audio gain.
- Scroll input follows the same wheel step in DataTable and Scrollable.
  Bounded horizontal tabs accept Shift-wheel on SDL. The gallery includes
  anchored updates in DataTable and variable-height VirtualList.
- Popover adds explicit trigger linking and optional right-click toggling.
  HoverCard follows intrinsic content changes. Composed Dialog discards held
  input across reopen and Command fills its panel on the first frame.
- Core indexes box IDs and semantic ancestry and reuses clipped hit results
  during hover queries. The Linux C adapter adds opt-in AT-SPI numeric Slider
  actions. Broader assistive-technology and hardware acceptance remains open.

All 78 baseline families remain in the gallery. Remote data adapters, full
component parity, native platform acceptance and registry delivery remain open.

## 0.1.0-alpha.4 — 2026-11-01

Published GitHub prerelease with matching website and native starter assets. Alpha.3
artifacts remain immutable. Evidence and remaining platform/registry gates are tracked in
[the release checklist](docs/RELEASING.md) and [beta roadmap](BETA_VERSION_ROADMAP.md).

- Adds controlled typed Slider APIs and authoring support in Python, JS/TS, Go
  and Rust, with compatibility, lifetime, malformed-record and copied-value checks.
  Native numeric accessibility delivery remains incomplete.
- Adds a separate locale-aware TimeField widget with typed configuration,
  parsing/formatting, bounds and C/Python/JS/TS/Go/Rust authoring. Legacy scalar
  fields remain available. Focused current/old/partial-provider, dynamic/static
  and browser checks pass; broader native IME/platform acceptance remains open.
- Adds six-slot Dialog composition, intrinsic initial sizing, fixed-footer body
  scrolling and idle quiescence, with desktop/touch gallery coverage. Broader
  lifecycle policies and foreign composition facades remain open.
- Adds configured C Badge artwork using ordinary Icon state, style constraints
  and explicit ownership; the gallery includes real artwork and child activation.
  Configured Badge language authoring remains pending.
- Adds opt-in public C software V2 rendering, bounded preparation and owned-frame
  lifetimes for backgrounds, shadows and styled text; SDL_Renderer V2 consumes
  the same checked representation. SDL_GPU, Sokol and equivalent foreign paint
  authoring are not established by this change.
- Adds owned Skeleton theme palettes and explicit preset/custom colors, a fixed
  painter, and own-root full/fraction/flex presentation with source-ordered style
  refinement. Legacy phase/easing behavior is preserved; modern binding options
  and gallery exposure remain separate gaps.
- Adds app-owned C bridge Icon resources with copied SVG/maps/bitmap utilities,
  bounded ownership and source quotas. These utilities do not yet provide
  configured Icon/Badge node rendering or foreign resource facades.
- Fixes movable Table header touch-drop admission without changing the legacy
  mouse path. Representative nested scrolling and touch regressions are covered.
- Preserves timestamps for unchanged generated SDK/distribution/skill outputs and
  bounds compiler concurrency. Rebuild/hosted-CI speedups remain unmeasured.
- Adds a maintained beta roadmap and updated API, human and agent guidance.

All 78 baseline families remain present. This release does not establish full
component parity, physical GPU/OS/assistive-technology acceptance or publication
to PyPI/npm/crates.io. Use matching headers/libraries and consult binding omissions.

## 0.1.0-alpha.3 — 2026-10-31

- Adds opt-in Stepper indicators/connectors, bitmap icons and shared xs–9xl
  sizing, with controlled navigation and copied gallery examples. Centered and
  responsive composition, exact source styling and language builders remain gaps.
- Adds a nested Resizable explorer/editor/console example with independent
  handles and copied C usage; repeated desktop/touch drag regressions are covered.

- Constrains populated grow-width Input viewports during parent resizing,
  including InputGroup prefix/suffix composition, while preserving clipping.
  Caret/selection scroll and IME projection can still lag one resize frame.

- Adds a separate civil DateTime picker with minute/second precision, 12/24-hour
  display, explicit assignment, presets and clear actions; includes compiled C
  examples and real desktop/touch gallery controls. Timezones and date ranges are
  outside this API; existing date-only entry points remain available.
- Adds Questionnaire progress and named title/description/error presentation,
  token sizes and copied usage examples. Legacy presentation defaults remain
  available. Adds checked container/card metrics, masked box refinements and
  borrowed custom-child views; Input and general styling parity remain incomplete.
- Adds opt-in Popover anchor positioning, signed offsets and arrows, with
  desktop/touch gallery controls, software/WASI and private-Xvfb validation.
  SDL3 and Canvas consume bounded analytic meshes with pixel and recovery tests;
  exact reference rasterization, untested native drivers and positioning builders
  remain outside the verified scope.
- Adds independent DropdownButton paint refinements and explicit custom-shadow
  gates, with desktop/touch gallery controls and compilable usage. Null/zero
  descriptors retain the legacy path. New options are C-only. Adds separate semantic names, signed
  tab ordering and skip-tab controls for each half; optional pressed-button
  accessibility mapping and source theme-token parity remain incomplete.
- Adds opt-in date-only DatePicker presets, clear/Delete actions and an interactive
  desktop/touch gallery mode. Legacy date-only APIs remain available.
- Adds bounded Questionnaire copy-out snapshots, keyboard shortcut policies and
  persistent typed observation with explicit payload, nesting and lifetime rules.
- Adds Avatar metrics, identity palettes, SVG placeholders and typed activation
  observers, including ordered stack-child observation.
- Adds reusable VideoPlayer defaults, seek icons, player-only browser fullscreen
  and an audible browser sample.
- Adds an optional macOS Speech transport with injected Apple SDK tests; physical
  microphone/permission acceptance and Linux/Windows recognizers remain unverified.
- Adds an optional native FFmpeg/SDL video-audio example with a local bounded
  fixture; this does not imply general codec, device or platform acceptance.
- Fixes source-package configuration when documentation examples are absent,
  keeps C++ header checks optional for C-only builds, and checks DatePicker
  semantic values independently of C struct padding.
- Rebuild C consumers and bindings with matching headers and libraries:
  `cui_questionnaire_state` gained an observer pointer. Language builders do not
  expose the new C observer/payload arenas, Questionnaire presentation/geometry options,
  DateTime/DatePicker action options, explicit Popover positioning or split-button
  paint refinements.
- [GitHub release artifacts](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.3)
  and the public Go module tag/proxy are available. PyPI, npm and crates.io
  publication remain separate work. All 78 catalog families remain present; full
  feature parity is unfinished.

## 0.1.0-alpha.2 — 2026-10-20

- Adds implemented component event facades while retaining explicit typed and
  host-service exceptions; this does not imply uniform APIs or full binding parity.
- Fixes secure Styled InputGroup masking and owned IME composition lifetime, and
  same-frame IME caret ownership during Find-to-Editor focus transfer.
- Adds focused callback-contract and distribution compiler regressions.
- Adds call-local VirtualList final-offset and MessageScroller double-offset/tail
  observers, preserving legacy reduction and deferred-window semantics.
- Adds bounded Questionnaire box-slot styles and six translated labels; rebuild
  consumers for the extended by-value options structure.
- Makes human skill discovery concise, preserves full/JSON output and adds explicit
  partial Markdown section reads without clipping canonical limitations.
- Adds a real browser-decoded Video gallery extension with bounded WebM/MP4
  assets, CUI controls and explicit decoder/audio limitations; baseline remains 78.
- Published as GitHub release artifacts with validation receipts. PyPI, npm,
  crates.io publication and a public Go module tag remain separate work.
- Alpha.1 versions, tags and published assets remain immutable.

## 0.1.0-dev

Initial experimental C11/Clay foundation. No stable ABI or production-readiness commitment.

### Application-building and debugging

- Bounded first/latest diagnostics report operation, ID, frame, message, subsystem,
  capacity counters and open-box paths. Optional callbacks leave logging to the host.
- Opt-in text/JSON layout exports include box ancestry, requested sizing and resolved
  bounds/clips. Native bundle hosts expose `--debug-layout text|json`.
- Identity-bearing avatar/tag calls and relevant Core operations reject zero IDs
  explicitly; anonymous layout boxes remain supported. See `docs/IDS_AND_LIFETIMES.md`.
- `cui_text_ellipsis` resolves constrained single-line text in the current frame,
  preserves grapheme boundaries and keeps the full semantic label.
- `cui_icon_query` distinguishes unknown, nonembedded and embedded icons; an
  opt-in missing-icon outline preserves the original failure result.
- Native bundles support multiple application sources, custom targets, a two-file
  starter and a mail acceptance fixture. Guides cover layout, identity and debugging.
- Website documentation adds symptom search, matching public-header declarations,
  versioned agent exports and checksummed native source downloads.
