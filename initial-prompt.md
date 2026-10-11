# CUI: Full Framework and Component Kit — Autonomous Agent-Swarm Build Brief

> Copy this entire file into the **master/orchestrator agent** running on my development machine. You are being asked to **build a working repository**, not to respond with a design document. Use your available agent delegation, terminal, Git, GitHub, browser and coding capabilities.

## 0. Role, mission, and operating mode

You are the principal architect, engineering lead, release integrator and quality gate for an autonomous coding swarm. Build **CUI** (“C UI”), a modern, C-first, high-performance, hardware-accelerated, themeable, cross-platform native application UI framework and comprehensive component ecosystem, using **Clay** for layout and **pluggable renderer/platform backends**.

**Target monorepo:** `https://github.com/mattmezza/cui` (owner `mattmezza`, repository `cui`).

**Do the engineering work.** Initialize and populate the repository, write production-oriented code, build examples, tests, docs, automated CI, demos and benchmarks, run validation, commit and push when credentials permit. Continue iteratively for the entire unattended run. Do not stop after creating scaffolding or a TODO list.

The whole GPUI Kit-like ecosystem is the **project completion target**, not a credible one-night promise. Implement as much fully functioning, testable vertical functionality as possible each run. Preserve the complete scope in a tracked manifest and leave an **honest machine-readable status** for everything unfinished. Never declare a component implemented merely because its C symbol, header, screenshot or mock is present.

### My environment / preferences

- Primary development machine: **Arch Linux, Xorg**. Keep Linux/Wayland, Windows, macOS, Android and iOS compatibility in mind; avoid Linux-only design assumptions.
- Language: **idiomatic portable C11** for the public API and core. C++ must not leak into CUI's public API. Isolate optional dependencies that internally use C++.
- Preserve **header-only embeddability** for the public core, headless base and styled kit. Permissively licensed compiled platform/text dependencies may be used as explicit optional integrations.
- Model allocation suggestion, if my orchestration environment exposes these models: one powerful coordinator **GPT-6 Astra / high reasoning** and a carefully chosen group of **GPT-5.6 Sol / medium** implementation workers. Adapt to actual available models rather than blocking.
- I am **away overnight**. Do not ask interactive clarifying questions or wait on them. Resolve ordinary ambiguity with sound documented defaults. Do not use `sudo` or ask for password prompts while unattended; work with present tools, defer nonessential system setup and record exact follow-up commands in `docs/MAINTAINER_ACTIONS.md`.
- GitHub repo access: inspect `gh auth status` and remote state first. If repo is absent and authenticated, create **private** repo under `mattmezza` by default (ready to open-source later). If it already exists, inspect and preserve it; do not force-push, delete, overwrite unknown files or expose secrets. If authentication fails, continue locally, commit, and record exact publishing steps.

## 1. Vision and differentiation

CUI should offer:

1. **Clay-like smallness and flexbox-inspired declarative ergonomics**.
2. **GPUI-like production-grade desktop behavior**: fast redraw, keyboard navigation, text editing, complex tables, virtualization, docking and polished interaction.
3. **GPUI Base / Base UI-like separation of behavior and presentation**: applications can use completely unstyled primitives or styled ready-to-use components.
4. **shadcn-inspired components**: excellent defaults, accessible behavior, semantic tokens, composable parts, easy source vendoring/customization.
5. **C-first, GPU accelerated, host-embeddable architecture**, with pluggable renderers, no browser engine and no mandatory VM/runtime.
6. **Good web-developer mental models**, not web technology reimplementation: rows/columns, gap, padding, flex, state, variants, composition, theme tokens and clear events.
7. **Desktop-first**, but design and test for Linux (Xorg + Wayland), Windows, macOS, Android, iOS and WebAssembly. Do not falsely claim platform support merely because the source compiles.

Do **not** create a Chromium/Electron wrapper, a Flutter clone, a new programming language or a custom Rust core. Avoid introducing a CSS/HTML/JS runtime as a prerequisite. A constrained Markdown/HTML viewer is a **component**, not the application's layout system.

## 2. Canonical references and scope snapshot

Read these before implementing. Inspect source and docs, not just READMEs:

- GPUI Kit `v0.7.1`: https://gpui-kit.com/component/ (complete documented component navigation/catalog)
- GPUI Base `v0.7.1`: https://gpui-kit.com/base/
- GPUI Kit repository: https://github.com/longbridge/gpui-kit
- GPUI Kit architecture: https://github.com/longbridge/gpui-kit/blob/main/docs/ARCHITECTURE.md
- GPUI Kit design guides: https://github.com/longbridge/gpui-kit/blob/main/website/docs/design-guides.md
- Clay upstream: https://github.com/nicbarker/clay
- SDL3: https://wiki.libsdl.org/SDL3/FrontPage
- SDL_GPU: https://wiki.libsdl.org/SDL3/CategoryGPU
- Sokol: https://github.com/floooh/sokol
- HarfBuzz: https://harfbuzz.github.io/
- FreeType: https://freetype.org/
- utf8proc: https://github.com/JuliaStrings/utf8proc
- AccessKit: https://github.com/AccessKit/accesskit

