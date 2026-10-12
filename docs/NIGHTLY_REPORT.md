# Nightly report — live development checkpoint

Repository: https://github.com/mattmezza/cui. Branch `main`. Evidence/source commit before this report update: `b582fde631fa7440ad2ed7d0660fdbb49f75f309`. See swarm-status.md for branch promotion and push/CI observations.

## Working architecture

Explicit C11 contexts, pinned Clay layout, stable scoped IDs, bounded frame strings, ordered previous-frame hit routing, capture/focus/modal scopes, independent semantics and draw lists. Unstyled controlled primitives feed theme-driven components. UTF-8 scalar editing with bounded undo/redo, retained keyed Fenwick virtualization, split/tab docking with transactional persistence and plot scales/clipped geometry are substantive headless models. Optional FreeType/HarfBuzz shapes runs. Software, SDL_Renderer compatibility, SDL_GPU Vulkan and independent Sokol GLCORE are separate renderers; actual hardware/runtime selection is reported.

GPUI Kit baseline 0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`; Clay e6cc36941ab2af5d81107617039d6f527a1c660b. See upstream-baseline.md and DECISIONS.md. Source code and generated one/few headers are tested independently.

## Catalog counts

| Status | Families |
|---|---:|
| not_started | 35 |
| scaffolded | 0 |
| implemented | 4 |
| tested | 39 |
| verified | 0 |

Every non-verified family remains incomplete; tested means only its documented subset has tests. **No full family is verified.** Full feature backlog: [gpui-parity.json](gpui-parity.json), [COMPONENTS.md](COMPONENTS.md).

## Validation evidence

Native strict GCC31/31 (including GPU/readback and picker base-only), source b582fde; instrumented ASan/UBSan29/29 and WASI22/22 at picker integration6bbb664; actual Chromium39-story gallery passed against b582fde with the same production sources. Earlier optimized Release24/24 applies before picker additions. See validation logs for exact scope. Full78-family feature parity remains unfinished.

Latest stored logs (including the preserved initial WASI failure and corrected rerun):

- [browser-39.txt](validation/browser-39.txt)
- [ci-01865d5.txt](validation/ci-01865d5.txt)
- [clang-zig-headless.txt](validation/clang-zig-headless.txt)
- [gcc-offset.txt](validation/gcc-offset.txt)
- [gcc-pickers.txt](validation/gcc-pickers.txt)
- [gcc-pointer.txt](validation/gcc-pointer.txt)
- [gcc-resumed.txt](validation/gcc-resumed.txt)
- [gcc-xorg.txt](validation/gcc-xorg.txt)
- [release-headless.txt](validation/release-headless.txt)
- [release-resumed.txt](validation/release-resumed.txt)
- [sanitizers-pickers.txt](validation/sanitizers-pickers.txt)
- [sanitizers-resumed.txt](validation/sanitizers-resumed.txt)
- [sanitizers.txt](validation/sanitizers.txt)
- [sokol-asan-xorg.txt](validation/sokol-asan-xorg.txt)
- [wasi-initial.txt](validation/wasi-initial.txt)
- [wasi-pickers.txt](validation/wasi-pickers.txt)
- [wasi-resumed-initial.txt](validation/wasi-resumed-initial.txt)
- [wasi-resumed.txt](validation/wasi-resumed.txt)
- [wasi.txt](validation/wasi.txt)

Commands: `cmake -S . -B build -G Ninja -DCUI_GPU_TESTS=ON`; `cmake --build build`; `ctest --test-dir build --output-on-failure`. ASan/UBSan config uses `-DCUI_SANITIZE=ON`; portable Clang 21.1.8 is supplied by Zig 0.16.0 with `-DCMAKE_LINK_DEPENDS_USE_LINKER=FALSE` to avoid a Zig/CMake linker dependency-file crash. This workaround is not required by ordinary GCC builds.

Linux Xorg actual runtime: SDL_Renderer OpenGL; SDL_GPU Vulkan reported Intel(R) Graphics (ARL), Intel open-source Mesa driver. Actual Vulkan and Sokol GLCORE pixel readback verifies nested clip/alpha/geometry; Sokol text cache/readback/eviction passed under ASan/UBSan. SDL dummy/software tests verify cache/clipboard/event service contracts; dummy is not OS platform evidence. C++ and Zig C-ABI consumer executables ran. Portable wasm32-wasi behavioral/embedding modules ran in pinned Wasmtime. Real Chromium browser tests separately exercise the WebAssembly gallery, including pointer/keyboard behavior, text input, popups and mobile-width bounds. These are separate validation paths. Native font/cache tests require a discovered real font.

## Captures and performance

Actual generated frames are in [screenshots](screenshots/): light gallery, dark text preview, narrow values, tiny-window resize fallback, SDL_GPU and Sokol. They are not borrowed upstream imagery. The browser gallery has also been captured from actual running CUI WebAssembly; no component mockups stand in for runtime evidence. No video captured.

CPU layout benchmark initial default unoptimized build on Intel Core Ultra 5 235U/Arch/GCC 16.2.1:500 frames × 100 boxes, 0.196 ms/frame CPU clock, 28000 visible commands after culling. This excludes shaping/rendering and is not a GPU frame or competitor benchmark. Keyed virtualization measurements/workload/memory are in VIRTUALIZATION.md. Reproduce with `make bench` and `./build/test_virtual --benchmark`.

## Platform matrix

Linux Xorg and headless: tested partial. Wayland: untested. Windows/macOS: hosted portable C suites run; results and coverage limits are in the platform matrix. Android/iOS: untested. Browser WASM: tested partial in Chromium with an optional Canvas/input host, independently of headless WASI. See [PLATFORM_MATRIX.md](PLATFORM_MATRIX.md) for current observed CI changes. SDL_GPU currently SPIR-V/Vulkan only; Metal/D3D shaders pending. Native screen-reader bridges absent.

## Risks and next tasks

1. Input is a preview: scalar boundaries, no painted caret/selection/pointer placement, IME, clipboard editor path, graphemes, paragraph bidi or font fallback. Finish this usable foundation before claiming Editor.
2. Menus, selects, comboboxes, disclosure/tabs and floating content have working styled slices. Complete nested/menu/selection features and styled dialogs/sheets remain in progress.
3. Virtualization is a real efficient keyed model, but a scroll/focus/widget layer and data table remain. Dock/advanced model progress must retain widget-level gaps.
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
| Accordion | tested | Single/multiple controlled expansion, disabled triggers, keyboard traversal and semantic panel relationships tested. Animated reveal, custom header slots and complete upstream audit pending. |
| AlertDialog | not_started | Full documented behavior pending implementation and audit. |
| Alert | tested | Severity text, title/message, banner sizing and controlled dismissal tested. Independent success/warning theme colors, rich content/action slots and native accessibility announcement pending. |
| Attachment | not_started | Full documented behavior pending implementation and audit. |
| Avatar | tested | Named UTF-8 initials fallback, circular themed surface and explicit size tested. Image loading/failure state, avatar groups, custom icon slots and exact typographic centering pending. |
| Badge | implemented | Themed text badge renders. Count/dot/overflow and standalone semantic tests pending. |
| Bubble | tested | Seven themed composable surface variants tested. Parent owns alignment/wrapping; built-in reactions, footer and complete message slot API pending. |
| Button | tested | Six variants, three sizes, controlled selected/loading/disabled states, text adornments and presentation slots tested. SVG icons, per-component token overrides, complete upstream interactions and native accessibility bridge pending. |
| Checkbox | tested | Binary and mixed base state tested; styled mixed state, real check geometry and validation states pending. |
| Clipboard | tested | SDL native/dummy roundtrip and optional capability adapter tested; core input paste/cut integration pending. |
| Collapsible | tested | Controlled keyboard/pointer disclosure and panel semantics tested. Animated reveal and custom trigger slots pending. |
| DropdownButton | not_started | Split button/menu functionality pending implementation and audit. |
| Empty | tested | Themed title/description layout and named semantic snapshot tested. Media/icon/action slots and complete upstream audit pending. |
| Focus Trap | tested | Core nested modal scopes and overlay state tested. No OS accessibility bridge or full modal relationship semantics. |
| GroupBox | tested | Composable title/content, themed border/layout and group semantic snapshot tested. Complete upstream audit and native accessibility bridge pending. |
| HoverCard | tested | Delayed hover/focus, interactive content retention, viewport portals/collision and Escape suppression tested. advanced focus ownership and complete native accessibility bridge pending. |
| Icon | not_started | Full documented behavior pending implementation and audit. |
| Image | not_started | Full documented behavior pending implementation and audit. |
| Kbd | tested | Platform modifier formatting and styled UTF-8-safe hint tested. Action/keybinding lookup and grouped shortcut composition pending. |
| Label | implemented | Text wrapper renders, but control association and standalone semantic labeling pending. |
| Marker | not_started | Full documented behavior pending implementation and audit. |
| Message | not_started | Full documented behavior pending implementation and audit. |
| MessageScroller | not_started | Full documented behavior pending implementation and audit. |
| Notification | not_started | Full documented behavior pending implementation and audit. |
| Pagination | tested | Independent bounded one-based page model, keyboard/pointer first/last/next/previous, ellipsis window and SIZE_MAX bounds tested. Responsive wrapping, configurable label slots and arbitrary page budgets pending. |
| Progress | implemented | Determinate bar renders/clamps. Indeterminate motion, semantic values and dedicated story tests pending. |
| Radio | tested | Selection/exclusivity tested; full radio-group roving focus and arrow navigation pending. |
| Rating | tested | Independent keyboard/pointer half-step values, controlled clearing, readonly/disabled and range semantics tested. SVG masks/custom icons/color and hover preview pending. |
| Shimmer | not_started | Full documented behavior pending implementation and audit. |
| Skeleton | tested | Placeholder dimensions/radii, secondary opacity, phase pulse and reduced motion tested. Loading owner semantics and richer placeholder layouts pending. |
| Spinner | tested | Themed eight-dot ring, explicit clock phase, redraw invalidation, reduced motion and status snapshot tested. Scheduler deadlines and alternative icon geometry pending. |
| Stepper | tested | Controlled current index, explicit pending/active/completed/error states, pointer/keyboard navigation and semantic snapshots tested. Validation callbacks, connectors/custom icons and exact density variants pending. |
| Switch | tested | Controlled activation tested; native switch geometry, theme variants and touch pending. |
| Tag | not_started | Full documented behavior pending implementation and audit. |
| Toggle | tested | Controlled press state tested; toggle groups and full visual states pending. |
| Tooltip | tested | Delayed pointer/focus opening, collision placement, viewport portals, Escape suppression, disabled omission and trigger description tested. configurable layer policy and native accessibility bridge pending. |
| Calendar | tested | Actual one-to-three month Gregorian grid, ordered keyboard navigation, controlled date/range selection, disabled/min/max predicates and roving semantic days tested. Locale labels, week numbers, today markers, hover range previews and neighboring-day interaction remain pending. |
| ColorPicker | tested | Actual HSV plane, hue/alpha/RGBA sliders and palettes; pointer capture, keyboard edits, disabled/readonly and 20,000 conversion/hex round trips tested. Editable text channels, continuous gradients, eyedropper and popup presentation remain pending. |
| Combobox | tested | Editable UTF-8 query, ASCII filtering, single/multiple selection, removable protected chips, filtered masks preserving hidden selections, ordered activations and popup dismissal tested. Remote data, Unicode case folding and fully interleaved text/activation reduction remain pending. |
| DatePicker | tested | Real collision-placed calendar portal, date/range selection, trapped Tab and same-frame focus restoration tested. Editable date masks, custom formats/locales, clear controls, presets and responsive scrolling remain pending. |
| Editor | implemented | Reusable bounded UTF-8 scalar editing model exists. No source editor, gutter, highlighting, diagnostics, grapheme editing or200K-line validation. |
| Form | not_started | Full documented behavior pending implementation and audit. |
| Input | tested | Real UTF-8 scalar editor with bounded undo/redo; no caret/selection painting, graphemes, IME, clipboard integration or pointer position. |
| Input Group | not_started | Full documented behavior pending implementation and audit. |
| NumberInput | tested | Independent headless and styled finite numeric editing, draft validation, CJK normalization, locale-aware ASCII parsing/formatting, bounds, steps, readonly/disabled and semantics tested. Thousands separators, full custom pattern/appearance, caret/selection painting, pointer positioning and IME pending. |
| OtpInput | not_started | Full documented behavior pending implementation and audit. |
| Questionnaire | not_started | Full documented behavior pending implementation and audit. |
| Select | tested | Controlled single/multiple selection, removable protected chips, ordered activations, keyboard/typeahead, collision placement and modal focus/restore tested. Custom option slots, remote loading, pointer wheel scrolling and full virtualization remain pending. |
| Slider | tested | Capture drag, keyboard step/clamp and disabled tests pass; tick/handle geometry and touch pending. |
| Speech | not_started | Full documented behavior pending implementation and audit. |
| Textarea | not_started | Full documented behavior pending implementation and audit. |
| TimeField | tested | Controlled 12/24-hour segmented hour/minute/optional-second model, persistent digit drafts, AM/PM, strict ASCII parsing and readonly/disabled behavior tested. Min/max times, typed separators, localization and IME remain pending. |
| Carousel | not_started | Full documented behavior pending implementation and audit. |
| DescriptionList | not_started | Full documented behavior pending implementation and audit. |
| Dialog | not_started | Full documented behavior pending implementation and audit. |
| Dock | tested | Retained split/tab tree, stable keys, constrained geometry and transactional versioned persistence tested. Styled Dock, drag/drop, resize gestures, floating windows and keyboard/focus integration pending. |
| List | tested | Stable keyed single/multiple selection, per-event activations, roving keyboard/typeahead, disabled items, semantic options and bounded visible window tested. Mouse wheel scrolling, remote data, custom rows and full virtualization remain pending. |
| Menu | tested | Flat portal menu actions, keyboard/typeahead, collision placement, outside/Escape dismissal, disabled controls and immediate focus restoration tested. Nested submenus, separators, check/radio items and custom rows pending. |
| Popover | tested | Base collision placement and popup dismissal reducer only. No styled portal/anchor UI or semantic relationship. |
| Resizable | not_started | Full documented behavior pending implementation and audit. |
| Root View | not_started | Full documented behavior pending implementation and audit. |
| Scrollable | not_started | Full documented behavior pending implementation and audit. |
| Settings | not_started | Full documented behavior pending implementation and audit. |
| Sheet | not_started | Full documented behavior pending implementation and audit. |
| Sidebar | not_started | Full documented behavior pending implementation and audit. |
| StatusBar | not_started | Full documented behavior pending implementation and audit. |
| Tabs | tested | Controlled stable-ID selection, horizontal/vertical keyboard navigation, manual activation, five themed variants, roving Tab and panel relationships tested. Overflow menus, close buttons, drag reorder and custom tab slots pending. |
| Theme | tested | Four runtime token structs tested; system integration, comprehensive tokens and reduced motion pending. |
| TitleBar | not_started | Full documented behavior pending implementation and audit. |
| Toolbar | not_started | Full documented behavior pending implementation and audit. |
| Tree | not_started | Full documented behavior pending implementation and audit. |
| VirtualList | tested | Retained keyed variable-height Fenwick model tested including anchor restoration. No styled scroll/focus/navigation widget yet. |
| Chart | not_started | Full documented behavior pending implementation and audit. |
| Command | not_started | Full documented behavior pending implementation and audit. |
| DataTable | not_started | Full documented behavior pending implementation and audit. |
| Plot | tested | Allocation-free linear/log scales, inverse mapping, ticks, pan/anchored zoom and clipped line/bar geometry tested. Band/ordinal/category/time scales, retained series, axes/labels/legend, missing-data segmentation, stacks, hover/selection, styled output and chart accessibility pending. |
| Table | not_started | Full documented behavior pending implementation and audit. |
| TextView | not_started | Full documented behavior pending implementation and audit. |

## Publishing/CI checkpoint

Safe branch pushes are performed with ordinary Git; no force push/history rewrite. This report records the evidence commit above. Integrator updates publishing/CI observations in swarm-status.md and PLATFORM_MATRIX.md after each push.
