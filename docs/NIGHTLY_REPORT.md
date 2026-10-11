# Nightly report — live development checkpoint

Repository: https://github.com/mattmezza/cui. Branch `build/initial-framework`. Evidence/source commit before this report update: `2e6917097e86999be73a1b35ea14ee3e326b489d`. Sources are on the safe integration branch; see push/CI notes below.

## Working architecture

Explicit C11 contexts, pinned Clay layout, stable scoped IDs, bounded frame strings, ordered previous-frame hit routing, capture/focus/modal scopes, independent semantics and draw lists. Unstyled controlled primitives feed theme-driven components. UTF-8 scalar editing with bounded undo/redo, retained keyed Fenwick virtualization, split/tab docking with transactional persistence and plot scales/clipped geometry are substantive headless models. Optional FreeType/HarfBuzz shapes runs. Software, SDL_Renderer compatibility, SDL_GPU Vulkan and independent Sokol GLCORE are separate renderers; actual hardware/runtime selection is reported.

GPUI Kit baseline0.7.1 commit87d10ae5e1299d1be18670a37570c5402f952e34; Clay e6cc36941ab2af5d81107617039d6f527a1c660b. See upstream-baseline.md and DECISIONS.md. Source code and generated one/few headers are tested independently.

## Catalog counts

| Status | Families |
|---|---:|
| not_started | 59 |
| scaffolded | 0 |
| implemented | 4 |
| tested | 15 |
| verified | 0 |

Every non-verified family remains incomplete; tested means only its documented subset has tests. **No full family is verified.** Full feature backlog: [gpui-parity.json](gpui-parity.json), [COMPONENTS.md](COMPONENTS.md).

## Validation evidence

Latest stored logs (each contains the actual test result, not assumed success):

- [clang-zig-headless.txt](validation/clang-zig-headless.txt)
- [gcc-xorg.txt](validation/gcc-xorg.txt)
- [sanitizers.txt](validation/sanitizers.txt)
- [sokol-asan-xorg.txt](validation/sokol-asan-xorg.txt)

Commands: `cmake -S . -B build -G Ninja -DCUI_GPU_TESTS=ON`; `cmake --build build`; `ctest --test-dir build --output-on-failure`. ASan/UBSan config uses `-DCUI_SANITIZE=ON`; portable Clang21.1.8 is supplied by Zig0.16.0 with `-DCMAKE_LINK_DEPENDS_USE_LINKER=FALSE` to avoid a Zig/CMake linker dependency-file crash. This workaround is not required by ordinary GCC builds.

Linux Xorg actual runtime: SDL_Renderer OpenGL; SDL_GPU Vulkan reported Intel(R) Graphics (ARL), Intel open-source Mesa driver. Actual Vulkan and Sokol GLCORE pixel readback verifies nested clip/alpha/geometry; Sokol text cache/readback/eviction passed under ASan/UBSan. SDL dummy/software tests verify cache/clipboard/event service contracts; dummy is not OS platform evidence. C++ and Zig C-ABI consumer executables ran. Native font/cache tests require a discovered real font.

## Captures and performance

Actual generated frames are in [screenshots](screenshots/): light gallery, dark text preview, narrow values, tiny-window resize fallback SDL_GPU and Sokol. They are not borrowed upstream imagery. Fresh gallery review scored both material narrow/tiny fixes resolved; approval is bounded to the development gallery. No video captured.

CPU layout benchmark initial default unoptimized build on Intel Core Ultra5 235U/Arch/GCC16.2.1:500frames×100boxes,0.196ms/frame CPU clock,28000visible commands after culling. This excludes shaping/rendering and is not aGPUframe or competitor benchmark. Keyed virtualization measurements/workload/memory are in VIRTUALIZATION.md. Reproduce with `make bench` and `./build/test_virtual --benchmark`.

## Platform matrix

Linux Xorg and headless: tested partial. Wayland: untested. Windows/macOS: hosted portable C suites run; results and outstanding failures are in the platform matrix. Android/iOS/browser WASM: untested; WASI compilation is separate from browser integration. See [PLATFORM_MATRIX.md](PLATFORM_MATRIX.md) for current observed CI changes. SDL_GPU currently SPIR-V/Vulkan only; Metal/D3D shaders pending. Native screen-reader bridges absent.

## Risks and next tasks

1. Input is a preview: scalar boundaries, no painted caret/selection/pointer placement, IME, clipboard editor path, graphemes, paragraph bidi or font fallback. Finish this usable foundation before claiming Editor.
2. Modal reducers/focus scopes and popup geometry are base-only; styled dialog/menu/select/combobox/portal semantics pending.
3. Virtualization is a real efficient keyed model, but a scroll/focus/widget layer and data table remain. Dock/advanced model progress must retain widget-level gaps.
4. Complete independent renderer/resource lifecycle/device-loss contracts, broader GPU shader formats, OS accessibility and native platform adapters.
5. Implement remaining catalog families against feature requirements, then add real stories/behavior/semantic/theme tests before verified status.

Licensing: original code MIT, Clay and Sokol zlib; optional renderer dependencies retain their own inventory/license. No GPUI code/runtime/branding copied. In-process capability checks would not be a security sandbox; no extension host yet.

