# MessageScroller exact offset and following-tail observers

Include `cui/cui_message_events.h`. `cui_base_message_scroller_events` and
`cui_message_scroller_events` append a borrowed `cui_message_scroller_handler` to
the old arguments. Their result carries the exact old return in `control_result`,
`valid`, independent `offset_changed`/`tail_changed`, delivered zero/one
`offset_change_count`/`tail_change_count`, and `count_overflow` (always false).

Offset payload `cui_message_offset_change` contains metadata and double
`previous,value`. Tail payload `cui_message_tail_change` contains metadata and bool
`previous,value`; tail means the model's following policy, not proximity, rendered
visibility or receipt of a message. Exact double inequality determines offset
change; the existing reducer's tail tolerance remains unchanged. Float scrollbar
and semantic coordinates do not replace the double snapshot.

Each wrapper snapshots before any legacy maintenance, including Styled width
remeasure and Base previous-row-bound reconciliation/layout. It executes the old
operation exactly once and freezes both payloads after completion. Offset callback
runs first, then tail callback if each changed and its handler exists. Styled
observers run after legacy row/jump-content callbacks, tracked-bound registration,
paint and box closure. Base observers run after the complete old reducer. There is
no nested Base observer, second layout call or new allocation/listener storage.

`control_result.changed` includes row measurement and opacity animation; it does
not gate these observers. Offset can change during reconciliation before the
legacy reducer's local old-offset snapshot. Conversely, animation-only changed,
wake scheduling, or total/window changes with stable offset/tail emit nothing.
Tail can change while offset stays zero. Both payloads describe the same completed
operation, independently of callback activity on unrelated application state.

Metadata is CHANGE, UNKNOWN source, with genuine nonzero frame only. Aggregate
layout/measurement/wheel/key/bar/jump effects do not establish one input cause,
index or activation. Non-submission entry rejects before work. Ordinary admitted
inert/layout-only frame-zero maintenance preserves legacy results and change flags
but suppresses callbacks. Missing handlers also leave flags intact and counts zero.
Invalid legacy operations emit nothing; unsupported nonfinite internal offsets do
not generate fabricated previous values. Omission retains no handler.

Explicit reset/splice/append/prepend/measure/remeasure/to-item/to-end/scroll/layout
operations remain synchronous and result-only. Their changes before wrapper entry
are the new baseline, without a delayed event on the next draw. Changes caused by
maintenance inside the observed call are included with UNKNOWN origin. Repeated
submissions observe each call independently; result reads never replay callbacks.
Choose results or observers for application side effects.

Payloads are pointer-free values: copy by assignment into caller storage and use
the copies after callback, frame or context destruction. This does not extend
active model lifetime. Handler/user are borrowed only through return. New observers
may copy payloads and mutate unrelated application data, but cannot mutate/free
active state, records, options/theme or input, recursively submit or destroy
context. Existing legacy renderer/jump callback behavior and restrictions remain
unchanged. State and collection must still agree and remain stable until
`cui_end_frame` completes because tracked bounds write into live state then.
Defer teardown accordingly. Later bounds reconciliation does not retain handlers.

There is no data-received/read, renderer completion, network, GPU, capture/bubble,
veto, context-destruction or foreign-language callback API. VirtualList's pending
window contract is separate; MessageScroller does not gain a deferred finish.

`tests/unit/test_message_events.c` compares legacy and observed Base/Styled results,
row counts, consumed input and exact offsets through anchor/layout/width changes.
It exercises double offsets beyond float precision, tail-only changes, frozen
callback order, animation-only silence, passive/missing/invalid/omitted calls and
copied payload lifetime, and runs existing message regression assertions. Native
execution uses `python scripts/test_local.py --build-dir build -- -R message`.