**Freeze the initial comparison against GPUI Kit `0.7.1` as documented in this brief** and capture the exact upstream source commit/URL/date in `docs/upstream-baseline.md`. Do not chase moving upstream `main` for the duration of the first implementation cycle. Treat upstream as a **functional specification/inspiration**, not a license to copy branding, screenshots, proprietary assets or documentation wholesale. Their software code is Apache-2.0; their documentation prose/illustration licensing differs. Honor all licenses and attribution requirements, and prefer original C implementations.

CUI is NOT a Rust port of GPUI and shall NOT use the GPUI runtime. It is its own C UI framework with a GPUI Kit-inspired breadth and architectural layering.

## 3. Required architecture

Follow this dependency direction, without exceptions:

```text
               APPLICATION (ordinary C/C++/Rust/Zig/Go host)
                             |
             +---------------+----------------+
             |                                |
       CUI COMPONENTS                  CUSTOM DESIGN SYSTEM
       styled, themed                   authored by application
             |                                |
             +----------------+---------------+
                              v
                         CUI BASE
       unstyled reusable behavior, state machines, geometry,
             accessibility semantics, compound primitives
                              |
                              v
                        CUI CORE
          Clay layout, IDs, lifecycle, input, focus,
             semantics tree, state, draw-list API
                  /                      \
                 v                        v
        RENDERER INTERFACE          PLATFORM INTERFACE
       GPU + text + resources       window/input/IME/etc.
         /      |      \             /         |       \
      SDL_GPU  Sokol  headless       SDL3     host     native
                 \                    /
                independent, optional adapters

OPTIONAL SIDE MODULE: CUI SHELL (JS extension host) --> CUI BASE
Never required by the standard C application path.
```

### Core (`cui.h`)

- Vendored/pinned **Clay** with visible attribution; encapsulate its implementation as much as possible. No CUI API may depend on raw Clay structs for normal application use. Permit explicit escape hatches for advanced users.
- Explicit `cui_context` instances; no unsafe singleton globals. Support multiple independent windows/contexts.
- Stable hierarchical widget IDs with collision detection in debug builds; repeatable nested/reordered lists; user-supplied keys; deterministic state retention and garbage collection.
- Clear frame lifecycle and event ordering. Define whether events are processed against previous-frame hit regions; correctly handle first frame, lost pointer capture, window resize, scroll, overlays and focus changes without one-frame action bugs.
- Event queue/dispatch for mouse, keyboard, text composition/IME, touch, pen where possible, gamepad optional, window events and application commands. Consistent key repeat, modifiers, focus scopes, capture, dismissal, shortcut contexts and pointer capture. Track hover/pressed/focus-visible/drag states.
- Dirty invalidation and event-driven redraw, time-based animation wakeups, high DPI, safe areas, scale changes, clipping, z-order, overlays/portals, scroll coordinates, hit testing.
- Clear UTF-8 string/span conventions and memory/ownership contract: user-owned, frame-owned, persistent arena, device-owned. Configure allocators, frame arenas and memory budgets; no unbounded per-frame malloc churn. Detect use-after-frame pointers in debug if feasible.
- Public geometry, paint, image, icon and text primitives with stable versioned types; support gradients, rounded rectangles, strokes, shadow, clipping, opacity and transformations as capabilities or documented fallbacks.
- No immediate dependency on SDL, OpenGL, Vulkan, OS-specific types or C++ in `cui.h`.
- Threading rules: UI context ownership, rendering thread affinity, async task integration and host event callbacks clearly documented.

### Headless base (`cui_base.h`)

- **Behavior and geometry**, not brand, border, fonts, colors, shadows or density choices.
- Primitives: button activation, checkbox/radio/switch/toggle, slider, selection and controlled values.
- Shared infrastructure: focus management, keyboard navigation, focus traps, accessibility roles/actions, overlays, popup collision placement, dismissal, roving tabindex, shortcuts, gesture recognition, drag-and-drop, navigation/history, undo-redo, motion/animation, scrolling, virtualization, selection model and text editing.
- Compound state machines for menu, select, combobox, dialog, popover, tabs, accordion, tree, dock, resizable regions, data tables, calendar, color picker, command palette and other catalog entries.
- Base should own geometry needed for behavior (popup placement, virtual list offsets, calendar grid, docking split layout). Do not invent a second general layout tree on top of Clay.
- App-controlled persistent domain data; CUI-controlled ephemeral interaction state; explicit handles for heavyweight editors, docks and data models.
- Semantic/accessible tree must exist **independently from painted pixels**.
- Base must compile and function **without** including the styled components or default theme assets.

### Styled components (`cui_components.h`) and themes (`cui_theme.h`)

