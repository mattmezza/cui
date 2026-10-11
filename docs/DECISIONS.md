# Decisions

- Core/base/styled layers use portable C11. Clay layout is hidden behind ordinary structs.
- MIT for original code; pinned Clay zlib license preserved. GPUI reference software is not linked or copied.
- Fixed budgets and frame arenas first, no hidden per-frame allocation. Persistent application data is host-owned.
- Previous completed frame supplies hit regions, ordered event queue runs before current composition. First frame establishes regions; host must perform initial layout before interaction.
- SDL_Renderer accelerated 2D is an explicit compatibility integration; it must never be called SDL_GPU. Missing native/GPU bridges stay in backlog.
- Public repo existed already; preserve visibility. Work on build/initial-framework with checked checkpoints.
- Full 78-entry manifest remains the scope; strict verified status requires whole-family feature evidence.
