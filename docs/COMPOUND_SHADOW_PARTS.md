# Compound paint identities

The additive C getters in `cui_content.h`, `cui_message.h`, `cui_status.h` and
`cui_media.h` expose the existing identities of compound parts. They do not submit,
open, allocate or query the presence of a box. Zero owners/scopes, invalid enums,
invalid indices and null scroller state return zero without changing diagnostics.
A valid identity may belong to an omitted, offscreen, layout-only or custom part.

Submit the component first, then call `cui_element_set_box_shadows` before
`cui_end_frame`, outside prepaint, measurement and diagnostic callbacks. Check the
setter's result: previous `cui_bounds` are not proof of current-frame presence.
The setter copies the layers; normal Core limits, clipping and failure behavior
apply. Getters preserve legacy emitted IDs, including the legacy painters' separate
zero-owner behavior. Existing action getters retain their original zero handling.

| Component | Getters and actual owners |
| --- | --- |
| Composed Bubble | `cui_bubble_part_id`: SURFACE is the painted bubble, REACTIONS the conditional reaction container. The caller ID is the outer row. For direct `cui_begin_bubble`, the caller ID itself is the surface; the composed getter does not apply. |
| Message | `cui_message_part_id`: AVATAR, HEADER, FOOTER. `cui_message_body_id(owner,index)` accepts indices below 256. A typed body owns its bubble surface; an untyped body owns its slot row. Optional/custom callbacks may omit boxes even when the namespace is valid. |
| MessageScroller | `cui_message_scroller_part_id`: VIEWPORT and conditional JUMP. `cui_message_scroller_row_id(owner,state,index)` uses the current retained row key. Prepending preserves retained row identities; removal/reset invalidates assumptions about old indices. State must remain alive. Offscreen rows have identities but no current-frame paint owner. |
| Notification | `cui_notification_part_id(scope,logical,part)`: CARD, CONTENT, ACTION, CLOSE. The logical `{type,key}` is not a paint ID, and the scope is not a submitted card. System-only, removed and fully hidden/layout-only cards cannot receive shadows. ACTION is a custom callback namespace or the default action button; CLOSE is conditional. The existing 128-bit logical-key to 64-bit paint mapping can collide: identity is not uniqueness or presence validation. |
| Attachment | `cui_attachment_part_id`: CARD, MEDIA, MEDIA_OVERLAY, METADATA, ACTIONS, REMOVE, MEDIA_RETRY. `cui_attachment_child_part_id` accepts original child-array indices below 128 and ROW or RETRY. Child RETRY identifies the first eligible description's textual retry, distinct from MEDIA_RETRY. Media overlays/actions may be portals; custom callbacks may emit no box. No synthetic owner is added for absent parts. |

```c
cui_bubble(ui, id, theme, options);
cui_box_shadow shadow = {
    .kind = CUI_BOX_SHADOW_DROP,
    .layer = {.offset_y = 2, .sigma = 2, .color = {0, 0, 0, 80}}
};
bool styled = cui_element_set_box_shadows(
    ui, cui_bubble_part_id(id, CUI_BUBBLE_PART_SURFACE),
    &shadow, 1, (cui_corner_radii){0});
```

These are C identities, not new language-builder targets. Python, JavaScript,
TypeScript, Go and Rust compound MAIN/ROOT/subpart authoring remains pending;
[the design](design/COMPOUND_SHADOW_PARTS.md) describes future mapping without
claiming binding coverage. Use matching canonical headers and rebuilt libraries,
standalone headers or SDKs. No public by-value layout changed.

The 12 focused headless checks pass: the getter/legacy/element-shadow tests,
existing Content/Message/Status/Media and three Base-only regressions, and
selective/umbrella generated-header C11 declaration/link smoke tests. The legacy fixture was recorded and replayed
before rebuilding, against the pre-change shared library with SHA-256
`7438a27c6e37855efe0d3e6009a29e0cad83faa56c1c830097695c700c1dd99b`.
It records 20 scenes across five facades, including zero owners, normalized
commands, layout IDs, semantics and callback order without reproducing ID salts.
The frozen binary-value fixture is limited to 64-bit little-endian hosts; the
getter tests are portable. These headless tests establish no native GPU/browser
rendering or complete component/binding parity.