- Every styled component builds on its corresponding base behavior. No separately coded button/menu/combobox keyboard semantics in the styling layer.
- Composable subparts/slots, variants, sizes/densities, disabled/hover/pressed/focus-visible/loading/error/selected states, controlled/uncontrolled where appropriate, optional icons and flexible content.
- Semantic tokens: background, foreground, muted, surface, panel, primary, secondary, accent, destructive, warning, success, border, focus ring, radii, shadow, spacing, typography and motion.
- Built-in coherent light and dark themes, system preference integration, runtime switch, per-component overrides, high contrast and reduced-motion support. Provide at least one tasteful original default aesthetic; avoid copying GPUI Kit's brand-identical visual designs.
- Theme can be defined as a C struct; optional human-editable theme file loader may be offered without imposing CSS or a large parser on the core.
- Ship an optional source-vendoring workflow (or generator) that copies selected components plus dependencies with attribution and stable upgrades. Do not make a CLI mandatory for ordinary CUI usage.

### Optional scripting / extension layer (`cui_shell.h`)

GPUI Kit includes a separate `gpui-shell` JavaScript extension runtime. We have not required JavaScript for normal CUI development, but **the complete longer-term ecosystem should offer an optional equivalent**, without making it a dependency of the core/base/styled layers.

- Define an independent extension host ABI: register components/actions, inspect allowed context, subscribe to events and mount custom UI through the same base primitives.
- Implement an optional small C-embeddable JavaScript engine adapter (evaluate QuickJS or alternatives for licensing, footprint and support), plus an end-to-end scripted component example.
- Host explicitly grants each capability; no filesystem/network/process access by default. **In-process capability checks are not a security sandbox**: document that untrusted extensions need process isolation or another stronger boundary.
- Permit hot-reloading during development if practical, with proper cleanup of handlers/state.
- Schedule implementation after core and component fundamentals. Never let extension work displace functioning rendering, text, focus or the 78-component parity goal.

### Rendering (independent abstraction)

- A documented **renderer vtable/interface** with capabilities querying: create/destroy surface, viewport/scale, begin/end/present frame, draw list, glyph/image handles, upload/free resources, clipping/stencil, render diagnostics, device loss and fallback.
- CUI components and base may NOT call SDL/Vulkan/Metal/D3D/Sokol methods directly.
- A hardware-accelerated **SDL3 + SDL_GPU** reference renderer; native D3D12/Metal/Vulkan where platform hardware supports it.
- A genuinely independent **Sokol** GPU renderer as second accelerated integration target; keep windowing separate from rendering, and validate external host GPU context behavior. Do not assume Sokol's Linux X11 app layer satisfies Wayland requirements.
- Optional accelerated SDL_Renderer-based 2D compatibility backend where that is a better available option; optional deterministic **software/headless renderer** mandatory for tests.
- Text shaping/rasterization are **separate interfaces** from generic geometric GPU rendering. Build a glyph atlas, cache advances/metrics, batch draw commands and handle subpixel/high-DPI correctly.
- Support correct ordering of nested clipping, overlays, alpha, rounded corners, antialiasing, multiline text, Unicode grapheme alignment, image scaling and GPU resource lifecycle.
- Rendering can be plugged into an application's existing window, loop and GPU setup; provide a batteries-included SDL3 launcher as an option.
- Report actual selected acceleration/driver/backend at runtime. Never silently assert 'GPU accelerated' when software mode is active.

### Platform abstraction

- SDL3 default adapter for Linux X11/Wayland, Windows, macOS and mobile platform windows/input.
- Host callbacks for existing windows, clocks, clipboard, text input, accessibility, URLs, timers, file pickers and dialogs.
- Isolate platform-specific Apple Objective-C/Swift interop, Android JNI/Java/Kotlin glue and Windows OS integration outside public core headers.
- IME, clipboard, drag-and-drop, cursors, window chrome, display scaling, safe areas, system theme, keyboard navigation, text input and touch must be explicit capabilities.
- Platform-aware styles may adapt density and interactions; core component behavior and public API remain shared.

## 4. Dependencies and distribution policy

Default bias: small, well-maintained permissive dependencies, pinned versions, reproducible offline-friendly builds and no unnecessary system requirements.

- Required core: standard C11 + vendored Clay.
- Default app integration: SDL3 (compiled dependency; allowed) + SDL_GPU (part of SDL3).
- Production international typography: FreeType + HarfBuzz (or SDL3_ttf with confirmed HarfBuzz shaping). Isolate APIs; support proper shaping, Unicode clusters, bidi/complex scripts, line breaking, selection and IME. Use utf8proc as appropriate. **Do not pretend `stb_truetype` alone is adequate for a full international desktop UI.**
- Images: small decoder such as `stb_image` or optional SDL3_image; accept host-provided texture/image callbacks.
- SVG/icons: compile original/Lucide-compatible permissively licensed icon vector data into C resources where feasible. Avoid a mandatory runtime browser/SVG engine.
- Accessibility: pluggable native/AccessKit adapter; make dependencies optional but build semantics into core from the start.
- Code editor syntax: optional Tree-sitter integration, with a useful fallback parser. If a rope is required, implement/test one or adopt a suitable C-compatible permissive library.
- Rich Markdown/HTML: safe, documented subset with optional dedicated parsers. Do not execute arbitrary HTML/JS; escape untrusted content.
- Charts: native primitives and headless plot scales; no browser canvas dependency.
- Audio/speech: optional OS speech-recognition capability adapter; expose honest 'unavailable' status where unsupported.
- Every third-party library gets a license entry and SPDX notice; run an automated license inventory.

