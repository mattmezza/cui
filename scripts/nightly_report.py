#!/usr/bin/env python3
"""Refresh truth-preserving checkpoint from the inventory and saved evidence."""
import collections,json,subprocess
from pathlib import Path
r=Path(__file__).resolve().parents[1]
def git(*args):return subprocess.check_output(['git',*args],cwd=r,text=True).strip()
m=json.loads((r/'docs/gpui-parity.json').read_text(encoding="utf-8"))['components'];counts=collections.Counter(e['status'] for e in m)
sha=git('rev-parse','HEAD');branch=git('branch','--show-current')
checkpoint_path=r/'docs/validation/latest-checkpoint.json'
checkpoint=json.loads(checkpoint_path.read_text(encoding='utf-8')) if checkpoint_path.exists() else {}
validation_summary=checkpoint.get('summary','See the explicitly scoped source commits in saved validation logs; later source changes require fresh checks.')
lines=['# Nightly report — live development checkpoint','',f'Repository: https://github.com/mattmezza/cui. Branch `{branch}`. Evidence/source commit before this report update: `{sha}`. See swarm-status.md for branch promotion and push/CI observations.','',
'## Working architecture','',
'Explicit C11 contexts, pinned Clay layout, stable scoped IDs, bounded frame strings, ordered previous-frame hit routing, capture/focus/modal scopes, independent semantics and draw lists. Unstyled controlled primitives feed theme-driven components. UTF-8 scalar editing with bounded undo/redo, retained keyed Fenwick virtualization, split/tab docking with transactional persistence and plot scales/clipped geometry are substantive headless models. Optional FreeType/HarfBuzz shapes runs. Software, SDL_Renderer compatibility, SDL_GPU Vulkan and independent Sokol GLCORE are separate renderers; actual hardware/runtime selection is reported.','',
'GPUI Kit baseline 0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`; Clay e6cc36941ab2af5d81107617039d6f527a1c660b. See upstream-baseline.md and DECISIONS.md. Source code and generated one/few headers are tested independently.','',
'## Catalog counts','',
'| Status | Families |','|---|---:|',*[f'| {s} | {counts[s]} |' for s in ['not_started','scaffolded','implemented','tested','verified']],'',
'Every non-verified family remains incomplete; tested means only its documented subset has tests. **No full family is verified.** Full feature backlog: [gpui-parity.json](gpui-parity.json), [COMPONENTS.md](COMPONENTS.md).','',
'## Validation evidence','',
validation_summary,'',
'Latest stored logs (including the preserved initial WASI failure and corrected rerun):','',
*[f'- [{p.name}](validation/{p.name})' for p in sorted((r/'docs/validation').glob('*.txt'))],'',
'Commands: `cmake -S . -B build -G Ninja -DCUI_GPU_TESTS=ON`; `cmake --build build`; `ctest --test-dir build --output-on-failure`. ASan/UBSan config uses `-DCUI_SANITIZE=ON`; portable Clang 21.1.8 is supplied by Zig 0.16.0 with `-DCMAKE_LINK_DEPENDS_USE_LINKER=FALSE` to avoid a Zig/CMake linker dependency-file crash. This workaround is not required by ordinary GCC builds.','',
'Linux Xorg actual runtime: SDL_Renderer OpenGL; SDL_GPU Vulkan reported Intel(R) Graphics (ARL), Intel open-source Mesa driver. Actual Vulkan and Sokol GLCORE pixel readback verifies nested clip/alpha/geometry; Sokol text cache/readback/eviction passed under ASan/UBSan. SDL dummy/software tests verify cache/clipboard/event service contracts; dummy is not OS platform evidence. C++ and Zig C-ABI consumer executables ran. Portable wasm32-wasi behavioral/embedding modules ran in pinned Wasmtime. Real Chromium browser tests separately exercise the WebAssembly gallery, including pointer/keyboard behavior, text input, popups and mobile-width bounds. These are separate validation paths. Native font/cache tests require a discovered real font.','',
'## Captures and performance','',
'Actual generated frames are in [screenshots](screenshots/): light gallery, dark text preview, narrow values, tiny-window resize fallback, SDL_GPU and Sokol. They are not borrowed upstream imagery. The browser gallery has also been captured from actual running CUI WebAssembly; no component mockups stand in for runtime evidence. No video captured.','',
'CPU layout benchmark initial default unoptimized build on Intel Core Ultra 5 235U/Arch/GCC 16.2.1:500 frames × 100 boxes, 0.196 ms/frame CPU clock, 28000 visible commands after culling. This excludes shaping/rendering and is not a GPU frame or competitor benchmark. Keyed virtualization measurements/workload/memory are in VIRTUALIZATION.md. Reproduce with `make bench` and `./build/test_virtual --benchmark`.','',
'## Platform matrix','',
'Linux Xorg and headless: tested partial. Wayland: untested. Windows/macOS: hosted portable C suites run; results and coverage limits are in the platform matrix. Android/iOS: untested. Browser WASM: tested partial in Chromium with an optional Canvas/input host, independently of headless WASI. See [PLATFORM_MATRIX.md](PLATFORM_MATRIX.md) for current observed CI changes. SDL_GPU currently SPIR-V/Vulkan only; Metal/D3D shaders pending. Native screen-reader bridges absent.','',
'## Risks and next tasks','',
'1. Single-line Input now has caret/selection painting, scalar pointer placement, scrolling, host clipboard and bounded history. Complete multiline presentation, IME, graphemes, shaping clusters, paragraph bidi and font fallback before claiming Editor.','2. Nested/context menus, multiple selection, custom row delegates, pickers and disclosure/tabs have tested styled slices. Complete reference-specific behavior; dialogs, alerts and sheets now have tested styled composition and nested focus, with advanced surface behavior pending.','3. Tree, table and scroll widgets now use clipped virtual viewports and keyboard models. Complete reference-default audits, variable heights and advanced integration; table pinning/multiple selection and grouped headers now have tested implementations; Dock and Plot remain partial model stories.','4. Complete independent renderer/resource lifecycle/device-loss contracts, broader GPU shader formats, OS accessibility and native platform adapters.','5. Implement remaining catalog families against feature requirements, then add real stories/behavior/semantic/theme tests before verified status.','',
'Licensing: original code MIT, Clay and Sokol zlib; optional renderer dependencies retain their own inventory/license. No GPUI code/runtime/branding copied. In-process capability checks would not be a security sandbox; no extension host yet.','',
'## Browser gallery','', '[Open the GitHub Pages gallery](https://matteo.merola.co/cui/). All 78 entries remain navigable; only implemented stories have live previews. Models are marked as partial and no family is promoted to verified by a screenshot. Deployment is gated by Chromium behavior tests and preserves the existing account domain.', '', 'Build with `python scripts/build_web.py` using pinned Zig 0.16.0, then serve `build/web` over HTTP. See WEB_GALLERY.md for dependency and browser-test commands.', '', '## Local commands','',
'```sh','cmake -S . -B build -G Ninja','cmake --build build','ctest --test-dir build --output-on-failure','./build/cui_gallery','./build/cui_gpu_demo --font /path/to/font.ttf','./build/cui_sokol_demo --smoke 3','make lint format-check dist-check bench','python scripts/check_manifest.py','```','',
'No dependency installation needed on the present host. Optional system CMake/Clang and Weston commands are in MAINTAINER_ACTIONS.md; no sudo or password prompts were used unattended. CI uses hosted-runner setup, not a personal token for builds.','',
'## Every incomplete family','',
'| Family | Current partial status | Remaining work |','|---|---|---|']
for e in m:
 if e['status']!='verified':lines.append(f"| {e['name']} | {e['status']} | {'; '.join(e['limitations']).replace('|','/')} |")
lines += ['','## Publishing/CI checkpoint','','Safe branch pushes are performed with ordinary Git; no force push/history rewrite. This report records the evidence commit above. Integrator updates publishing/CI observations in swarm-status.md and PLATFORM_MATRIX.md after each push.']
(r/'docs/NIGHTLY_REPORT.md').write_text('\n'.join(lines)+'\n', encoding='utf-8', newline='\n')
print('Updated nightly report:',sha,dict(counts))
