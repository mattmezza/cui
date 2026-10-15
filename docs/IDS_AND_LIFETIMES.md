# IDs and lifetimes

The declarations and comments in `include/cui/cui.h` are authoritative. Layout,
semantics and interaction share stable application identity across frames.
Nonzero layout IDs must be unique within a frame; semantic IDs must also be
unique. A semantic control normally owns the box with its matching ID. Zero is
an anonymous layout ID, not an interactive identity. Duplicate public IDs and
collisions in the internal layout ID mapping report errors.

For repeated rows, scope by a persistent record key, not its display position:

```c
if (cui_push_id(ui, message_key)) {
    cui_begin_box(ui, cui_id_from(ui, "row"), (cui_box){.width = -1});
    /* Submit this record's children using local keys such as "subject". */
    cui_end_box(ui);
    cui_pop_id(ui);
}
```

Keep `message_key` distinct for each record. Reordering records then preserves
identity. Generating an ID does not open an ID scope; nesting boxes does not
replace explicit scoping. Balance only successful `cui_push_id` calls.

| Data | Required lifetime |
| --- | --- |
| Application values, editable buffers, controlled models | Across frames while used; application owns them |
| Submitted text and semantic names | Copied into bounded frame storage during submission |
| Draw list and semantic snapshot | Borrowed until the next `cui_begin_frame`; render/read first |
| Bitmap pixel data | Borrowed until renderer completion; update its revision when changing cached content |
| Callback user data | Alive whenever the callback can run; follow each component's contract |
| Font and host/context objects | Font alive during measurement/rendering; destroy in reverse ownership order |

Queue host events before beginning the frame. Interaction targets and bounds
queried during submission come from the previous completed frame; the first
frame has no targets. Completed semantics describe current layout. Render the
completed draw list before beginning another frame. Do not retain pointers into
frame strings or snapshots as durable app state. Component models and optional
resource/session APIs have additional ownership rules in their public headers.
