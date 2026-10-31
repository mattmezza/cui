# Unreleased Core V2 background transport

Explicit C-only opt-in in `include/cui/cui_draw_v2.h`. Core mode/storage/setters and
emission are implemented; **no renderer, Canvas wire, bridge or foreign binding
supports this list yet**. Software, SDL_Renderer, SDL_GPU, Sokol GLCORE and Canvas
V2 consumption/preparation remain mandatory follow-ups, not completed parity.
Do not pass a V2 list to a V1 renderer, cast command arrays or fabricate adapter
support in an application. There is currently no usable end-to-end background
rendering example. The test consumer descriptor is deliberately synthetic.

The [reviewed record/adapter contract](design/BACKGROUND_V2_RECORDS.md) retains the
full five-adapter objective. Pure [background values/sampling](BACKGROUND_VALUES.md)
remain independently usable. Existing V1 APIs/config/records/enums/layout/input
and default commands/pixels are unchanged; no V2 buffers are allocated by V1-only
contexts. No component-wide or all78-parts coverage claim follows from setters.

A future actual consumer query must precede composition. Copy its exact-sized
`cui_draw_v2_support` into exact-sized `cui_frame_v2_options`; select required
features explicitly, then call `cui_begin_frame_v2` and matching
`cui_end_frame_v2`. Unknown features/record version/output policy fail
INVALID_ACTION; insufficient budgets/allocation fail CAPACITY. Admission reserves
all optional storage before opening/consuming input. Replaced frame-only blocks
are freed before growth allocation
to bound peak memory; allocation failure leaves input queued and context usable,
with no new active mode/frame. Previous V2 lists expire at begin attempt, including
failed growth. Invalid option/budget validation never opens or consumes a frame.
Wrong end reports LIFECYCLE without closing, finalizing layout or dispatching paint
callbacks. Clear the diagnostic, then match the end once. Null V2 output rejects
before closing. Matching end closes even on failure and returns an empty ABI2
non-OK list; never render/clear a non-OK list. Numeric snapshot invalidity from
an in-frame diagnostic persists after clear; only a fresh successful frame restores
readiness. No conversion occurs after semantic/readiness publication.

`cui_set_background` addresses the actual open anonymous/named box;
`cui_element_set_background` addresses a named box already submitted this frame,
including a closed box or portal. No previous-frame geometry, new nodes, ID
replacement, input routing or callback dispatch architecture. Reject suppressed,
layout-only, missing/colliding targets and mutation from measurement/prepaint or
diagnostics. Values validate/copy only active fields; inactive fields become zero.
Invalid/capacity failure preserves the previous override and all other paint.
NULL clears; transparent SOLID replaces the ordinary fill. Repeated calls reuse
an owner slot; clear preserves other custom paint and the owner's slot until the
next admitted begin. Distinct touched owners consume capacity even if cleared.

Final-layout bounds and clamped box corner radii accompany the brush by value;
ancestor opacity is separate and applied once by future preparation. Replace ONLY
box fill: DROP, outside outline, fill, INSET, mesh, locally clipped underlay,
prepaint, children retain their order; deferred layers/portals retain their route.
A no-setter V2 frame wraps ordinary V1 commands exactly. Legacy strings, bitmap
pixels, mesh vertices and font/provider backing retain V1 borrowing; copying the
array copies brushes, **not a complete resource snapshot**. Array lifetime ends at
next begin attempt (either mode) or context destruction.

Command capacity is min(config.max_commands,16384,support.max_commands,
command_bytes/sizeof(cui_draw_command_v2)). Default16MiB, hard16MiB. Background
capacity is min(config.max_elements,4096,support.max_backgrounds); its default512KiB
budget must fit align_up(max_elements*sizeof(uint32_t),alignof(private owner))
plus capacity*sizeof(private owner record), in one aligned block. Hard16MiB,
checked actual sizes. Feature-free V2 needs no owner map.
Retain bounded high-water capacities until destruction; smaller later logical
budgets do not shrink them or allocate duplicate frame copies. At most32MiB
additional retained payload plus one fixed private control record; ordinary Core
arrays/frame/resource budgets remain separate. No raster cache/upload allocation.
All five support descriptor pixel/upload budgets are for future consumer admission;
Core validates nonzero bounds when requesting backgrounds but cannot preflight
physical pixels or target availability before layout/render.

Validation combines the independent pre-V2 frozen V1 commands/software pixels,
all24 scenes wrapped without setters, strict V1 ABI checks, faulted allocation,
transactional owner/identity/clear tests, current portal move/resize, exact paint
order/opacity/radii, wrong-mode/non-replay, typed numeric queue/claim and reentrancy.
No GPU/browser/glyph rendering result is claimed for this Core-only slice.
