# Initialized software V2 adapter (proposal)

Design only, after private painter690189fd (integrated/accepted as63b77c0b). No public declarations, capability advertisement or renderer changes here. Current `include/cui/cui_software.h` exposes only V1; `cui_software_interface` returns the unchanged `cui_renderer` function table. The proven private with-font helper is the implementation seam, not a consumer API. SDL_Renderer, SDL_GPU, Sokol, Canvas and foreign bindings remain required follow-ons.

## Smallest public surface

Add two software-specific functions and one owned POD information record to `cui_software.h`, including `cui_draw_v2.h`. Do not extend `cui_renderer`, reinterpret V1 commands or introduce a generic renderer registry.

```c
enum {
    CUI_SOFTWARE_V2_TEXT_A8 = 1u << 0,
    CUI_SOFTWARE_V2_TEXT_CONTEXT = 1u << 1,
    CUI_SOFTWARE_V2_TEXT_TRACKING = 1u << 2
};
typedef struct {
    uint32_t size;
    cui_draw_v2_support draw;
    uint32_t text_features;
    cui_render_v2_options defaults;
} cui_software_v2_info;
cui_error cui_software_query_v2(const cui_software *, cui_software_v2_info *out);
cui_error cui_software_render_v2(cui_software *, const cui_draw_list_v2 *,
                                cui_color clear, const cui_render_v2_options *);
```

These are proposals, not existing symbols. Query requires a live initialized renderer and `out->size==sizeof(*out)`, rejects known output overlap with the renderer/pixel allocation, builds a local fully initialized result, and leaves output untouched on error. It allocates nothing and performs no FT/HB/provider work. Null/bad size is INVALID_ACTION; active V2 rendering is LIFECYCLE; a target without room for even the basic staging/command plan is CAPACITY. Opaque font-graph overlap cannot be validated; valid disjoint application output storage is a caller precondition.

`draw` reports exact ABI2/command stride policy through existing shared records, CENTER_V1 and BACKGROUNDS. All owned SOLID/two-stop CSS/SOURCE_038, SRGB/OKLAB backgrounds are supported under existing numerical/resource admission. Do not fabricate a text bit in shared `draw.features`. Software-specific A8 means the actual text build plus non-NULL borrowed font; CONTEXT additionally uses the actual private Bidi availability query, and TRACKING uses the actual font capability function. Context/span shaping is distinct from raw text. A8 includes resolved text gradients; it does not advertise intrinsic RGBA/color-font formats or coverage of every Unicode glyph. Missing faces and unsupported bitmap modes remain content failures before publication.

`defaults` has exact size/tag, the current physical renderer dimensions, scales1, origins0 and the queried resource ceilings. NULL render options selects those current defaults. Explicit options must have the exact shared size/tag, match current renderer dimensions, satisfy shared density/origin limits, and use positive ceilings no larger than query ceilings; smaller limits intentionally may reject a composed frame. No hidden default substitution for explicit zero fields.

## Real ceilings and composition handshake

Let P=checked `width*height*4`, C=`sizeof(cui_draw_command_v2)`, D=`sizeof(cui_software_v2_paint_descriptor)`. Default CPU ceiling B=32MiB, metadata M=1MiB. Query command ceiling is `min(16384,(B-P)/C,M/D)` after checking P<=B; zero means CAPACITY. Background ceiling is `min(4096,max_commands)`. This establishes room for basic command copies/descriptors, not all possible command resources. Render recomputes the same count bound from explicit smaller CPU/metadata ceilings before invoking the helper; unknown command tags remain INVALID_ACTION.

Other ceilings retain the reviewed helper contract: aggregate background/shadow/glyph target rectangles8M, per background4M, shadow rectangles1M and source evaluations1M, CPU/texture/transfer32MiB each and metadata1MiB. Text source/input/attempt/face/glyph ceilings remain those in `cui_draw_v2.h` and `TEXT_PREPARATION.md`; intrinsic color glyphs are unsupported. Render rejects above-ceiling options with CAPACITY, malformed tags/ranges/mismatched dimensions with INVALID_ACTION, and malformed/non-OK lists without clearing. Counts and byte ceilings are upper bounds, not a reservation or a promise that every combination fits.

Core composition storage is separate from renderer transaction storage. `cui_begin_frame_v2` further intersects query counts with context configuration and command bytes, and accounts the aligned max-elements owner map plus background owners against frame background bytes. Its defaults remain16MiB command/512KiB background storage, independently subject to existing Core limits. Do not subtract renderer scratch from Core storage or call either budget total process memory.

Normal flow: query the actual renderer; put `info.draw` in frame options; request BACKGROUNDS before composition; begin/compose/matching-end exactly once; render the resulting V2 list with `info.defaults` or explicit physical transform. The old V1 loop/interface stays intact. A failed render neither calls Core begin/end nor replays input/application callbacks, and never falls back to V1 or a flat color.

A query is an owned snapshot of limits, not an instance token, pin or allocation reservation. Requery after resize. Render always checks current dimensions/counts/ceilings independently: stale defaults fail before paint; an old support record cannot authorize a larger current allocation. A still-live V2 list may be retried with valid options while its borrowed resources remain stable. Rendering transforms do not change Core logical layout/input dispatch; host coordinate mapping remains the application's existing responsibility.

## Renderer ownership and lifecycle