## Morning commands

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
| Accordion | not_started | Full documented behavior pending implementation and audit. |
| AlertDialog | not_started | Full documented behavior pending implementation and audit. |
| Alert | not_started | Full documented behavior pending implementation and audit. |
| Attachment | not_started | Full documented behavior pending implementation and audit. |
| Avatar | not_started | Full documented behavior pending implementation and audit. |
| Badge | implemented | Themed text badge renders. Count/dot/overflow and standalone semantic tests pending. |
| Bubble | not_started | Full documented behavior pending implementation and audit. |
| Button | tested | Activation, focus and disabled states tested; variants/sizes/icons/loading/error/slots remain incomplete. |
| Checkbox | tested | Binary and mixed base state tested; styled mixed state, real check geometry and validation states pending. |
| Clipboard | tested | SDL native/dummy roundtrip and optional capability adapter tested; core input paste/cut integration pending. |
| Collapsible | not_started | Full documented behavior pending implementation and audit. |
| DropdownButton | not_started | Split button/menu functionality pending implementation and audit. |
| Empty | not_started | Full documented behavior pending implementation and audit. |
| Focus Trap | tested | Core nested modal scopes and overlay state tested. No OS accessibility bridge or full modal relationship semantics. |
| GroupBox | not_started | Full documented behavior pending implementation and audit. |
| HoverCard | not_started | Full documented behavior pending implementation and audit. |
| Icon | not_started | Full documented behavior pending implementation and audit. |
| Image | not_started | Full documented behavior pending implementation and audit. |
| Kbd | not_started | Full documented behavior pending implementation and audit. |
| Label | implemented | Text wrapper renders, but control association and standalone semantic labeling pending. |
| Marker | not_started | Full documented behavior pending implementation and audit. |
| Message | not_started | Full documented behavior pending implementation and audit. |
| MessageScroller | not_started | Full documented behavior pending implementation and audit. |
| Notification | not_started | Full documented behavior pending implementation and audit. |
| Pagination | not_started | Full documented behavior pending implementation and audit. |
| Progress | implemented | Determinate bar renders/clamps. Indeterminate motion, semantic values and dedicated story tests pending. |
| Radio | tested | Selection/exclusivity tested; full radio-group roving focus and arrow navigation pending. |
| Rating | not_started | Full documented behavior pending implementation and audit. |
| Shimmer | not_started | Full documented behavior pending implementation and audit. |
| Skeleton | not_started | Full documented behavior pending implementation and audit. |
| Spinner | not_started | Full documented behavior pending implementation and audit. |
| Stepper | not_started | Full documented behavior pending implementation and audit. |
| Switch | tested | Controlled activation tested; native switch geometry, theme variants and touch pending. |
| Tag | not_started | Full documented behavior pending implementation and audit. |
| Toggle | tested | Controlled press state tested; toggle groups and full visual states pending. |
| Tooltip | not_started | Full documented behavior pending implementation and audit. |
| Calendar | tested | Base Gregorian date arithmetic only. No calendar widget, range selection, locale or keyboard grid. |
| ColorPicker | not_started | Full documented behavior pending implementation and audit. |
| Combobox | not_started | Full documented behavior pending implementation and audit. |
| DatePicker | not_started | Full documented behavior pending implementation and audit. |
| Editor | implemented | Reusable bounded UTF-8 scalar editing model exists. No source editor, gutter, highlighting, diagnostics, grapheme editing or200K-line validation. |
| Form | not_started | Full documented behavior pending implementation and audit. |
| Input | tested | Real UTF-8 scalar editor with bounded undo/redo; no caret/selection painting, graphemes, IME, clipboard integration or pointer position. |
| Input Group | not_started | Full documented behavior pending implementation and audit. |
| NumberInput | not_started | Full documented behavior pending implementation and audit. |
| OtpInput | not_started | Full documented behavior pending implementation and audit. |
| Questionnaire | not_started | Full documented behavior pending implementation and audit. |
| Select | not_started | Full documented behavior pending implementation and audit. |
| Slider | tested | Capture drag, keyboard step/clamp and disabled tests pass; tick/handle geometry and touch pending. |
| Speech | not_started | Full documented behavior pending implementation and audit. |
| Textarea | not_started | Full documented behavior pending implementation and audit. |
| TimeField | not_started | Full documented behavior pending implementation and audit. |
| Carousel | not_started | Full documented behavior pending implementation and audit. |
| DescriptionList | not_started | Full documented behavior pending implementation and audit. |
| Dialog | not_started | Full documented behavior pending implementation and audit. |
| Dock | tested | Retained split/tab tree, stable keys, constrained geometry and transactional versioned persistence tested. Styled Dock, drag/drop, resize gestures, floating windows and keyboard/focus integration pending. |
| List | not_started | Full documented behavior pending implementation and audit. |
| Menu | not_started | Full documented behavior pending implementation and audit. |
| Popover | tested | Base collision placement and popup dismissal reducer only. No styled portal/anchor UI or semantic relationship. |
| Resizable | not_started | Full documented behavior pending implementation and audit. |
| Root View | not_started | Full documented behavior pending implementation and audit. |
| Scrollable | not_started | Full documented behavior pending implementation and audit. |
| Settings | not_started | Full documented behavior pending implementation and audit. |
| Sheet | not_started | Full documented behavior pending implementation and audit. |
| Sidebar | not_started | Full documented behavior pending implementation and audit. |
| StatusBar | not_started | Full documented behavior pending implementation and audit. |
| Tabs | not_started | Full documented behavior pending implementation and audit. |
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
