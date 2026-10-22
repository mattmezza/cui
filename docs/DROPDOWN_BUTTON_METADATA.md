# DropdownButton names and focus ordering

`cui_dropdown_button_metadata.h` adds independently optional semantic names and signed tab ordering for the action and menu trigger. Existing options/state layouts and legacy entry points stay unchanged. NULL or zero metadata uses the original path; paint remains independent through the optional `cui_dropdown_button_styles` argument.

```c
#include "cui/cui_dropdown_button_metadata.h"

void submit_dropdown(cui_context *ui, const cui_theme *theme,
                     const cui_menu_entry *items, size_t count,
                     cui_menu_tree_state *state, cui_id *command) {
    const cui_dropdown_button_metadata metadata = {
        .action = {.custom_name = true, .name = {"Save document", 13},
                   .custom_tab_index = true, .tab_index = -2},
        .trigger = {.custom_name = true, .name = {"More document actions", 21},
                    .custom_tab_index = true, .tab_index = -1},
    };
    (void)cui_dropdown_button_with_metadata_ex(
        ui, 100, 101, 102, theme, cui_string("Save"), cui_string("More"),
        items, count, command, state, (cui_dropdown_button_options){.width = 180},
        NULL, &metadata);
}
```

Descriptors copy at entry. Custom name bytes are borrowed and immutable through submission; Core copies semantic name bytes into frame storage. Names need valid UTF-8 with no embedded NUL. An explicit empty custom name is supported (NULL plus zero length). Invalid visible custom names reject before input consumption, state changes, callbacks or painting. Hidden halves and unmasked fields are ignored.

`skip_tab` affects keyboard traversal independently of signed native-int ordering; pointer and accessibility activation remain governed by the existing enabled state. Dynamic changes use existing previous-frame routing and focus rules. Metadata is registered before existing synchronous action/menu callbacks, which retain their original mutation and replacement behavior. No deferred callbacks or new dispatch mechanism are introduced. Visible labels/custom children remain unchanged.

`toggle_semantics` applies the existing Button checked/value convention to the action, independently of selected paint. The trigger finishes unchecked (the existing menu callback still sees its transient open/checked state); expanded, controls and busy metadata continue to work. This does **not** provide optional toggled presence or a pressed-button accessibility adapter: that frozen-source contract still requires a separate Core/adapter change. No binding facade is shipped. All consumers must use matching revision headers and library.
