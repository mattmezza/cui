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

## Mail UI integration feedback (001–022)

These decisions address the screenshot-recreation report without changing the
78-family scope or claiming full native-framework parity.

| Issue | Resolution |
| --- | --- |
| 001 | Existing first-include distribution fix retained; the documented single-TU snippet remains a CTest target. |
| 002 | Existing resolved-width button-content fix retained and exercised by the suite. |
| 003 | Single-line values/placeholders never word-wrap; fixed-width viewport geometry is constrained. Headless regressions cover fixed and grow placeholders. Text is clipped, without automatic ellipsis. |
| 004 | README distinguishes portable dist from native hosting; installed `cui-sdl3` package and starter supply the supported native build recipe. |
| 005 | Default SVG bundle increased from 106 to 123 essentials using pinned assets; filter/trash-2/more-vertical aliases added. The actual pinned catalog has 1,830 entries. |
| 006 | Interaction documents a complete clickable row with stable matching IDs, paint states, activation and keyboard-role guidance. |
| 007 | Added opt-in per-edge padding with explicit zero support; uniform padding remains the default. No separate row/column gap API needed for current row/column layouts. |
| 008 | Text API matrix verifies wrapping in positive fixed/grow parents; Text documents paragraph versus explicit no-wrap behavior. No additional core change needed on this revision. |
| 009 | Native starter prominently explains real font registration with checked bold/semibold examples. No filename guessing, synthetic weights or global diagnostic policy added. |
| 010 | Generated `cui_all.h`/`CUI_IMPLEMENTATION_ALL` and machine-readable implementation inventory cover all portable source implementations, with a compile/link/run regression. |
| 011 | Opt-in `auto_measure_content` uses preceding-frame ordinary content extent; wrapped-height, shrink/clamp and explicit extent precedence are tested. Base-only and virtual content still use explicit extents. |
| 012 | Added C11 static-initializer `CUI_STR` macro and expression guidance, tested in Core. |
| 013 | Avatar options accept independent fallback background/foreground overrides, tested without a theme copy. |
| 014 | Sidebar has a complete persistent account-tree recipe. State remains caller-owned: implicit context retention would add lifecycle complexity for this minor issue. |
| 015 | Documented theme-palette/variant mapping and local overrides. Deferred a breaking theme-struct redesign. |
| 016 | Icon docs explain error checking, fallback and default-bundle inspection. Errors stay explicit return values; global logging/assertion or theme placeholder policy is left to the host. |
| 017 | Form documents a leading-search-icon InputGroup; headless test verifies artwork placement and prefix-click focus followed by typing. No duplicate input-slot API added. |
| 018 | Added generated 78-family status table and `scripts/cui_status.py` for selected features/evidence/limitations, with a freshness check. |
| 019 | Documented custom corner/border precedence, including explicit zeros; retained the compatible public representation. |
| 020 | Documented custom button active/hover/normal and disabled precedence. Independent selected-hover palettes remain custom-row composition. |
| 021 | Documented physical alignment axes and main-axis centering in either orientation. No duplicate justify/items fields added. |
| 022 | Documented dedicated static-library implementation TU and existing modular parallel build; no second generated split-source distribution added. |

Known follow-up: grow-width Input containing a long populated value can still
report an expanded internal viewport. Explicit input width avoids the caret/hit
geometry ambiguity. This is separate from the fixed/grow empty-placeholder
regression; a broader grow/min-content sizing change is deferred. GPU hardware
checks unsupported by Xvfb remain deferred; automated native windows run only in
the isolated test display.

Integration validation: warnings-as-errors CMake build passed; the final
`python scripts/test_local.py --build-dir build` run passed 200/200 CTest entries.
Distribution/icon generation checks and the 78-entry manifest check passed.
The new clickable-row, account-tree, search-prefix and font-registration recipes
also compile as C11 with `-Wall -Wextra -Wpedantic -Werror`. These checks do not
add physical GPU/device, other-OS or full-family parity evidence.
