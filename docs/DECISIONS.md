# Decisions

- Core/base/styled layers use portable C11. Clay layout is hidden behind ordinary structs.
- MIT for original code; pinned Clay and Sokol zlib licenses preserved. GPUI reference software is not linked or copied.
- Fixed budgets and frame arenas first, no hidden per-frame allocation. Persistent application data is host-owned.
- Previous completed frame supplies hit regions, ordered event queue runs before current composition. First frame establishes regions; host must perform initial layout before interaction.
- SDL_Renderer accelerated 2D is an explicit compatibility integration; it must never be called SDL_GPU. Missing native/GPU bridges stay in backlog.
- Public repo existed already; preserve visibility. Work on build/initial-framework with checked checkpoints.
- Full 78-entry manifest remains the scope; strict verified status requires whole-family feature evidence.
- Independent Sokol GLCORE borrows the host context/framebuffer and keeps SDL out of its library interface. Its runtime ownership mode and bounded resource cache are explicit.
- Dock persistence uses a bounded versioned ASCII grammar with transactional restore. Parser correctness across floating-point implementations is a portability gate.
- Plot starts with allocation-free scales/geometry; chart presentation and retained series remain separate component work.
