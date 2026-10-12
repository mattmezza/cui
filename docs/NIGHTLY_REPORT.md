# Nightly report — live development checkpoint

Repository: https://github.com/mattmezza/cui. Branch `main`. Evidence/source commit before this report update: `47dfd82128af0914e94271667b23ec2694180f5c`. See swarm-status.md for branch promotion and push/CI observations.

## Working architecture

Explicit C11 contexts, pinned Clay layout, stable scoped IDs, bounded frame strings, ordered previous-frame hit routing, capture/focus/modal scopes, independent semantics and draw lists. Unstyled controlled primitives feed theme-driven components. UTF-8 scalar editing with bounded undo/redo, retained keyed Fenwick virtualization, split/tab docking with transactional persistence and plot scales/clipped geometry are substantive headless models. Optional FreeType/HarfBuzz shapes runs. Software, SDL_Renderer compatibility, SDL_GPU Vulkan and independent Sokol GLCORE are separate renderers; actual hardware/runtime selection is reported.

GPUI Kit baseline 0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`; Clay e6cc36941ab2af5d81107617039d6f527a1c660b. See upstream-baseline.md and DECISIONS.md. Source code and generated one/few headers are tested independently.

## Catalog counts

| Status | Families |
|---|---:|
| not_started | 18 |
| scaffolded | 0 |
| implemented | 4 |
| tested | 56 |
| verified | 0 |

Every non-verified family remains incomplete; tested means only its documented subset has tests. **No full family is verified.** Full feature backlog: [gpui-parity.json](gpui-parity.json), [COMPONENTS.md](COMPONENTS.md).

## Validation evidence

Source47dfd82 passed native49/49, ASan/UBSan47/47, WASI40/40 and Chromium58 real-story gates, including standalone Shimmer, Message/MessageScroller, exact Tab sizes and disclosure animation. Publishededea484 has57 stories with six CI jobs and Pages successful, confirmed by an actual HTTPS Notification click. This58-story checkpoint awaits deployment. All78 entries remain; full parity is unfinished.

Latest stored logs (including the preserved initial WASI failure and corrected rerun):

- [asan-56.txt](validation/asan-56.txt)
- [asan-57.txt](validation/asan-57.txt)
- [asan-58.txt](validation/asan-58.txt)
- [asan-form.txt](validation/asan-form.txt)
- [asan-measurement-input-group.txt](validation/asan-measurement-input-group.txt)
- [asan-otp.txt](validation/asan-otp.txt)
- [asan-surfaces.txt](validation/asan-surfaces.txt)
- [asan-table-dropdown.txt](validation/asan-table-dropdown.txt)
- [browser-39.txt](validation/browser-39.txt)
- [browser-42.txt](validation/browser-42.txt)
- [browser-43.txt](validation/browser-43.txt)
- [browser-46.txt](validation/browser-46.txt)
- [browser-51-integrated.txt](validation/browser-51-integrated.txt)
- [browser-51-measurement.txt](validation/browser-51-measurement.txt)
- [browser-51.txt](validation/browser-51.txt)
- [browser-53-integrated.txt](validation/browser-53-integrated.txt)
- [browser-56-integrated.txt](validation/browser-56-integrated.txt)
- [browser-57-integrated.txt](validation/browser-57-integrated.txt)
- [browser-58-integrated.txt](validation/browser-58-integrated.txt)
- [browser-pre-surfaces.txt](validation/browser-pre-surfaces.txt)
- [ci-01865d5.txt](validation/ci-01865d5.txt)
- [clang-zig-headless.txt](validation/clang-zig-headless.txt)
- [deployed-42.txt](validation/deployed-42.txt)
- [gcc-56.txt](validation/gcc-56.txt)
- [gcc-57.txt](validation/gcc-57.txt)
- [gcc-58.txt](validation/gcc-58.txt)
- [gcc-button-intents.txt](validation/gcc-button-intents.txt)
- [gcc-choice-wheel.txt](validation/gcc-choice-wheel.txt)
- [gcc-clip-lifecycle.txt](validation/gcc-clip-lifecycle.txt)
- [gcc-combo-table.txt](validation/gcc-combo-table.txt)
- [gcc-focus-proxy.txt](validation/gcc-focus-proxy.txt)
- [gcc-focus-within.txt](validation/gcc-focus-within.txt)
- [gcc-form.txt](validation/gcc-form.txt)
- [gcc-gallery46.txt](validation/gcc-gallery46.txt)
- [gcc-inert.txt](validation/gcc-inert.txt)
- [gcc-input-group.txt](validation/gcc-input-group.txt)
- [gcc-input-paint.txt](validation/gcc-input-paint.txt)
- [gcc-input-tree.txt](validation/gcc-input-tree.txt)
- [gcc-integrated-51.txt](validation/gcc-integrated-51.txt)
- [gcc-intent-theme.txt](validation/gcc-intent-theme.txt)
- [gcc-link.txt](validation/gcc-link.txt)
- [gcc-measurement.txt](validation/gcc-measurement.txt)
- [gcc-offset.txt](validation/gcc-offset.txt)
- [gcc-otp.txt](validation/gcc-otp.txt)
- [gcc-palette-caps.txt](validation/gcc-palette-caps.txt)
- [gcc-pickers.txt](validation/gcc-pickers.txt)
- [gcc-pointer.txt](validation/gcc-pointer.txt)
- [gcc-pressed.txt](validation/gcc-pressed.txt)
- [gcc-reference-controls.txt](validation/gcc-reference-controls.txt)
- [gcc-resumed.txt](validation/gcc-resumed.txt)
- [gcc-scroll-table.txt](validation/gcc-scroll-table.txt)
- [gcc-shapes.txt](validation/gcc-shapes.txt)
- [gcc-sheet-resize.txt](validation/gcc-sheet-resize.txt)
- [gcc-status.txt](validation/gcc-status.txt)
- [gcc-surface-footer.txt](validation/gcc-surface-footer.txt)
- [gcc-surface-scroll.txt](validation/gcc-surface-scroll.txt)
- [gcc-surface-tracking.txt](validation/gcc-surface-tracking.txt)
- [gcc-surfaces-choice.txt](validation/gcc-surfaces-choice.txt)
- [gcc-surfaces.txt](validation/gcc-surfaces.txt)
- [gcc-table-dropdown.txt](validation/gcc-table-dropdown.txt)
- [gcc-table-navigation.txt](validation/gcc-table-navigation.txt)
- [gcc-tabs-scrollbar.txt](validation/gcc-tabs-scrollbar.txt)
- [gcc-wheel.txt](validation/gcc-wheel.txt)
- [gcc-xorg.txt](validation/gcc-xorg.txt)
- [pages-daf43b9-failure.txt](validation/pages-daf43b9-failure.txt)
- [published-0e61b2f.txt](validation/published-0e61b2f.txt)
- [published-edea484.txt](validation/published-edea484.txt)
- [release-headless.txt](validation/release-headless.txt)
- [release-resumed.txt](validation/release-resumed.txt)
- [sanitizers-input-tree.txt](validation/sanitizers-input-tree.txt)
- [sanitizers-pickers.txt](validation/sanitizers-pickers.txt)
- [sanitizers-resumed.txt](validation/sanitizers-resumed.txt)
- [sanitizers-scroll-table.txt](validation/sanitizers-scroll-table.txt)
- [sanitizers.txt](validation/sanitizers.txt)
- [sokol-asan-xorg.txt](validation/sokol-asan-xorg.txt)
- [wasi-56-fuel-limit.txt](validation/wasi-56-fuel-limit.txt)
- [wasi-56.txt](validation/wasi-56.txt)
- [wasi-57.txt](validation/wasi-57.txt)
- [wasi-58.txt](validation/wasi-58.txt)
- [wasi-form.txt](validation/wasi-form.txt)
- [wasi-initial.txt](validation/wasi-initial.txt)
- [wasi-input-tree.txt](validation/wasi-input-tree.txt)
- [wasi-measurement-input-group.txt](validation/wasi-measurement-input-group.txt)
- [wasi-otp.txt](validation/wasi-otp.txt)
- [wasi-pickers.txt](validation/wasi-pickers.txt)
- [wasi-resumed-initial.txt](validation/wasi-resumed-initial.txt)
- [wasi-resumed.txt](validation/wasi-resumed.txt)
- [wasi-scroll-table.txt](validation/wasi-scroll-table.txt)
- [wasi-surfaces.txt](validation/wasi-surfaces.txt)
- [wasi-table-dropdown.txt](validation/wasi-table-dropdown.txt)
- [wasi.txt](validation/wasi.txt)

Commands: `cmake -S . -B build -G Ninja -DCUI_GPU_TESTS=ON`; `cmake --build build`; `ctest --test-dir build --output-on-failure`. ASan/UBSan config uses `-DCUI_SANITIZE=ON`; portable Clang 21.1.8 is supplied by Zig 0.16.0 with `-DCMAKE_LINK_DEPENDS_USE_LINKER=FALSE` to avoid a Zig/CMake linker dependency-file crash. This workaround is not required by ordinary GCC builds.

Linux Xorg actual runtime: SDL_Renderer OpenGL; SDL_GPU Vulkan reported Intel(R) Graphics (ARL), Intel open-source Mesa driver. Actual Vulkan and Sokol GLCORE pixel readback verifies nested clip/alpha/geometry; Sokol text cache/readback/eviction passed under ASan/UBSan. SDL dummy/software tests verify cache/clipboard/event service contracts; dummy is not OS platform evidence. C++ and Zig C-ABI consumer executables ran. Portable wasm32-wasi behavioral/embedding modules ran in pinned Wasmtime. Real Chromium browser tests separately exercise the WebAssembly gallery, including pointer/keyboard behavior, text input, popups and mobile-width bounds. These are separate validation paths. Native font/cache tests require a discovered real font.

## Captures and performance

Actual generated frames are in [screenshots](screenshots/): light gallery, dark text preview, narrow values, tiny-window resize fallback, SDL_GPU and Sokol. They are not borrowed upstream imagery. The browser gallery has also been captured from actual running CUI WebAssembly; no component mockups stand in for runtime evidence. No video captured.

CPU layout benchmark initial default unoptimized build on Intel Core Ultra 5 235U/Arch/GCC 16.2.1:500 frames × 100 boxes, 0.196 ms/frame CPU clock, 28000 visible commands after culling. This excludes shaping/rendering and is not a GPU frame or competitor benchmark. Keyed virtualization measurements/workload/memory are in VIRTUALIZATION.md. Reproduce with `make bench` and `./build/test_virtual --benchmark`.

## Platform matrix

Linux Xorg and headless: tested partial. Wayland: untested. Windows/macOS: hosted portable C suites run; results and coverage limits are in the platform matrix. Android/iOS: untested. Browser WASM: tested partial in Chromium with an optional Canvas/input host, independently of headless WASI. See [PLATFORM_MATRIX.md](PLATFORM_MATRIX.md) for current observed CI changes. SDL_GPU currently SPIR-V/Vulkan only; Metal/D3D shaders pending. Native screen-reader bridges absent.

## Risks and next tasks

1. Single-line Input now has caret/selection painting, scalar pointer placement, scrolling, host clipboard and bounded history. Complete multiline presentation, IME, graphemes, shaping clusters, paragraph bidi and font fallback before claiming Editor.
2. Nested/context menus, multiple selection, custom row delegates, pickers and disclosure/tabs have tested styled slices. Complete reference-specific behavior; dialogs, alerts and sheets now have tested styled composition and nested focus, with advanced surface behavior pending.
3. Tree, table and scroll widgets now use clipped virtual viewports and keyboard models. Complete reference-default audits, variable heights and advanced integration; table pinning/multiple selection and grouped headers now have tested implementations; Dock and Plot remain partial model stories.
4. Complete independent renderer/resource lifecycle/device-loss contracts, broader GPU shader formats, OS accessibility and native platform adapters.
5. Implement remaining catalog families against feature requirements, then add real stories/behavior/semantic/theme tests before verified status.

Licensing: original code MIT, Clay and Sokol zlib; optional renderer dependencies retain their own inventory/license. No GPUI code/runtime/branding copied. In-process capability checks would not be a security sandbox; no extension host yet.

## Browser gallery

[Open the GitHub Pages gallery](https://matteo.merola.co/cui/). All 78 entries remain navigable; only implemented stories have live previews. Models are marked as partial and no family is promoted to verified by a screenshot. Deployment is gated by Chromium behavior tests and preserves the existing account domain.

Build with `python scripts/build_web.py` using pinned Zig 0.16.0, then serve `build/web` over HTTP. See WEB_GALLERY.md for dependency and browser-test commands.

## Local commands

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/cui_gallery
./build/cui_gpu_demo --font /path/to/font.ttf
./build/cui_sokol_demo --smoke 3
make lint format-check dist-check bench
python scripts/check_manifest.py
```

