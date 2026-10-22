# DropdownButton metadata forwarding checkpoint

2026-10-22. Design only at this commit; CUI base `5a0d1165`, frozen source
`87d10ae5e1299d1be18670a37570c5402f952e34` (v0.7.1). All 78 entries remain.

| Contract | Exact evidence and present gap |
| --- | --- |
| Inner action retains Button metadata | `crates/component/src/button/dropdown_button.rs:70-80,167-189` moves the supplied Button and changes only explicit outer properties; its name/tab/toggled settings survive. CUI's legacy options have no equivalent forwarding fields. |
| Explicit accessible name, including empty | `button/button.rs:391-394,737-740,805-808`: an optional explicit name wins over the visible label. CUI currently passes the same action name into Base and visible paint; a custom body does not change that name. |
| Signed order and independent eligibility | `button.rs:230-231,281-282,472-481,817-818`: default index 0, tab stop true; an explicit signed index does not replace tab-stop policy. Existing CUI Button uses `custom_tab_index`, `tab_index`, `skip_tab` with Core setters; `test_button_tab_order_source.c` covers negative ordered indices independently of eligibility. Split controls currently expose neither. |
| Toggle is metadata, not selected paint | `button.rs:490-498,809-815` forwards optional true/false `aria_toggled`; source selected does not imply toggled. CUI Button currently maps `toggle_semantics && toggled` to semantic checked/value, independently of selected. Dropdown does not expose that convention. |
| Genuine end-to-end toggle gap | `cui_semantic` has checked/value but no optional button-toggled presence. `backends/accesskit/cui_accessibility.c:183-186` emits Toggled only for check/radio/switch roles, not Button. Forwarding existing checked/value cannot establish source `Some(false)` versus `None`, or end-to-end pressed-button AT output. No role substitution or Core/adapter change is proposed here. |
| Trigger | Source `dropdown_button.rs:193-219` constructs a new internal Button with default tab/name/toggle properties. Independent trigger name/tab overrides are a useful explicit CUI extension, not inherited action properties. Existing CUI menu_name remains its legacy default. |

## Bounded additive contract

Proposed independent `cui_dropdown_button_metadata.h`, leaving legacy option/state
layouts unchanged:

```c
typedef struct {
    bool custom_name;
    cui_str name;
    bool custom_tab_index;
    int tab_index;
    bool skip_tab;
} cui_dropdown_button_part_metadata;
typedef struct {
    cui_dropdown_button_part_metadata action, trigger;
    bool toggle_semantics, toggled; /* action checked/value convention only */
} cui_dropdown_button_metadata;
```

Add `cui_dropdown_button_with_metadata_ex` and
`cui_dropdown_button_with_metadata_events`: existing Styled argument sequence,
then optional `const struct cui_dropdown_button_styles *`, optional metadata
pointer, and (for events) the existing handler record. NULL/zero metadata keeps
the exact legacy registration, semantic values, layout, paint and callback path;
NULL styles independently keeps legacy paint. Existing functions remain unchanged.
The metadata header includes only Choice and forward-declares the style tag;
the existing style typedef gains that named tag without changing its fields or
layout. This avoids a Base-to-Components include dependency and adds no ABI size.

Names are independent of the visible label/custom body. `custom_name=true` with
zero-length name explicitly clears the accessible name, including NULL+0.
Active custom names require valid UTF-8 with no embedded NUL, using the existing
checked string validator. Nonempty NULL and malformed UTF-8 reject before state,
handlers, reduction or paint. Unmasked name/index values are ignored. Validate
only visible halves: hidden-half records are ignored and need no string lifetime.
All native `int` indices are accepted, including negative values; this is the
existing portable CUI ordering range, not an arbitrary source `isize` range.
`skip_tab` independently removes the half from Tab navigation; pointer and
accessibility activation remain governed by existing disabled/loading policies.
Selected, expanded, checked/value and busy stay separate. Action checked/value
is `toggle_semantics && toggled` / 1-or-0, exactly the existing Button convention;
it does not toggle the application model and does not close the AT presence gap.
Trigger checked/value remains its existing false/0; menu expanded/controls and
loading description/busy must survive.

## Ordering, private seams and lifetime

Copy the two descriptors at submission entry; names remain borrowed for the
submission, and Core copies semantic name bytes into its frame arena. No new
retained owner, observer, callback contract or dispatcher is introduced.
Preflight occurs before `state->drag_scroll` and existing Base reduction.
Semantic name overrides enter the original registrations, while paint retains
the original labels. Apply action tab/checked metadata immediately after its
existing Button registration and loading details, before existing action-event
enumeration. Apply trigger tab metadata immediately after its original popup
trigger registration, before menu command callbacks or the closed-menu return.
Thus existing handlers can inspect the current metadata without changing their
order, multiplicity, accepted evidence, bool result or mutation privileges.

This needs a narrow optional metadata parameter in the private Base Dropdown
reducer and the shared private menu-trigger registration seam. Every existing
menu/Choice caller passes NULL and retains its exact path; only the new Dropdown
entry supplies the trigger record. A small private header shares the internal
submission declaration; it adds no public Base entry or second reduction.
Outer actions, menu handlers and custom body callbacks may mutate owner/options
as before; copied descriptors prevent those mutations from changing this
submission. Pointer/key/AT targeting and dynamic tab changes use existing Core
previous-frame routing/focus-generation rules; no retroactive rerouting is added.

## Proof and integration

- Independent legacy/zero draw, pixel and memberwise semantic oracle; unchanged
  action/menu result counts, callback order and body-once behavior.
- Explicit named/empty/localized/custom-body labels; malformed/NULL/NUL inputs,
  omitted halves, borrowed-name copying and descriptor mutation in legacy handlers.
- Real Tab/Shift-Tab with signed order and skip independently; pointer/touch,
  Enter/Space, accessibility activation, live disabled/loading, selected versus
  checked/value and open-menu focus restoration. Inspect metadata inside handlers.
- Existing Choice/Dropdown/style regressions, strict C11 guide/C++ declarations,
  private Release wrapper, pinned WASI, standalone legacy/new/CUI_ALL definitions
  and runtime proofs, metadata human mirror and explicit foreign-builder omissions.
- Register and regenerate SDK/dist/skill only for the bounded shipped API. No web,
  workflow, Editor, Button implementation, Core, AccessKit or role changes.

Full source toggled presence/pressed-button AT output remains a separate required
Core/adapter design, not a completed or permanently exempted feature.
