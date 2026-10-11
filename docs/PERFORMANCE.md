# Measured performance

Development host: Intel Core Ultra 5 235U (14 logical CPUs), Arch Linux x86_64 kernel7.2.8, GCC16.2.1, default unoptimized CMake build. `./build/cui_bench`:500 frames composing100 boxes+text at800×600, Clay culls outside viewport;28000 visible draw commands total. CPU `clock()` measurement:98.079ms total /0.196ms per composed frame in the initial run. This is CPU layout/draw-list generation, not end-to-end GPU frame time, and excludes renderer/text shaping and OS scheduling. Reproduce with `make bench`; numbers are not a comparison to other frameworks.

SDL_Renderer has bounded persistent glyph textures (256 LRU entries,4MiB coverage budget), text tint reuse tested. SDL_GPU has a1024×1024 grayscale atlas with4096 cached masks and32768 per-frame quad entries; uploads only after newly cached glyph masks. Atlas exhaustion returns failure instead of claiming successful output. GPU batching/regression thresholds and workload-specific measurement remain pending.