Use the renderer's existing tightly packed pixel allocation as the target, with checked stride/span. Adapter-private malloc/free wrappers are the sole transaction allocator; no new public allocator parameters, global allocator swaps or retained workspace/cache. The private helper owns command copies, staging, prepared BG/shadow bytes and counted text blocks until synchronous return. It preserves original command/glyph order and existing coverage/alpha/clip policies; there is no logical-ceil IMAGE remask.

The existing constructor merely borrows `font`; it never retains/closes it. A non-NULL font and its current graph/dependency storage must be valid, stable/disjoint from renderer target/workspace through text-capable query/render, with no recursive/concurrent rendering of that font on another target. Graph changes and release of removed faces between completed calls remain allowed under existing font ownership rules. Normally keep the root font until renderer destruction; closing it after the last text operation is also safe if no later query/render uses it, since destroy never reads/closes it. Only known source/span/stop/image/mesh ranges can be validated; a one-byte font sentinel is not a whole-graph alias proof.

Add one private zero-initialized V2 busy guard. Set it before any transaction allocation/font work and retain it through all cleanup. A local private target view has its own helper guard; the renderer guard prevents public reentry through fresh views. Busy V2 render/query returns LIFECYCLE unchanged. V1 render and resize return false before work while this new state is active. Existing void destroy must return without freeing while busy; callers retry destroy after the synchronous call. Read-only pixels/interface access remains valid; bytes stay old until successful publication. No generic last-error/dispatch architecture is needed.

**Review decision:** one private guard may grow the opaque renderer allocation. Measure original/candidate size/offsets before implementation; do not assume tail padding. Public ABI and old call counts/pixel/cache allocation routes can stay unchanged, but byte-identical object allocation cannot be promised without that proof. If strict unchanged object-allocation bytes are required and padding does not fit, stop for a specific decision rather than introduce a side registry. The new busy-state refusals affect only V2 reentry; ordinary V1 behavior remains unchanged.

Outside active V2, resize retains its current allocate-before-free behavior: failure preserves dimensions/pixels; success invalidates old pixel pointers and changes future defaults. Destroy releases existing V1 shadow cache/pixels/object, never the borrowed font. Software has no async/inflight resources; all V2 blocks are released before return. Query/render add no persistent pixel allocations. Existing renderer pixels/font state are class1; counted per-render payload/prefixes are class2; new opaque FT/HB/Sheen/provider work is class3. Only class2 fits the stated CPU/metadata envelope.

## Failure and publication

Validate the original options/list pointers and their known target aliases before any wrapper copy could hide overlap. NULL options uses local defaults; explicit options preserve the helper's alias checks. Require an ABI2 list with matching stride/output policy/features and no upstream error. Current list/target/font resources stay stable until return; copying command values does not own legacy backing storage.

Whole-list validation, exact base plans, all BG/shadow preparation, counted text recording and transformed glyph/quota preflight finish before scratch clear. Only infallible pure painting and active-row publication follow. Any validation/capacity/allocator/dependency/unsupported-content failure leaves every published pixel unchanged, releases all class2 blocks and clears busy for retry. Provider/font side effects cannot be rolled back; publication is failure atomicity, not concurrent memory-transaction atomicity. Existing V1 render remains nontransactional, including its historical late-failure behavior.

## Independent implementation and acceptance

1. Freeze old public V1 declarations/function-table layout and actual original renderer outputs first. Measure private object size/offsets and constructor/resize allocation counts/bytes, then review the guard decision above. Implement only the new software query/render wrappers and new-state guards, forwarding the proven helper; no V1 painter rewrite.
2. Test actual query flags in text/Bidi, text/no-Bidi, no-text and NULL-font builds; exact size/output atomicity/known aliases, no allocations on queries, dynamic command formulas, target too large to stage, explicit budgets above/below/equal limits and stale resize defaults. Query must work before Core composition; no fabricated support fixtures as the only proof.
3. Run real Core query→begin→all-kind composition→end→public render, including named/anonymous backgrounds, portals/clips, text, images, meshes and DROP/INSET. Compare independent frozen V1 unit pixels and proven private physical reference matrices (anisotropic/fractional/extreme scales, transparent destinations, source order); preserve no-Bidi poisoned-source and missing-font rejection.
4. Sweep adapter and counted-text allocations with private same-TU test instrumentation, not global production allocator swaps. Exercise late failure/retry and reentry into V2, query, V1 render, resize and void destroy during allocation/free; busy destroy must not free the renderer. Alternate V1→V2→failed V2→V1 and verify legacy pixels/cache behavior. Borrowed-font misuse/concurrent graph mutation remains an explicit precondition, not a validated safety claim.
5. Compile selective/umbrella strict C11 and public C++ declarations with real symbol use; copied SDK/native/WASM variants use existing pinned dependencies and bounded build leases. Run automated native checks only through private-Xvfb wrapper; browser checks headless. Update authoritative API/docs/metadata, canonical SDK/amalgamation/skill and all78 statuses only after actual public adapter acceptance. Four other adapters and bindings remain pending; a software-only success does not complete shared rendering parity.

Source evidence: `cui_software.c` constructor/resize/destroy/pixels/interface and its private renderer record; `cui_renderer.h` V1 function table; `cui_draw_v2.h` shared records/options; `cui_draw_v2_internal.h::cui_begin_frame_v2` actual composition minima/map accounting; `cui_software_v2_paint_internal.h` exact workspace/ordered preparation; `software-v2-text-worker.json` and root63b77c0b acceptance. No implementation/compiler/new dependency accompanies this proposal.
