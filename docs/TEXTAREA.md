# Textarea

`cui_textarea` is a bounded multiline editor with measured soft wrapping, pointer selection, vertical keyboard navigation, selection highlights, caret painting and a scrolling viewport. It uses a caller-owned multiline `cui_edit` for text, selection and undo history. It is independent of the single-line Input renderer.

```c
char text[4096] = "First line\nSecond line";
cui_edit *edit = cui_edit_create((cui_edit_config){
    .buffer = text, .capacity = sizeof(text), .multiline = true,
    .history_bytes = 8192, .max_history_steps = 64
});
cui_textarea_line rows[256];
cui_textarea_state state;
cui_textarea_state_init(&state, rows, 256);
/* Each frame, inside an open layout box: */
cui_textarea_result result = cui_textarea(ui, id, &theme,
    cui_string("Description"), edit, &state,
    (cui_textarea_options){.width = 320, .rows = 4,
        .placeholder = cui_string("Write a description"), .clipboard = platform});
```

Keep the editor, row workspace and state alive across frames. Each widget needs its own state. Text buffers must remain valid and unchanged until the draw list has been rendered. Destroy the editor when finished. No Textarea allocation or hidden cache is used. Text is limited to 65,536 bytes and the row workspace to 65,536 entries. Invalid inputs return `CUI_TEXTAREA_INVALID`; insufficient measured-row capacity returns `CUI_TEXTAREA_CAPACITY` and suppresses partial painting. An edit may succeed before exhausting visual-row capacity; expand the workspace rather than treating this error as a text rollback. Text-buffer capacity errors are reported separately through `edit_error`.

The zero-initialized options use the source Medium metrics: font 14, line height 20, horizontal padding 10, vertical padding 8, one row, appearance and border enabled. Preset metrics scale by `theme.font_size / 16`. XS/S/M/L fonts are 12/14/14/16; horizontal padding is 4/8/10/12 and vertical padding 0/2/8/10. Custom size uses font `custom_size * .875` and horizontal/vertical padding 8/2 at the same density. Width zero selects the CUI fallback 280; a negative width grows. Positive height overrides row height. Auto-grow defaults to one through ten rows and can be bounded explicitly. It is an optional CUI policy, not the upstream initial mode.

Soft wrapping preserves every byte and prefers whitespace boundaries. LF, CRLF and CR split hard lines; a final newline creates an empty final row. A cursor at a soft boundary has upstream/downstream affinity, so Left/Right can move between the two visual positions without changing the byte offset. Up/Down and Page Up/Down preserve a measured horizontal target; Home/End operate on the visual row, and command Home/End on the whole buffer. Movement reveals the caret. Pointer dragging and Shift navigation extend selection. Clipboard copy/cut/paste use the supplied synchronous platform callbacks; read-only fields retain navigation, selection and copy. Disabled fields reject interaction. External `cui_edit` mutations are detected through revision and selection changes, without reporting them as edits performed by the widget.

Enter inserts a newline and reports `pressed_enter`, `enter_secondary` and `enter_shift`. With `submit_on_enter`, ordinary or secondary Enter reports submission without insertion; Shift-Enter still inserts. These fields describe the last handled Enter in a frame. Read-only and disabled fields do not report submission.

Editable Textarea consumes Tab and Shift-Tab for indentation by default, including repeated key events. `tab_size` defaults to two spaces; `hard_tabs` inserts one tab byte instead. A collapsed Tab inserts at the cursor. A selection indents every touched hard line, including a line where the selection ends at its start. Shift-Tab removes only an exact matching prefix at each line start. An unchanged outdent creates no history step. Each successful operation is one undo step that restores the original selection; redo restores the mapped endpoints. Capacity failure leaves text, history and selection unchanged. Set `no_indent` to use normal Tab focus traversal. Read-only and disabled fields also retain normal focus traversal.

The semantic node carries a Textbox role, multiline, read-only, required, invalid, disabled and selection/scroll details. `name` labels the semantic node; it is not a visible label. A custom role and removal from Tab stops are available. The theme's input background/border, text, muted, selection and independent caret tokens are consumed. Reduced motion gives a steady caret. Otherwise focused fields invalidate for caret blinking.

## Remaining source gaps

This is a scalar UTF-8 editor, not the upstream shaped editor. Grapheme-aware movement, word navigation, bidi shaping and cluster hit testing, IME preedit, inline atomic tokens, multiple cursors, language/editor features, rich paste interception and native context menus remain unsupported. Numeric Tab ordering and external accessibility IDs also require shared APIs. No parity claim is made for these behaviors.

Grow-width wrapping uses the previous tracked viewport, with a 280-pixel initial fallback; same-frame intrinsic width changes can therefore require another frame. Placeholder painting uses the core text renderer rather than an independently measured multiline placeholder workspace. There is no source focus shadow or independently configured line-height option. Explicitly sized fields avoid the initial grow-width measurement limitation.
