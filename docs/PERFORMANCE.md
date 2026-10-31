# Measured performance

Development host: Intel Core Ultra 5 235U (14 logical CPUs), Arch Linux x86_64 kernel7.2.8, GCC16.2.1, default unoptimized CMake build. `./build/cui_bench`:500 frames composing100 boxes+text at800×600, Clay culls outside viewport;28000 visible draw commands total. CPU `clock()` measurement:98.079ms total /0.196ms per composed frame in the initial run. This is CPU layout/draw-list generation, not end-to-end GPU frame time, and excludes renderer/text shaping and OS scheduling. Reproduce with `make bench`; numbers are not a comparison to other frameworks.

SDL_Renderer has bounded persistent glyph textures (256 LRU entries,4MiB coverage budget), text tint reuse tested. SDL_GPU has a1024×1024 grayscale atlas with4096 cached masks and32768 per-frame quad entries; uploads only after newly cached glyph masks. Atlas exhaustion returns failure instead of claiming successful output. GPU batching/regression thresholds and workload-specific measurement remain pending.

## Hosted check scheduling

Portable CI runs at most two CTest processes at once and sets
`CMAKE_BUILD_PARALLEL_LEVEL=2` for nested consumer builds. Each test uses its own
process and temporary consumer directories; reference fixtures remain read-only.
Native CI and local test scheduling are unchanged. On Linux, continue using
`python scripts/test_local.py --build-dir build`, which isolates the display.

The baseline Windows Debug job in [run 41249695765](https://github.com/mattmezza/cui/actions/runs/41249695765)
spent 165.88 seconds on `agent_skill_contract` and 98.43 seconds on
`binding_package_source_manifest`. Bounded overlap targets this serial cost;
these timings are a baseline, not measured savings from the scheduling change.
Hosted duration, memory use and any contention regressions still need comparison.