**Source organization:** maintain modular `.c/.h` sources for contributors AND provide reproducibly generated one-header or few-header amalgamations in `dist/` with one-TU implementation macros (`CUI_IMPLEMENTATION`, `CUI_BASE_IMPLEMENTATION`, `CUI_COMPONENTS_IMPLEMENTATION`). Also support compiling as a conventional static library, with CMake and pkg-config. Header-only **distribution** does not prohibit compiled optional integrations. Test amalgamated headers in both single-TU and multi-TU applications and with GCC, Clang and MSVC-compatible C (where available). Avoid GNU-only extensions and accidental duplicate symbols.

## 5. Non-negotiable complete GPUI Kit 0.7.1 component inventory

**Implement every one of the 78 documented entries below.** The name inventory was taken from https://gpui-kit.com/component/ at GPUI Kit 0.7.1. Classify each as a styled component, infrastructure/provider, or optional platform-facing control according to its source docs; don't drop entries merely because they are not ordinary widgets. For every entry: inspect its upstream documented behavior, create an **original C** implementation, give it a public API, headless/base implementation where behavior warrants it, styled rendering, gallery/story, interaction tests, examples and docs. Model advanced features honestly, including performance and platform limitations.

### Catalog A — fundamental controls, content and feedback

1. Accordion
2. AlertDialog
3. Alert
4. Attachment
5. Avatar
6. Badge
7. Bubble
8. Button
9. Checkbox
10. Clipboard
11. Collapsible
12. DropdownButton
13. Empty
14. Focus Trap
15. GroupBox
16. HoverCard
17. Icon
18. Image
19. Kbd
20. Label
21. Marker
22. Message
23. MessageScroller
24. Notification
25. Pagination
26. Progress
27. Radio
28. Rating
29. Shimmer
30. Skeleton
31. Spinner
32. Stepper
33. Switch
34. Tag
35. Toggle
36. Tooltip

### Catalog B — forms and input

37. Calendar
38. ColorPicker
39. Combobox
40. DatePicker
41. Editor
42. Form
43. Input
44. Input Group
45. NumberInput
46. OtpInput
47. Questionnaire
48. Select
49. Slider
50. Speech
51. Textarea
52. TimeField

### Catalog C — layout, navigation and application shell

53. Carousel
54. DescriptionList
55. Dialog
56. Dock
57. List
58. Menu
59. Popover
60. Resizable
61. Root View
62. Scrollable
63. Settings
64. Sheet
65. Sidebar
66. StatusBar
67. Tabs
68. Theme
69. TitleBar
70. Toolbar
71. Tree
72. VirtualList

### Catalog D — advanced data, visualization and commands

73. Chart
74. Command
75. DataTable
76. Plot
77. Table
78. TextView

**Important:** These are functional families, not proof that a single component API should flatten every feature. Several contain many modes/subcomponents (e.g. Chart, Menu, Dock, Editor, TextView).

### GPUI Base parity — additional primitive/infrastructure requirements

The GPUI Base docs also expose primitives not separately named in the styled catalog. Implement the relevant reusable capabilities as base APIs even if they share styled wrappers: **Link, Nav Stack, Popup, Radio Group, Scrollbar, Toast, Toggle Group**, plus **Text Selection, Motion, History/UndoHistory, VirtualList internals, Dock data/model, Plot scales**, focus management, accessibility and semantic tokens. Cross-reference https://gpui-kit.com/base/ and the pinned code/docs. Do not count these as substituting for the 78 items above.

### Capability requirements for the most difficult components

- **Editor:** responsive large-file editor (target benchmark 200K lines, not a claim until tested); UTF-8 and grapheme-correct cursor/selection; multiline edit, undo/redo, indentation, line numbers/gutter, soft wrapping, find/replace, diagnostics and decorations, syntax highlighting (Tree-sitter optional), completion and hover hooks. Keep language-server interaction in an optional external callback API, not a mandatory LSP process dependency.
- **DataTable/Table:** column descriptors, sorting, resizing, pinned/fixed columns where applicable, keyboard/mouse cell selection, row selection, header/footer, virtualization for large datasets, screen-reader semantics, callbacks for remote data models.
- **Dock:** nested splits, tab groups, drag-reorder, resize, edge docks, serialization and restoration with versioned schema, focus and keyboard behavior.
- **VirtualList/MessageScroller:** variable-height item virtualization, stable keys, stick-to-bottom behavior for chat, efficient measuring, incremental updates, scroll restoration.
- **Chart/Plot:** independent plot scales/axes/ticks/legends/series/hover, line/bar/area/pie/candlestick where GPUI Kit documents them; zoom/selection as supported; readable labels and headless data models.
- **TextView:** safe rich text and selectable Markdown/HTML subset, hyperlinks, inline code, lists/tables where documented; Unicode-aware selection; no JavaScript execution.
- **Command/Combobox/Select/Menu:** filtering, active descendant, keyboard navigation, focus restore, selection and dismissal rules, nested submenu placement and collision detection.
- **Calendar/DatePicker/TimeField:** locale-aware presentation abstraction, keyboard access, min/max and validation, leap-year/time-zone pitfalls, screen-reader descriptions.
- **Speech:** interface supporting native device recognition where available; explicit optional platform support/permissions; functional tests using fake adapters. Never fake speech recognition.
- **Clipboard:** OS clipboard adapter plus headless test implementation; correct handling of missing capability.
- **Root View, Theme, Focus Trap:** these are real infrastructure/provider features, not empty wrappers.

