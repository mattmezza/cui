# Platform evidence

| Target | Status | Evidence and limits |
|---|---|---|
| Linux Xorg | tested partial | Native SDL_Renderer OpenGL and SDL_GPU Vulkan on Intel Graphics; actual frames/readbacks, pointer/key model and clipboard tests. No screen-reader/IME bridge. |
| Linux headless | tested partial | GCC C11/software renderer, behavioral suites, ASan/UBSan, SDL dummy services. |
| Linux Wayland | untested | SDL supports host integration; no compositor smoke run yet. Xorg is not Wayland evidence. |
| Windows | untested | Portable CI authored; no runtime evidence yet. |
| macOS | untested | Portable CI authored; no Metal shader integration yet. |
| Android | untested | No wrapper/device validation. |
| iOS | untested | No wrapper/simulator/device validation/signing. |
| WASM | untested | Core portability considered; no browser-host runtime integration yet. |

Optional unavailable integrations are explicit. Compilation will be recorded separately from runtime tests. SDL_GPU SPIR-V shader support currently confines that adapter to Vulkan-capable devices; D3D12/Metal formats are pending. Sokol is a planned independent GPU integration, not a shipped claim.
