# Full-scope roadmap

Every one of the 78 baseline families remains in gpui-parity.json with feature-level subtasks. None are currently verified as a full parity family.

1. Complete core contracts: timers/animation, persistent widget state budgets, scrolling, overlays/portals and semantics relationships.
2. Finish usable input: caret/selection geometry and pointer placement, clipboard/IME, graphemes, bidi/fallback, wrapping, undo grouping.
3. Compose real dialogs/menus/select/combobox/tabs and calendar from tested behavior.
4. Build keyed virtual list/data table/dock models and real native stories; measure100K rows and200K editor lines before performance claims.
5. Independent Sokol backend, Metal/D3D12 shaders, device loss, resources/images and native platform/accessibility bridges.
6. Cross-platform runtime evidence, source vendoring, docs website, optional capability-based extension host.

See COMPONENTS.md for the full family breakdown and MAINTAINER_ACTIONS.md for manual setup. Advanced editors, dock/table/chart widgets will not be counted from placeholder rectangles.

Shared backgrounds have owned pure values/sampling and an unreleased
[Core V2 transport](CORE_DRAW_V2.md) slice plus private allocation-free [CPU preparation](BACKGROUND_PREPARATION.md) and [counted text recording](TEXT_PREPARATION.md). V2 consumers
for software, SDL_Renderer, SDL_GPU, Sokol and Canvas, foreign authoring, and
component-part coverage across all78 remain pending. No family status is upgraded
by generic setters or Core transport alone.

Core Y-layout now includes optional bounded real overlay track/thumb allocation
with independent visibility/interactivity and final current paint/hit geometry.
This C-only constraint does not complete Base/Styled Dialog slots, Root/native-host
actions or any of the five foreign-builder composition plans. See
[SCROLL_LAYOUT.md](SCROLL_LAYOUT.md) for admission and resource-budget limits.
