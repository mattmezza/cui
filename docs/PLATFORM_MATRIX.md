# Platform evidence

| Target | Status | Evidence and limits |
|---|---|---|
| Linux Xorg | tested partial | Native SDL_Renderer OpenGL, independent Sokol GLCORE and SDL_GPU Vulkan on Intel Graphics; actual frames/readbacks, pointer/key model and clipboard tests. No screen-reader/IME bridge. |
| Linux headless | tested partial | GCC C11/software renderer, behavioral suites, ASan/UBSan, SDL dummy services. |
| Linux Wayland | untested | SDL supports host integration; no compositor smoke run yet. Xorg is not Wayland evidence. |
| Windows | tested portable subset | Hosted MSVC:22/22 core/software checks passed on CI run38219053307, including encoding/hash/distribution gates. Native GUI untested. |
| macOS | tested portable subset | Hosted Apple Clang:22/22 core/software checks passed on [CI run38219053307](https://github.com/mattmezza/cui/actions/runs/38219053307). Native GUI/Metal untested. |
| Browser WASM | tested partial | Actual Chromium runs CUI WebAssembly with Canvas/input host, including pointer/keyboard, scalar input, popup focus/Escape, themes and 390px bounds. See [browser gallery](WEB_GALLERY.md). Native text shaping/OS accessibility remain separate. |
| Android | untested | No wrapper/device validation. |
| iOS | untested | No wrapper/simulator/device validation/signing. |
| WASM/WASI | tested headless WASI subset | Zig0.16/Wasmtime49 portable behavioral/embedding suites compile and run; current source/counts are recorded in validation logs; see [WASI.md](WASI.md). |

Optional unavailable integrations are explicit. Compilation will be recorded separately from runtime tests. SDL_GPU SPIR-V shader support currently confines that adapter to Vulkan-capable devices; D3D12/Metal formats are pending. Sokol GLCORE is tested on Linux Xorg; its other backends and platforms are pending.

Linux hosted GCC and Clang each passed25/25 native/sanitizer checks plus Xvfb gallery smoke and Sokol pixel/cache tests. The CI GL driver reported **llvmpipe**, a software driver; this is API/readback coverage and is separate from the actual Intel/Mesa hardware evidence on the development Xorg host. Saved [previous CI summary](validation/ci-01865d5.txt) and [resumed job metadata](validation/ci-8d67069.json) record exact source and outcomes.
