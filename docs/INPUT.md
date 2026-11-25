# Input behavior and presentation

`cui_input_ex` combines the editor with an application-owned `cui_input_state` and options for size, width, placeholder, appearance, text alignment, disabled, readonly, required, invalid and an optional synchronous `cui_platform` clipboard interface. The convenience `cui_input` supplies defaults. Retained state preserves horizontal scrolling and pointer selection across frames. Applications own the editor and buffer.

Single-line inputs paint a caret and a selected-text background, clip text to the viewport, and adjust horizontal scrolling to keep the caret visible. Pointer placement measures UTF-8 scalar prefixes using the same configured font measurement as layout. Shift-click extends selection and captured dragging continues across frames. Secondary buttons do not mutate selection. Measurement is based on logical text order; shaping clusters, bidi and grapheme boundaries remain separate unfinished work. The renderer paints one unbroken text node with a selection underlay, preserving the same text measurement across selection boundaries. The legacy multiline rendering path still lacks caret painting, pointer placement and vertical movement; use the independent Textarea module for its bounded multiline engine.

In single-line Input, Left/Right move by grapheme, Ctrl+Left/Right and Alt+Left/Right move by word, and Home/End move to the value edges. Shift extends the selection. Up/Down leave the caret unchanged. ComboBox and other composites can delegate those keys to a list using `delegate_up_down`.

The drawing-free `cui_base_input_ex` accepts a host-defined pointer-to-byte-offset callback and reduces pointer, keyboard, text and clipboard events in order. Readonly fields allow navigation, selection and copying while rejecting mutations. Disabled fields ignore input. Semantics carry readonly, required, invalid and multiline states. Validation flags are controlled by the application; this API does not infer validation rules.

Control/Meta+C copies the selection; X removes it only after a successful copy; V validates and inserts the returned UTF-8 text atomically. Missing/rejected clipboard access reports `CUI_EDIT_CLIPBOARD_UNAVAILABLE`, preserving the text. Control/Meta+Z, Shift+Z and Y use bounded editor history. `cui_edit_apply_event` lets other compound inputs preserve their own event ordering. Browser clipboard permissions and asynchronous integration belong to the browser adapter. The separate [IME guide](IME.md) covers preedit, commit and host caret geometry.

`tests/unit/test_input.c` checks UTF-8 clipboard round trips, copy failure, invalid paste, readonly and disabled behavior, semantic state, ordered undo/redo, variable-width pointer selection, captured dragging, all four themes, caret/selection draw geometry and horizontal scrolling. Its base-only target links core/base/editor without styled or theme objects. Existing editing property tests continue to exercise history and UTF-8 validity.

`cui_input_paint` exposes presentation without reducing events or registering a second textbox semantic. Compound components can run their own reducer once and reuse the caret/selection appearance. `cui_input_viewport_id` and the drawing-free `cui_base_input_hit_test` geometry helper let their reducers share measured pointer positioning. Tests prove painting alone leaves queued edits untouched and composition inserts each character once.

## Source sizes and fixed-height centering

`cui_input_options.size` defaults to `CUI_INPUT_MEDIUM` (zero), with SMALL, LARGE, XSMALL and CUSTOM choices. `cui_input_measure` exposes the exact renderer metrics before layout. Presets use `density` when positive, otherwise `theme.font_size / 16`:

| Size | Height | Font | Horizontal padding | Vertical padding | Line box |
| --- | ---: | ---: | ---: | ---: | ---: |
| XS | 20 | 12 | 4 | 0 | 20 |
| S | 24 | 14 | 8 | 2 | 20 |
| M | 32 | 14 | 10 | 8 | 20 |
| L | 44 | 16 | 12 | 10 | 20 |

Custom size sets the height directly and font to `custom_size * .875`, with horizontal/vertical padding 8/2 at the selected density. A positive `height` refines the frame height; `custom_height` also allows exact zero. Invalid/nonfinite metrics are rejected before interaction. The horizontal text inset includes the one-pixel border when present. The source single-line child has a fixed 20px line height and its row frame centers it on the cross axis: Medium's vertical padding does not enlarge its 32px frame. Actual glyph measurement is centered inside that line box. Numeric and custom compound renderers can reuse this through `cui_input_paint`.

`name` labels the semantic node and is no longer painted as a second row. This corrects the previous Input appearance. `no_appearance` suppresses the frame surface/border; `no_border` keeps the surface. Focus/invalid border colors remain separately controlled. There is no outer source focus shadow yet. `text_align` supports Start, Center and End. Both growing and fixed-width fields align text in the current layout pass. Selection uses `theme.selection` without changing the foreground, and the caret uses independent `theme.caret` with source height 85% of the line box. Disabled appearance uses input background at 20% alpha and muted foreground at 50% alpha. Complete bidi caret and selection geometry and native context menus remain open.

Growing single-line inputs now align text through the current layout pass. Their inner content no longer uses a previous-frame viewport width as a minimum, so changing parent width or horizontal padding does not force a stale viewport size. Headless tests cover left, center and right alignment on the first frame and after shrink/grow transitions with changing padding; the regression fails against the previous implementation.

Single-line inputs render both their value and placeholder as one unwrapped line,
including when the host supplies only a measurement callback. The content viewport
clips overflow; focused values scroll horizontally to keep the caret visible.
A long placeholder does not increase the input height or wrap into extra lines.

Growing single-line fields (`width = -1`) constrain long populated values to the
resolved frame width on the first frame and after parent resizing. The outer and
text-viewport clips remain active; InputGroup prefix/suffix segments retain their
widths. An extra internal layout row provides this containment.

### Follow the current parent width

Set `width = -1` inside a parent sized by your layout. During a resize, the
viewport uses its current width for horizontal scroll, selection paint, the
caret and the IME anchor. Long values scroll from the start edge. Center and
End take effect when the text fits. A placeholder aligns by its own width.

```c
/* Within the sized parent, during submission. Retain edit and state. */
cui_input_result result = cui_input_ex(ui, input_id, &theme, cui_string("Name"),
    edit, &state, (cui_input_options){.width = -1});
/* After cui_end_frame, if your application needs the actual scroll offset. */
float painted_scroll;
bool has_painted_scroll = cui_input_scroll_x(ui, input_id, &painted_scroll);
```

The optional query returns true for the growing single-line viewport in the
latest successful frame, including a field composed by InputGroup. It leaves
its output untouched during submission and when the viewport is absent or the
frame fails. Fixed-width fields continue to update `state.scroll_x` at
submission. Growing fields also keep that synchronous update, then reconcile
the last painted offset on the next submission when the supplied scroll is
unchanged. An explicit change to the supplied scroll is honored. The layout
pass retains no pointer to the caller’s state.

Each admitted growing viewport uses one bounded Core resource for this placement.
Resource exhaustion is reported as `CUI_ERROR_CAPACITY`. The C query has no
handwritten Python, Node, Go or Rust facade. The single-line behavior does not
cover same-frame multiline rewrap or animated width overrides. See the
[resize design and regression](design/INPUT_GROW_WIDTH.md).
