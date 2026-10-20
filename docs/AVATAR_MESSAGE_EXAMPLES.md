# Profile images and compact conversations

Avatar now accepts a borrowed RGBA bitmap through `cui_avatar_ex`. The legacy
`cui_avatar` initials API is unchanged. The gallery uses three original fictional
portraits, processed by the pinned `scripts/generate_avatar_assets.py` script.
The same masked pixels enter the native and WASM draw lists; no DOM image is used.

Initialize a caller-owned cache once, outside the frame loop. Retain the source
pixels, cache object and destination storage until rendering has completed.
Changing the source pointer, dimensions, stride or revision rebuilds the cache.
Increment the source revision when its pixels change. There is no per-frame heap
allocation and no cleanup function: the application owns all storage. A diameter
of 1–512 is supported; storage requires diameter × diameter × 4 bytes. Source and
cache storage must not overlap. Missing or invalid images fall back to initials. A
failed preparation leaves the last successful cache intact, but `cui_avatar_ex`
uses the current preparation result and never displays that stale photo for an
invalid source. To represent acquisition/decode failure or a pending download,
pass a zero bitmap; retry with valid borrowed RGBA pixels when available. Recovery
can reuse the same cache. Acquisition, decoding and retry policy belong to the
host; there is no network or asynchronous source-status API.

The supplied full name labels both photo and initials frames; it is not shortened
for assistive technology. An empty name becomes “Anonymous” in `cui_avatar_ex`.
Passive avatars expose image semantics and interactive avatars expose button
semantics. Read semantic records within the current frame lifetime; copy names
before retaining them across frames. These names are a CUI accessibility policy:
the frozen upstream Avatar does not define an equivalent automatic name policy.
See `docs/AVATAR_FAILURE_AUDIT.md` for the precise frozen upstream distinction.

```c
#include <cui/cui_content.h>
static unsigned char circle_pixels[96 * 96 * 4];
static cui_avatar_image circle;

bool profile_init(void) {
    return cui_avatar_image_init(&circle, circle_pixels, sizeof circle_pixels, 96);
}

bool profile_draw(cui_context *ui, const cui_theme *theme, cui_bitmap borrowed) {
    return cui_avatar_ex(ui, cui_id_from(ui, "profile"), theme, cui_string("Alex Morgan"),
        (cui_avatar_options){.size = 48, .ring_width = 2,
            .image = borrowed, .image_state = &circle, .interactive = true});
}
```

`cui_avatar_stack` takes up to 64 items with stable application keys. Its default
shows three portraits and a passive overflow count. Later portraits paint and
receive pointer hits above earlier portraits. Positive `width` reduces the number
shown to fit; zero uses the intrinsic width. Interactive stacks return the selected
item index, or `SIZE_MAX`. Disabled items retain their semantics and reject
activation. Cursor and interaction state use the shared Core hooks. The image name
is its accessible name; interactive portraits use button semantics.

Message separates avatar, header, body and footer callbacks. Put sender and time
in the header, one message in the body, and delivery/reaction controls in the
footer. The gallery demonstrates incoming and outgoing rows, portrait callbacks,
controlled acknowledgement and an actual reaction callback. `max_bubble_width`
caps the content column while preserving the available row width and START/END
alignment. Zero preserves the existing layout. The footer inset uses the actual
avatar width and row gap. MessageScroller uses the same row composition with
stable IDs and a virtualized conversation model. Slots are inferred present from
`present`, nonempty text or a render callback; omit a slot by clearing all three.
Scroll deltas that change MessageScroller position are consumed, allowing an
ongoing touch pan to remain internal; refused boundary deltas can reach the host.

```c
#include <cui/cui_message.h>
void conversation_row(cui_context *ui, cui_id id, const cui_theme *theme,
                      cui_message_options slots) {
    cui_message_body body = {
        .slot = {.text = cui_string("Can you review the release notes?")},
        .typed_bubble = true, .bubble = CUI_BUBBLE_SECONDARY};
    slots.header.text = cui_string("Alex Morgan · 09:30");
    slots.body = &body;
    slots.body_count = 1;
    slots.footer.text = cui_string("Delivered · 09:30");
    slots.width = 600;
    slots.max_bubble_width = 280;
    slots.alignment = CUI_MESSAGE_START;
    slots.name = cui_string("Message from Alex");
    cui_message(ui, id, theme, slots);
}
```

The reusable API accepts pixels supplied by the application; it does not decode
PNG files or manage image downloads. Applications own conversation/session state
and callback lifetimes. These examples do not implement a network messaging
service, attachments or a complete messaging accessibility model.

Validation includes headless cover-crop/alpha/cache/revision checks, native
software rendering, overlap hit order, keyboard activation, disabled and cancelled
touch activation, and actual compact message bounds in both alignments. The
Chromium suite uses CDP touch contacts for profile selection, reactions and virtual
scrolling, checks actual WASM portrait pixels and circular alpha, all seven bubble
variants and 390px layouts. This is Chromium evidence, not physical iOS/Android
or full platform parity.
