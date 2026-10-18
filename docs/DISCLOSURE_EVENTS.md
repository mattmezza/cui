# Disclosure applied transitions

Include `cui/cui_disclosure_events.h`. `cui_base_collapsible_events` and
`cui_collapsible_events` add a synchronous observer to the existing owned bool.
They preserve the old return in `result.changed`; `change_count` counts actual
observer calls and saturates at SIZE_MAX with `count_overflow`. A missing observer
still permits editing and ignores its user pointer.

```c
static void expanded(void *user, const cui_disclosure_change *change) {
    bool *needs_save = user; /* unrelated application state */
    *needs_save = true;
    /* Copy *change here if needed after this callback. */
}
/* During the normal UI submission: */
/* cui_collapsible_events(ui, id, theme, label, &open, options, panel, panel_user,
                         (cui_disclosure_handler){expanded, &needs_save}); */
```

Each real bool write emits CHANGE immediately afterward. Two toggles in one frame
emit twice even when the final bool equals its initial value. `previous` and
`expanded` describe applied values, never a rejectable request. Collapsible uses
item_index 0. Notification precedes final expanded semantics and Styled header,
panel and motion work; those retain their original timing. There is no notification
for animation, passive/layout-only submission, disabled input or absent observers.

Metadata targets the compound identity. When Core supplies accepted activation
evidence, it is copied before the write; `event.activation.target` retains the
original activation identity. Otherwise only actual frame/input-index/type-source
facts are copied. Presence flags define what is available; no cause is invented.
Payloads contain no borrowed strings/pointers and may be copied by value.

The callback and user pointer are borrowed for the operation. The observer must
not mutate/rebind/free expanded state, active items/motion/style storage, input or
context, recursively submit UI, or destroy resources. Unrelated flags may change.
Defer omission, removal and teardown until the outer call returns and the frame's
renderer has consumed retained resources. Existing header/panel callback contracts
remain unchanged. This C facade adds no bridge or language-binding support.

## Accordion

`cui_base_accordion_events` takes an explicit nonzero compound target;
`cui_accordion_events` uses its container ID. Item IDs and required pointers must
be valid before admission; invalid required arguments return a zero result without
calling the legacy operation. Existing unique/stable ID preconditions apply.

`item` and `item_index` identify the affected model slot. Opening in single mode
closes each true sibling in ascending index order, including a disabled-open
sibling, then opens the chosen item. Already-false writes stay silent. Each callback
sees this intermediate array; there can temporarily be no open item or several
remaining old items. No normalization occurs just because input starts multi-open.
Multiple mode only changes the chosen slot. Noncollapsible open activation is silent.

The copied activation still identifies the originating trigger, even when keyboard
navigation redirects the reducer cursor or single-open policy closes another item.
`event.target` remains the compound. Notifications precede the existing final
semantic pass: Button checked values reflect initial registration and expanded
details have not yet been finalized. Do not infer finalized semantics, panel paint
or motion from a callback. No observers run for spring updates or passive cleanup.
