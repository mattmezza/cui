# Core contract and limits

The core uses the pinned Clay implementation for sizing and draw generation. Public APIs contain C11 types only. Calls run on one UI thread; contexts may be interleaved, including open frames, because each layout operation selects its owning Clay context. Concurrent calls are unsupported.

Queue input before `cui_begin_frame`. Events resolve in order against enabled interactive regions from the preceding completed frame, intersected with ancestor clips. The first frame has no input targets. Pointer down captures and focuses the topmost submitted hit region; release activates only inside that same region. Cancel and Escape clear pointer and space captures. Tab and Shift+Tab follow previous submission order. Enter activates on non-repeated key down; Space activates on key up when its captured focus remains current. Repeated key downs do not activate. Disabled controls suppress pending activation; disabled and omitted controls lose focus and capture by frame end.

`cui_event_target` reports the target at dispatch time, allowing controls to process multiple down/move/up or focus/text events in a single frame. Key events target the focus before that event; pointer move and up target capture. `cui_interact` reports final held state and consumes pending activation once. Multiple queued activations for one control are coalesced to its boolean activation. `cui_update_semantic` updates a submitted controlled value after a same-frame change.

Text and semantic names are copied to the fixed frame string arena; draw lists and semantic names survive until the next begin. The default text measure uses approximate width `length * font_size * 0.6` and height `font_size * 1.3`; applications needing font metrics supply the callback. Draw conversion supports rectangles, uniform borders, text, and clip push/pop. Unsupported Clay draw types are not exposed by the public composition API.

The configuration defaults to 4096 elements, 8192 commands, and 262144 string bytes. There are at most 256 queued events, ID scopes, and nested boxes. Capacity failures set a sticky error. String exhaustion produces empty strings. Command exhaustion returns an empty list to avoid exposing unbalanced clip commands. Skipped elements remain safely balanced by matching end calls. Errors can be explicitly cleared after inspection; frame lifecycle errors reject invalid transitions.

IDs use scoped 64-bit FNV hashing. Nonzero element IDs and semantic IDs must be unique within a frame. Clay internally receives a domain-mixed 32-bit ID; consecutive raw public IDs are avalanched before Clay derives its automatic text IDs. Distinct public IDs with equal internal IDs are reported as duplicates rather than silently aliased. ID zero creates an anonymous box and is not an interactive control ID. Semantic controls should be submitted inside their matching element box so clipped hit regions can be established. Bounds returned during submission refer to the previous completed frame; completed semantic bounds reflect the current layout.

Headless tests cover input ordering, first-frame input, reordered IDs, capture and cancellation, focus traversal, keyboard activation, disabled and omitted controls, string ownership, draw conversion, clipped hit testing, context interleaving, duplicate/scoped IDs, lifecycle errors, and capacity recovery. This is the core interaction and composition contract, not a claim of complete shadcn component parity.

Focus scopes are explicit, independently nested declarations. A successful `cui_begin_focus_scope` must be paired with `cui_end_focus_scope`; failed begins must not be ended. At most 256 scopes can be declared per frame. The last declared modal scope is active, including nested scopes; enabled descendants receive initial focus by frame end. On the next input dispatch, Tab traversal and pointer hit testing are restricted to its descendants. Pointer down outside an active modal targets the scope ID without activating the underlying control. Escape targets the focused control's innermost scope, or the last declared scope when focus is outside all scopes; this also supports nonmodal popup dismissal. Nonmodal scopes alone do not restrict traversal or pointer routing. Omitted scopes stop trapping after the frame completes.

`cui_focus_restore` checks that a trigger appeared as an enabled interactive semantic in the previous completed frame. It permits restoring a trigger while its former modal scope is being omitted. The completed frame clears focus if that trigger is now disabled or omitted, and an active current modal still enforces its own descendants. Scope IDs should be distinct from interactive control IDs when their dismissal events must be distinguished. These APIs provide routing and focus state; overlays own visibility, panel bounds, outside-dismiss policy, placement, and rendering.

## Floating layers and compound controls

`cui_begin_portal` opens a viewport-positioned box outside ancestor clipping and
normal layout flow. Positive layers sort painting and interactive hit regions by
layer, with later submitted regions winning ties. Close it with `cui_end_box`.
Portals can clip their own content. Bounds must be finite and nonnegative; a
malformed portal suppresses its subtree while preserving paired begin/end calls,
even at the nesting budget. A portal does not itself trap focus: use a modal
focus scope where appropriate. Passive portal content lets pointer events pass
through; an active modal scope blocks underlying interactive controls.

`cui_set_semantic_details` records expanded/selected/read-only/required/invalid
state, numeric ranges, descriptions and relationships independently of painting.
Description strings are frame-copied. Group, tab-list/panel, list/menu, status,
alert, image and other roles distinguish container/content from focusable controls.
`cui_set_tab_stop(false)` removes an item from Tab traversal while preserving
pointer/programmatic focus, so composite controls can use roving focus.

`cui_event_activation` preserves each validated pointer-release, Enter or
released-Space activation even after `cui_interact` consumes the aggregate flag.
Compound reducers process this alongside events in input order, so ArrowDown then
Enter selects the newly navigated item within one frame. Core target IDs still
refer to dispatch-time previous-frame focus; reducers apply their local cursor.

Call `cui_dismiss_focus_scope` after reducing closing popup behavior and before
painting it. It withdraws current semantic descendants, deactivates nested traps,
and restores an enabled previous-frame trigger. Paired scope begin/end calls
remain required. This avoids briefly forcing focus back into a closing popup.

Boxes expose portable `align_x`/`align_y` start, center and end values. They align
children inside the available content area after sizing/padding; default start
alignment preserves existing layout behavior.

Clipped boxes accept `offset_x`/`offset_y` to translate children while preserving the box viewport and ancestor clips; negative offsets scroll content forward. Fractional offsets are retained. Secondary/middle pointer events carry `pointer_button`, hit-route to previous regions, and never change ordinary primary capture/focus/activation. SDL3 maps all three buttons.

`cui_box.fixed_width` and `fixed_height` explicitly constrain a dimension,
including exactly zero; without the flags, zero retains the existing FIT rule.
Negative/nonfinite explicit dimensions normalize to zero. This supports collapsed
viewports without a visible epsilon-sized remnant.

`cui_box.layout_only` measures its complete subtree but suppresses paint, semantic
nodes and routed input, including nested portals. Bounds tracking still works;
callbacks still execute and applications must avoid unrelated side effects.
Focus requests from the subtree are ignored; hidden modal scopes do not trap
outside focus. Clipping commands remain balanced even when all enclosed paint is
suppressed. This is useful for measuring a disclosure before animating its outer
viewport. The application retains measured bounds through `cui_track_bounds`.

`cui_box.inert` preserves layout and painting while suppressing semantics, hover,
routed events, focus requests and modal scope effects throughout the subtree,
including portals. It supports visible exit animations without leaving closing
controls interactive. `layout_only` implies inert behavior and also suppresses
painting. A hidden/inert subtree cannot dismiss an enclosing active focus scope.
