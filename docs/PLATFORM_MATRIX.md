# Platform evidence

| Target | Status | Evidence and limits |
|---|---|---|
| Linux Xorg | tested partial | Native SDL_Renderer OpenGL, independent Sokol GLCORE and SDL_GPU Vulkan on Intel Graphics; actual frames/readbacks, pointer/key model and clipboard tests. SDL composition/candidate-rectangle forwarding has injected service tests; real OS IME/candidate-window acceptance and native screen readers remain unverified. |
| Linux headless | tested partial | GCC C11/software renderer, behavioral suites, ASan/UBSan, SDL dummy services. |
| Linux Wayland | untested | SDL supports host integration; no compositor smoke run yet. Xorg is not Wayland evidence. |
| Windows | tested portable subset | Published67d86ed4: hosted MSVC19.51.36260.0 passes145/145 portable checks on [CI run38488091130](https://github.com/mattmezza/cui/actions/runs/38488091130). Native adapters are disabled; native GUI/Direct3D, OS IME, accessibility and app packaging are unverified. |
| macOS | tested portable subset | Published67d86ed4: hosted Apple Clang21.0.0.21000101 passes145/145 portable checks on [CI run38488091130](https://github.com/mattmezza/cui/actions/runs/38488091130). Native adapters are disabled; native GUI/Metal, OS IME, accessibility and app packaging are unverified. |
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

The optional pinned Unicode regex archive is currently validated on Linux native and WASI only. Portable Core/Base/Components need neither that archive nor Rust. Developing on Linux does not prevent macOS support: hosted macOS runners can compile/test the native integration once implemented. Full platform acceptance also requires actual macOS window/render/input/accessibility and hardware/session testing, which can use a remotely accessible Mac rather than the maintainer's laptop.

Linux hosted GCC and Clang passed their configured native/sanitizer checks on CI run38225532353 plus Xvfb gallery smoke and Sokol pixel/cache tests. The CI GL driver reported **llvmpipe**, a software driver; this is API/readback coverage and is separate from the actual Intel/Mesa hardware evidence on the development Xorg host. Saved [previous CI summary](validation/ci-01865d5.txt) and [latest published-source job metadata](validation/ci-3ef6c49.json) record exact source and outcomes.

Native macOS build/bundle and headless adapter CI are now authored; they have not yet been observed running for this change. See [the Mac workflow](MACOS.md) and [Linux cross compilation](MACOS_CROSS.md). Cocoa/Retina/Metal, IME and accessibility acceptance remain separate.
