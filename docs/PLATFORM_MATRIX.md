# Platform evidence

| Target | Status | Evidence and limits |
|---|---|---|
| Linux Xorg | tested partial | Native SDL_Renderer OpenGL, independent Sokol GLCORE and SDL_GPU Vulkan on Intel Graphics; actual frames/readbacks, pointer/key model and clipboard tests. SDL composition/candidate-rectangle forwarding has injected service tests; real OS IME/candidate-window acceptance and native screen readers remain unverified. |
| Linux headless | tested partial | GCC C11/software renderer, behavioral suites, ASan/UBSan, SDL dummy services. |
| Linux Wayland | untested | SDL supports host integration; no compositor smoke run yet. Xorg is not Wayland evidence. |
| Windows | tested portable subset | Alpha.10 hosted MSVC portable suites pass in Debug and Release. Native GUI/Direct3D, OS IME, accessibility and app packaging remain open. [Debug](https://github.com/mattmezza/cui/actions/runs/45852900282/job/140023625532) · [Release](https://github.com/mattmezza/cui/actions/runs/45852900282/job/140032909612) |
| macOS | tested portable and native headless subsets | Alpha.10 hosted Apple Silicon passed the SDL dummy/software suite with declared skips, installed the development app bundle and verified its ad-hoc signature. Cocoa/Retina/Metal, OS IME, accessibility and distributable signing remain open. [Native job](https://github.com/mattmezza/cui/actions/runs/45852900282/job/140023625018) |
| Browser WASM | tested partial | Actual Chromium runs CUI WebAssembly with Canvas/input host, including pointer/keyboard, scalar input, popup focus/Escape, themes and 390px bounds. See [browser gallery](WEB_GALLERY.md). Native text shaping/OS accessibility remain separate. |
| Android | untested | No wrapper/device validation. |
| iOS | untested | No wrapper/simulator/device validation/signing. |
| WASM/WASI | tested headless WASI subset | Zig0.16/Wasmtime49 portable behavioral/embedding suites compile and run; current source/counts are recorded in validation logs; see [WASI.md](WASI.md). |

Optional unavailable integrations are explicit. Compilation will be recorded separately from runtime tests. SDL_GPU SPIR-V shader support currently confines that adapter to Vulkan-capable devices; D3D12/Metal formats are pending. Sokol GLCORE is tested on Linux Xorg; its other backends and platforms are pending.

Linux X11 also has [continuous external resize evidence](LINUX_LIVE_RESIZE.md)
under private Xvfb/software rendering: intermediate captures follow window size,
responsive sidebar/card changes and retained application state while a resize
burst is ongoing. This does not establish physical window-manager dragging or
Wayland behavior. Linux resize support is a separate acceptance scope from the
Cocoa/Windows callback scheduling contract.

The optional pinned Unicode regex archive is currently validated on Linux native and WASI only. Portable Core/Base/Components need neither that archive nor Rust. Hosted macOS runners cover native builds and headless execution. Full platform acceptance also requires interactive macOS window, rendering, input and accessibility checks. A remotely accessible Mac can provide that evidence.

Linux hosted GCC and Clang passed their configured native/sanitizer checks on CI run38225532353 plus Xvfb gallery smoke and Sokol pixel/cache tests. The CI GL driver reported **llvmpipe**, a software driver; this is API/readback coverage and is separate from the actual Intel/Mesa hardware evidence on the development Xorg host. Saved [previous CI summary](validation/ci-01865d5.txt) and [latest published-source job metadata](validation/ci-3ef6c49.json) record exact source and outcomes.

See [the Mac workflow](MACOS.md) for hosted and interactive checks, and [Linux cross compilation](MACOS_CROSS.md) for build-only evidence.