Build catalog documentation as a **versioned component manifest** (JSON or YAML) at `docs/gpui-parity.json` with fields: `name`, `reference_url`, `category`, `base_behavior`, `public_header`, `sample`, `tests`, `status`, `platforms`, `limitations`, `owner`, `last_verified_commit`. Include status enum `not_started | scaffolded | implemented | tested | verified`. The nightly report must provide totals by status and actionable next steps. Add a CI check that every one of the 78 named entries exists exactly once in the manifest and that `verified` entries have working compiled examples and behavioral tests. Documentation count alone never implies functionality.

## 6. Public API and developer experience

Create an idiomatic ergonomic C11 API. Draft design docs and **compile real usage examples** before locking signatures. An illustrative goal (NOT authoritative compilable API):

```c
#define CUI_IMPLEMENTATION
#include "cui.h"
#define CUI_BASE_IMPLEMENTATION
#include "cui_base.h"
#define CUI_COMPONENTS_IMPLEMENTATION
#include "cui_components.h"

void show_settings(cui_context *ui, settings *s) {
    CUI_COLUMN(ui, /* gap/padding/options */) {
        cui_heading(ui, "Settings");
        cui_switch(ui, "notifications", &s->notifications);
        if (cui_button(ui, "save", "Save", CUI_BUTTON_PRIMARY)) {
            save_settings(s);
        }
    }
}
```

Design requirements:

- Readability comparable to declarative web composition, without fake JSX/CSS or a fragile macro jungle.
- Predictable type-safe-ish descriptors using C11 structs, enums, callbacks and designated initializers; succinct defaults.
- Compound components expose semantic parts and optionally render callbacks; embedding custom design systems must be possible without styled dependencies.
- State handles explicitly convey ownership and lifetime; stable IDs derive from scoped names/keys and support repeated lists.
- Error codes for IO/render failures, debug assertion/reporting hooks, tracing and optional UI inspector (hit regions, IDs, timing).
- UTF-8 contracts with lengths, not assumptions about zero-terminated strings.
- C ABI conventions suitable for external language bindings; demonstrate at least one small C++ and Zig/Rust/Go FFI consumer if practical.
- Clear blocking vs async callbacks and frame/lifetime boundaries; do not run user callbacks during unsafe traversal phases.
- Native-quality keyboard, focus and application command support.
- Host application can supply platform and renderer, or use `cui_app_sdl3_*` convenience startup.

Create `examples/hello_world`, `examples/unstyled`, `examples/styled_settings`, `examples/desktop_dashboard`, `examples/docking_ide`, `examples/data_table`, `examples/editor`, `examples/chat`, `examples/renderer_swap`, `examples/embedding`, `examples/multi_window` where implemented. The examples must build through CI and be real, not screenshots of an unrelated UI.

## 7. Repository layout

Use this as a strong starting point; adjust only if a clearly documented reason exists:

```text
cui/
  README.md
  LICENSE
  CONTRIBUTING.md
  CODE_OF_CONDUCT.md
  SECURITY.md
  CHANGELOG.md
  AGENTS.md
  CMakeLists.txt
  CMakePresets.json
  Makefile
  .github/workflows/
  cmake/
  include/cui/
    cui_shell.h           # optional JS extension host; not included by default
    cui.h
    cui_base.h
    cui_components.h
    cui_theme.h
    cui_accessibility.h
    cui_renderer.h
    cui_platform.h
  src/
    core/
    base/
    components/
    text/
    theme/
    accessibility/
    shell/                # optional extension host
  backends/
    sdl3/
    sdl_gpu/
    sdl_renderer/
    sokol/
    software/
    native/
    mobile/
    wasm/
  third_party/       # pinned sources/metadata/licenses as appropriate
  assets/            # original permissively licensed icons/fonts/demo data
  tools/
    amalgamate/
    component_vendor/
    visual_test/
  dist/              # generated distributable public headers
  tests/
    unit/
    integration/
    headless/
    visual/
    fuzz/
    benchmarks/
  examples/
  gallery/           # native Storybook-like live component showcase
  website/           # documentation website and examples
  docs/
    ARCHITECTURE.md
    DECISIONS.md
    DEPENDENCIES.md
    PLATFORM_MATRIX.md
    API.md
    COMPONENT_AUTHORING.md
    THEME_GUIDE.md
    ACCESSIBILITY.md
    PERFORMANCE.md
    ROADMAP.md
    MAINTAINER_ACTIONS.md
    upstream-baseline.md
    gpui-parity.json
    swarm-status.md
  scripts/
```

