# Platform evidence

| Target | Status | Evidence and limits |
|---|---|---|
| Linux Xorg | tested partial | Native SDL_Renderer OpenGL, independent Sokol GLCORE and SDL_GPU Vulkan on Intel Graphics; actual frames/readbacks, pointer/key model and clipboard tests. No screen-reader/IME bridge. |
| Linux headless | tested partial | GCC C11/software renderer, behavioral suites, ASan/UBSan, SDL dummy services. |
| Linux Wayland | untested | SDL supports host integration; no compositor smoke run yet. Xorg is not Wayland evidence. |
| Windows | tested portable subset with failed script gates | MSVC core/software behavioral executables passed on CI run38185199059; encoding/hash checks failed and fixes are being retested. Native GUI untested. |
| macOS | tested portable subset | Hosted Apple Clang core/software behavioral tests passed on CI run38185199059. Native GUI/Metal untested. |
| Android | untested | No wrapper/device validation. |
| iOS | untested | No wrapper/simulator/device validation/signing. |
| WASM/WASI | compiled only | Zig0.16 cross-compiled the core/bootstrap test to wasm32-wasi. No WASI runtime or browser-host test. |

Optional unavailable integrations are explicit. Compilation will be recorded separately from runtime tests. SDL_GPU SPIR-V shader support currently confines that adapter to Vulkan-capable devices; D3D12/Metal formats are pending. Sokol GLCORE is tested on Linux Xorg; its other backends and platforms are pending.