No dependency installation needed on the present host. Optional system CMake/Clang and Weston commands are in MAINTAINER_ACTIONS.md; no sudo or password prompts were used unattended. CI uses hosted-runner setup, not a personal token for builds.

## Every incomplete family

| Family | Current partial status | Remaining work |
|---|---|---|
| Accordion | tested | Controlled expansion, disabled/keyboard behavior, custom content, measured reversible reveal, dynamic content height, inert painted exits and reduced motion tested. Styled animation consumes theme response/damping/epsilon/travel, including under/overdamping and immediate snap. Complete inherited styling audit remains pending. |
| AlertDialog | tested | Partial surface behavior; see docs/SURFACES.md. Nested focus, validated deferred close, resize, measured wheel scrolling and pinned footer composition are tested. Automatic root overlay management, body scrollbar/focus reveal, custom part styles/backdrop, exact default margins and animated transitions remain pending. |
| Alert | tested | Severity text, title/message, banner sizing and controlled dismissal tested. Independent success/warning theme colors, rich content/action slots and native accessibility announcement pending. |
| Attachment | not_started | Full documented behavior pending implementation and audit. |
| Avatar | tested | Named UTF-8 initials fallback, circular themed surface and explicit size tested. Image loading/failure state, avatar groups, custom icon slots and exact typographic centering pending. |
| Badge | implemented | Themed text badge renders. Count/dot/overflow and standalone semantic tests pending. |
| Bubble | tested | Seven themed composable surface variants tested. Parent owns alignment/wrapping; built-in reactions, footer and complete message slot API pending. |
| Button | tested | Nine variants, four sizes, colored outlines, controlled selected/loading/disabled states, text adornments and presentation slots tested. Selected styling stays distinct from toggle semantics. SVG icons, explicit independent toggle metadata, per-component token overrides, full reference interactions and native accessibility bridge remain pending. |
| Checkbox | tested | Binary and mixed base state tested; styled mixed state, real check geometry and validation states pending. |
| Clipboard | tested | SDL native/dummy roundtrip and optional capability adapter tested; core input paste/cut integration pending. |
| Collapsible | tested | Controlled expansion, disabled/keyboard behavior, custom content, measured reversible reveal, dynamic content height, inert painted exits and reduced motion tested. Styled animation consumes theme response/damping/epsilon/travel, including under/overdamping and immediate snap. Complete inherited styling audit remains pending. |
| DropdownButton | tested | Split actions, optional halves, tooltip, inner style inheritance, shared ghost state, corner anchors, joined geometry and eight semantic variants/four sizes tested. Remaining inherited Button style/metadata surface is under reference audit. |
| Empty | tested | Themed title/description layout and named semantic snapshot tested. Media/icon/action slots and complete upstream audit pending. |
| Focus Trap | tested | Core nested modal scopes and overlay state tested. No OS accessibility bridge or full modal relationship semantics. |
| GroupBox | tested | Composable title/content, themed border/layout and group semantic snapshot tested. Complete upstream audit and native accessibility bridge pending. |
| HoverCard | tested | Delayed hover/focus, interactive content retention, viewport portals/collision and Escape suppression tested. advanced focus ownership and complete native accessibility bridge pending. |
| Icon | not_started | Full documented behavior pending implementation and audit. |
| Image | not_started | Full documented behavior pending implementation and audit. |
| Kbd | tested | Platform modifier formatting and styled UTF-8-safe hint tested. Action/keybinding lookup and grouped shortcut composition pending. |
| Label | implemented | Text wrapper renders, but control association and standalone semantic labeling pending. |
| Marker | tested | Text wrapping, inherited font styling and arbitrary separator styling remain incomplete. Custom child widths/layout are application supplied. Default shimmer now uses alpha-aware Oklab and the source lightness fallback. |
| Message | tested | Built-in Image/TextView/Attachment content is pending; custom content callbacks are supported. Exact inherited font weight, rounded avatar clipping, arbitrary style refinements and jump-button tooltip/variants remain incomplete. Browser stories are pending. |
| MessageScroller | tested | Built-in Image/TextView/Attachment content is pending; custom content callbacks are supported. Exact inherited font weight, rounded avatar clipping, arbitrary style refinements and jump-button tooltip/variants remain incomplete. Browser stories are pending. |
| Notification | tested | Native OS delivery adapters remain absent; portable host callbacks are opt-in. Shadows approximate blur with bounded rounded layers; intent icons are text symbols and title font weight is not independently styled. Custom content and action descriptors remain borrowed through lifecycle callbacks. |
| Pagination | tested | Independent bounded one-based page model, keyboard/pointer first/last/next/previous, ellipsis window and SIZE_MAX bounds tested. Responsive wrapping, configurable label slots and arbitrary page budgets pending. |
| Progress | implemented | Determinate bar renders/clamps. Indeterminate motion, semantic values and dedicated story tests pending. |
| Radio | tested | Selection/exclusivity tested; full radio-group roving focus and arrow navigation pending. |
| Rating | tested | Independent keyboard/pointer half-step values, controlled clearing, readonly/disabled and range semantics tested. SVG masks/custom icons/color and hover preview pending. |
| Shimmer | tested | Single-line text only; inherited styled runs, wrapping, ellipsis and font weight await rich text infrastructure. Caller supplies elapsed time and schedules base-plan deadlines. |
| Skeleton | tested | Placeholder dimensions/radii, secondary opacity, phase pulse and reduced motion tested. Loading owner semantics and richer placeholder layouts pending. |
| Spinner | tested | Themed eight-dot ring, explicit clock phase, redraw invalidation, reduced motion and status snapshot tested. Scheduler deadlines and alternative icon geometry pending. |
| Stepper | tested | Controlled current index, explicit pending/active/completed/error states, pointer/keyboard navigation and semantic snapshots tested. Validation callbacks, connectors/custom icons and exact density variants pending. |
| Switch | tested | Controlled activation tested; native switch geometry, theme variants and touch pending. |
| Tag | tested | Frozen Tag has no built-in close action; interactive children are application composition. Semantic secondary colors are independently configurable. Exact inherited typography remains incomplete. |
| Toggle | tested | Controlled press state tested; toggle groups and full visual states pending. |
| Tooltip | tested | Delayed pointer/focus opening, collision placement, viewport portals, Escape suppression, disabled omission and trigger description tested. configurable layer policy and native accessibility bridge pending. |
| Calendar | tested | Actual one-to-three month Gregorian grid, ordered keyboard navigation, controlled date/range selection, disabled/min/max predicates and roving semantic days tested. Locale labels, week numbers, today markers, hover range previews and neighboring-day interaction remain pending. |
| ColorPicker | tested | Actual HSV plane, hue/alpha/RGBA sliders and palettes; pointer capture, keyboard edits, disabled/readonly and 20,000 conversion/hex round trips tested. Editable text channels, continuous gradients, eyedropper and popup presentation remain pending. |
| Combobox | tested | Editable UTF-8 query with measured caret, selection, pointer placement and horizontal scrolling; ASCII filtering; single/multiple selection; protected removable chips; and ordered text/activation/wheel commits are tested. Remote data, Unicode case folding, rich IME/grapheme editing and complete upstream behavior remain pending. |
| DatePicker | tested | Real collision-placed calendar portal, date/range selection, trapped Tab and same-frame focus restoration tested. Editable date masks, custom formats/locales, clear controls, presets and responsive scrolling remain pending. |
| Editor | implemented | Reusable bounded UTF-8 scalar editing model exists. No source editor, gutter, highlighting, diagnostics, grapheme editing or200K-line validation. |
| Form | tested | Validation, submission and responsive column counts are application-owned as in the frozen reference.; Feature-level reference audit remains in progress; inherited Input text/IME/native accessibility limitations remain. |
| Input | tested | Actual single-line caret/selection painting, UTF-8 scalar pointer positioning, horizontal scrolling, disabled/readonly/required/invalid semantics, host clipboard and bounded undo/redo shortcuts tested. Multiline presentation, graphemes, IME, bidi, shaping cluster hit testing and asynchronous browser clipboard integration remain pending. |
| Input Group | tested | Reference audit remains in progress; actual gallery composition and state tests exist.; Uses retained single-line Input; multiline, IME, grapheme and native accessibility limitations remain. |
| NumberInput | tested | Independent headless and styled finite numeric editing, draft validation, CJK normalization, locale-aware ASCII parsing/formatting, bounds, steps, readonly/disabled and semantics tested. Thousands separators, full custom pattern/appearance, caret/selection painting, pointer positioning and IME pending. |
| OtpInput | tested | Actual gallery integration and feature-level audit remain in progress.; Host schedules caret blink deadlines. OS OTP autofill and IME bridges are absent. Frozen source does not provide bulk paste or caret-arrow editing; CUI does not infer these features. |
| Questionnaire | not_started | Full documented behavior pending implementation and audit. |
| Select | tested | Controlled single/multiple selection, removable protected chips, ordered activations, keyboard/typeahead, collision placement and modal focus/restore tested. remote loading, full virtualization remain pending. |
| Slider | tested | Capture drag, keyboard step/clamp and disabled tests pass; tick/handle geometry and touch pending. |
| Speech | not_started | Full documented behavior pending implementation and audit. |
| Textarea | not_started | Full documented behavior pending implementation and audit. |
| TimeField | tested | Controlled 12/24-hour segmented hour/minute/optional-second model, persistent digit drafts, AM/PM, strict ASCII parsing and readonly/disabled behavior tested. Min/max times, typed separators, localization and IME remain pending. |
| Carousel | not_started | Full documented behavior pending implementation and audit. |
| DescriptionList | not_started | Full documented behavior pending implementation and audit. |
| Dialog | tested | Partial surface behavior; see docs/SURFACES.md. Nested focus, validated deferred close, resize, measured wheel scrolling and pinned footer composition are tested. Automatic root overlay management, body scrollbar/focus reveal, custom part styles/backdrop, exact default margins and animated transitions remain pending. |
| Dock | tested | Retained split/tab tree, stable keys, constrained geometry and transactional versioned persistence tested. Styled Dock, drag/drop, resize gestures, floating windows and keyboard/focus integration pending. |
| List | tested | Stable keyed single/multiple selection, per-event activations, roving keyboard/typeahead, disabled items, semantic options and bounded visible window tested. remote data, full virtualization remain pending. |
| Menu | tested | Actual flat/nested menus, check/radio, separators, text adornments, custom row delegates, context opening with modal guard, bounded collision placement and immediate focus restoration tested. OS shortcuts/link activation, arbitrary nested viewport scrolling and complete upstream behavior remain pending. |
| Popover | tested | Partial surface behavior; see docs/SURFACES.md for exact upstream gaps. Automatic root overlay management, independent scrolling with pinned footer, frame-independent close requests, customizable part styles and transitions remain pending.; Current side-based flip/clamp differs from upstream anchor-only clamp. Complete anchor/offset/arrow, right-click trigger and accessible trigger relationships are pending. |
| Resizable | not_started | Full documented behavior pending implementation and audit. |
| Root View | not_started | Full documented behavior pending implementation and audit. |
| Scrollable | tested | Actual clipped two-axis content, nested wheel routing and keyboard/pointer scrollbars tested. Auto-measured/grow sizing, inertia/touch, auto-hide and native system behavior remain pending. |
| Settings | not_started | Full documented behavior pending implementation and audit. |
| Sheet | tested | Partial surface behavior; see docs/SURFACES.md. Nested focus, validated deferred close, resize, measured wheel scrolling and pinned footer composition are tested. Automatic root overlay management, body scrollbar/focus reveal, custom part styles/backdrop, exact default margins and animated transitions remain pending. |
| Sidebar | not_started | Full documented behavior pending implementation and audit. |
| StatusBar | not_started | Full documented behavior pending implementation and audit. |
| Tabs | tested | Stable-ID selection, keyboard navigation, five variants, custom body slots/close composition, measured UTF-8 ellipsis, wheel scrolling, overflow menu, application-applied drag requests, explicit/selected reveal and interactive scrollbars tested. Custom body width caps preserve short content and clip oversized bodies from the first frame. Structured per-item slots and text shaping remain incomplete. Cross-strip drag/autoscroll are not APIs in the frozen source. |
| Theme | tested | Four runtime token structs tested; system integration, comprehensive tokens and reduced motion pending. |
| TitleBar | not_started | Full documented behavior pending implementation and audit. |
| Toolbar | not_started | Full documented behavior pending implementation and audit. |
| Tree | tested | Actual bounded keyed hierarchy, controlled expand/select, ordered parent/child navigation, fractional virtual viewport and clipped wheel routing tested. Variable row heights, drag/drop, custom row slots, full context actions and native accessibility remain pending. |
| VirtualList | tested | Retained keyed variable-height Fenwick model tested including anchor restoration. No styled scroll/focus/navigation widget yet. |
| Chart | not_started | Full documented behavior pending implementation and audit. |
| Command | not_started | Full documented behavior pending implementation and audit. |
| DataTable | tested | Actual keyed rows/columns, controlled sort, bounded multiple selection, pointer/keyboard resize/reorder, pinned left columns and row headers, virtual rows/columns, context requests, load-range signaling, multi-level group headers, whole-row/trailing callbacks and actual scrollbars tested. Complete reference-default, remote adapter and behavior audit remains in progress. |
| Plot | tested | Allocation-free linear/log scales, inverse mapping, ticks, pan/anchored zoom and clipped line/bar geometry tested. Band/ordinal/category/time scales, retained series, axes/labels/legend, missing-data segmentation, stacks, hover/selection, styled output and chart accessibility pending. |
| Table | tested | Real themed rows/cells, static header/body/footer sections, captions and validated spans plus associated data-table behavior tested. Exhaustive alignment/sizing/style composition and complete upstream scope remain pending. |
| TextView | not_started | Full documented behavior pending implementation and audit. |

## Publishing/CI checkpoint

Safe branch pushes are performed with ordinary Git; no force push/history rewrite. This report records the evidence commit above. Integrator updates publishing/CI observations in swarm-status.md and PLATFORM_MATRIX.md after each push.