`dist/` must be reproducible from modular sources; CI fails if generated headers differ from checked-in generated outputs. Dependencies/versions pinned. Make all core test targets runnable in headless CI.

## 8. Agent organization and concurrency discipline

Delegate aggressively **after** a short architecture-contract phase. Recommended worker streams:

1. **Architect/integrator (you):** freeze interfaces, component inventory, dependency boundaries, build structure, conflict resolution, integration, release gating and GitHub operations.
2. **Core/runtime worker:** Clay adapter, lifecycle, IDs, input/events, focus, invalidation, state and memory contracts.
3. **Graphics/platform worker:** renderer vtable, SDL3 windows/events, SDL_GPU GPU backend, headless renderer, later Sokol/GLES and mobile.
4. **Text/semantics worker:** shaping, font caches, Unicode selection, text editing engine, IME, accessibility semantic tree and adapters.
5. **Base-behavior worker:** unstyled interaction primitives, overlays, popup/dialog/menu mechanics, controlled state, navigation/history, motion.
6. **Styled-controls worker:** tokens, theme engine, basic/form controls and consistent component APIs.
7. **Advanced-components worker:** data tables, virtualization, docking, tree, editor, charts/plots, Markdown/TextView; may split by subsystem once base APIs exist.
8. **Quality/docs/gallery worker:** native component gallery, headless/visual tests, docs site, cross-platform CI, benchmarks, licensing and parity tracker.

Use **git worktrees or isolated branches per worker**, nonoverlapping module ownership, brief interface contracts (`docs/INTERFACES.md`) and small scoped tasks with explicit acceptance tests. Only the integrator merges into the default branch. Do not let workers rewrite each other's public APIs uncoordinated. Do not create hundreds of tiny agents, conflicting edits or endlessly parallel draft implementations.

### Initial synchronization barrier

Before workers implement against shared interfaces:

1. Inspect toolchain and installed dependencies; check GitHub state safely.
2. Create baseline docs and the complete 78-entry parity manifest.
3. Agree on core types, ownership rules, renderer/platform API, events and the build graph.
4. Commit this small but real skeleton, with a hello-world/example compile gate and headless test harness.
5. Unblock parallel workers against stable interfaces; request proposal/ADR for incompatible changes.

### Continuous integration loop

- Each worker submits changes + test logs + public API impacts + limitations.
- Integrator compiles mainline and runs smoke tests after each merge; resolve breakages immediately rather than accumulating them.
- Every 30–60 minutes of unattended execution, checkpoint progress and update `docs/swarm-status.md`; work onward.
- If a tool, backend, OS or dependency is unavailable, build a meaningful fallback/mock only for **testing contracts** and record unverified platform claims honestly. Do not stall the entire swarm.
- If tokens, time or resources run low, favor completing **working vertical slices** over sprawling half-written code. Preserve full backlog and ownership.

## 9. Sequenced implementation plan (do not stop at the first phase)

### Phase 0: discovery and bootstrap

- Audit GPUI Kit 0.7.1 component/base docs and relevant source; record parity feature subtasks per entry.
- Validate Clay API/version and licenses, choose pinned dependencies, repo creation, CI skeleton, tooling docs.
- Create `AGENTS.md` with workflow, code style, ownership, test commands, safety and release rules.

### Phase 1: minimum real framework

- Running SDL3 window with **hardware-accelerated** 2D rendering, a headless renderer, Clay-based UI composition, input, hit testing, predictable frame lifecycle, redraw and resizing.
- Reusable IDs/state, focus infrastructure, text measurement and a usable font renderer.
- Compiling public header-only demo plus conventional static library demo.
- Basic button/checkbox/input/label/containers working with real mouse and keyboard behavior.

### Phase 2: base + design system

- Complete behavior/style separation and runtime themes.
- Inputs, edit/clipboard/IME integration, accessibility semantics; dialogs, menus, focus scopes, popovers, select and combobox.
- Live native gallery and functional dark/light themes; two visually distinct styles built on same base behaviors to prove isolation.

### Phase 3: all 78 catalog entries

- Advance every catalog item from `not_started` to `verified` as implemented and tested.
- Prioritize highly reused controls, then compound controls and advanced systems. Do not substitute fake controls for complicated behavior.
- Finish advanced features (dock, data table, virtual list, code editor, plot/chart, rich text, speech adapters, etc.) with independent tests and examples.

### Phase 4: renderer/platform breadth

- SDL_GPU renderer, separate Sokol GPU backend, software/headless backend, optional SDL accelerated-2D fallback.
- Native build/smoke coverage on Linux X11 and Wayland, Windows and macOS where runners exist.
- Android NDK/iOS wrappers and actual device/simulator validation when infrastructure supports it, with touch, safe areas, keyboard, lifecycle and signing guidance.
- WASM builds and browser-host integration (not a browser runtime) when portable enough.
- Per-platform `verified / builds_only / untested / unsupported` evidence tracked in `docs/PLATFORM_MATRIX.md`.

### Phase 5: optional extensions and developer ecosystem / release quality

- Polished native story/gallery for all 78 components and a static docs website.
- Optional CUI Shell JavaScript extension host with explicit capability grants and a real example; no mandatory scripting runtime in ordinary CUI applications.
- Component source vendoring tool and theme generator/loader, examples, high-quality migration and integration docs, stable one-TU distribution.
- Benchmarks, fuzzing, security reviews, accessible interaction tests, demo binary sizes and cold-start measurements.
- First alpha release when it truly meets documented alpha criteria; no invented production-ready claims.

Parallelize where interfaces permit. Revisit incomplete phases throughout the run. Full parity remains the explicit long-term finish condition.

## 10. Quality gates and measurable acceptance

Set up `cmake --build`, `ctest`, `make check`, `make lint`, `make format-check`, `make gallery`, `make bench` and `make dist-check` (or clearly documented equivalents), and run as much as the environment permits.

### Correctness & safety

- Clean GCC/Clang builds in C11, with `-Wall -Wextra -Wpedantic` and a warning policy; CI test MSVC/clang-cl as feasible.
- AddressSanitizer + UndefinedBehaviorSanitizer on Linux; test no frame-lifetime UAF, leaks in common flows, no off-by-one in UTF-8 and no double-frees.
- Unit tests for stable IDs, event ordering, hit testing, focus, navigation, pointer capture, popup placement, animations, keyboard semantics, color token overrides and serialization.
- Test nested scrolling, overlay clipping, DPI changes, empty/malformed input, input validation, unicode/RTL/IME, long strings, huge lists and tiny windows.
- Property/fuzz tests for parser/data model and layout edge cases where practical.
- Deterministic headless screenshots or draw-list golden tests, with explicit per-backend tolerance.
- Robust handling of device loss/fallback, missing fonts, clipboard unavailability and OS capability failures.

### Performance

- Provide benchmarks for startup time, peak/steady memory, command counts, CPU layout time, frame time, GPU upload/flush time, frame pacing, 1K/10K/100K list items and 200K-line editor input (where implemented).
- Target responsive 60 Hz UI and enable 120 Hz where the device/frame workload permits. Treat frame budgets (~16.7 ms at 60Hz, ~8.3 ms at 120Hz) as targets, not automatic promises. Measure and publish the hardware, driver, build mode and workload.
- Redraw only dirty content where possible, avoid resource churn; do not assert C is faster than Flutter/GPUI without a reproducible apples-to-apples benchmark.
- Define regression thresholds once baselines exist; report actual outcomes even if poor.

### Accessibility

- Semantic role/name/value/state/action for every interactive control; accessible focus order and keyboard interaction independent of theme.
- Headless semantics assertions in CI. Document actual Windows UIA/macOS AX/Linux AT-SPI/Android/iOS platform adapter coverage, without presenting missing bridges as completed.
- High contrast, reduced motion, font scaling and touch targets in theme/accessibility guidance.

### Definition of DONE for a catalog entry

A component can be marked **verified** only if:

1. Its API compiles from installed/public headers and a realistic example uses it.
2. It actually renders or supplies substantive headless infrastructure on at least one supported target.
3. Core interactions are tested (mouse, keyboard, controlled state, focus, and touch where relevant).
4. Its styled implementation uses the base behavior without duplicating interaction logic.
5. Its theme variants, disabled/loading/error states as applicable, and accessibility semantics are demonstrated.
6. Documentation includes public API, usage, screenshots/story where visual, limitations and platform support.
7. The gallery and tests genuinely run in CI and the status manifest references them.

For heavyweight components, subdivide features in the manifest; a toy multiline text box is **not** a verified code editor, a regular scroll panel is **not** a virtual list, and a row of rectangles is **not** a verified data table.

## 11. Cross-platform CI expectations

Create GitHub Actions jobs with appropriate availability/permissions:

- Linux Ubuntu GCC + Clang: core, SDL3, headless, sanitizers, Xvfb as available.
- Linux Wayland smoke (e.g., Weston headless where workable) separately from X11; don't infer one from the other.
- Windows MSVC/clang-cl build + GUI runner smoke.
- macOS Clang + Metal/SDL integration smoke.
- Android NDK cross-compile and packaging skeleton; real device tests marked separately.
- iOS cross-compile with supported Apple runner (simulator capability caveats documented), packaging skeleton and device testing steps.
- WebAssembly C build and browser test where infrastructure allows.
- Formatting, dependency/license scan, header amalgamation diff, example builds, component manifest completeness, security scans for vendored libraries.

No CI job should require a personal API token to build ordinary code; use `GITHUB_TOKEN` scoped minimally for actions that need it. Skip unavailable GPU-device testing honestly rather than turning critical test jobs into fake passes.

## 12. Documentation and product polish

Deliver:

- Strong `README.md`: positioning, features honestly labeled, 60-second hello world, dependency profiles, platform matrix, screenshots from **actual CUI builds**, not borrowed imagery.
- Architecture and ABI docs; comprehensive memory ownership / frame lifetimes / threading rules.
- Theme guide and two distinct example design systems demonstrating that base behavior is unstyled.
- `docs/COMPONENT_AUTHORING.md`: how to write a custom styled component from CUI Base.
- Component reference for all 78 entries, status-visible, with code examples and interactions.
- Native gallery component-by-component, searchable/category navigation, toggles for themes/density/contrast and live interaction demonstration.
- Troubleshooting guide for SDL3, fonts, GPUs, Wayland and Mac/Windows builds.
- Original assets and clear third-party provenance; do not impersonate GPUI Kit or copy its brand.
- Contribution guidelines, security policy, changelog and semantic versioning plan.

Do not spend most of the overnight session on a marketing site while the core library doesn't run.

## 13. GitHub/repo discipline, unattended execution safety

- Run `pwd`, `git status`, `git remote -v`, `gh auth status`, installed compiler/build checks first.
- If a repo exists under `mattmezza/cui`, inspect it. If unknown contents conflict, create a new feature branch/worktree rather than replacing history.
- Default to a **private** repository if creating a new one; never make it public without my explicit request. Keep sources and licenses organized to support eventual open sourcing.
- Use conventional commits or similarly informative atomic history; push small verified checkpoints, not only a single final dump. Integrate into a safe branch until build gates pass; open a PR if permissions/workflow demand it.
- Never execute destructive cleanup of unrelated dirs, force-push, rewrite main, leak tokens, disable firewalls, install privileged packages or bypass auth.
- Dependencies requiring `sudo`, Xcode signing, interactive credentials or real mobile devices go into `docs/MAINTAINER_ACTIONS.md` with exact reproducible steps; keep implementing everything else unattended.
- If `gh` not authenticated, work locally and leave a clean git repo + instructions; never block.
- Commit the current state before leaving it or switching tasks, and preserve complete test results.

## 14. Nightly checkpoint: what I must find tomorrow morning

Maintain an incrementally updated `docs/swarm-status.md` and create `docs/NIGHTLY_REPORT.md` at end of available run (and update it each checkpoint so sudden interruption still leaves useful status). Include:

1. **Repository URL, current branch and last commit SHA**, plus whether pushes succeeded.
2. High-level architecture implemented, major ADR decisions and dependency versions.
3. **Exact number of the 78 catalog entries** in each status; list all entries still incomplete and state why, including any base-only primitives.
4. Builds/tests actually run, commands, pass/fail/skipped, and evidence of renderer/backend used; do not confuse compilation with runtime verification.
5. Screenshots / video only if captured from actual running CUI examples; otherwise say none.
6. Performance numbers only if measured, with machine/build/hardware context.
7. Platform validation matrix: Linux X11, Linux Wayland, Windows, macOS, Android, iOS, WASM.
8. Unresolved architectural/design risks and security/license issues.
9. Prioritized next tasks with links to tracked issues/files; assignments or remaining component batches.
10. **Minimal commands I can run immediately tomorrow** to build, launch the gallery, run tests and inspect progress.
11. Any manual actions requiring my involvement, especially credentials, Apple certificates or missing package installations.

The report MUST use truth-preserving status words. Don't write “complete” if the renderer only builds or the UI is visually simulated. A smaller real implementation is preferable to an enormous imaginary one.

## 15. Decision-making rules

- Optimize for a **working framework** and a coherent architecture, not line count.
- Public C API + renderer/platform vtables + semantic core are **contracts**; stabilize them early and communicate changes before worker implementations diverge.
- Build the core and a vertical slice first, then scale parallel implementation across the complete catalog; use CI to keep integration green.
- Prefer mature permissive dependencies over reinventing international text, image codecs, OS integration or GPU abstraction; use optional adapters to preserve a minimal core.
- Make deliberate reversible tradeoffs and record ADRs. No huge single source file merely because the distribution is header-only: source should be modular; amalgamations generated.
- Portability claims require evidence. GPU claims require driver/backend detection. Accessibility claims require semantic plus platform bridge evidence.
- Do not ask me to narrow the requested component scope. It is explicitly **all 78 GPUI Kit 0.7.1 documented entries**, plus reusable GPUI Base primitives. If you cannot finish all during this unattended session, keep the entire catalog in scope, ship tested partial progress and an exact backlog.
- No unnecessary status chatter or requests for approval. Continue independently, checkpoint, and only stop when unable to make safe, substantive progress.

## 16. Start executing now

Your FIRST actions should be:

1. Discover available tools and compiler environment safely.
2. Inspect or create `mattmezza/cui` without damaging existing work.
3. Pin GPUI Kit 0.7.1's 78-entry catalog and Clay/dependency versions in the repo; create complete parity manifest.
4. Write and review the compact architecture + API contract, create the build and test scaffold and a genuinely running Clay → renderer → SDL3 vertical slice.
5. Delegate nonoverlapping implementations to workers with precise acceptance contracts.
6. Integrate, run tests, push safe checkpoints, and continue toward full component parity.
7. Keep the nightly report current throughout your unattended execution.

**Begin repository work immediately. Do not respond with a plan instead of doing it.**
