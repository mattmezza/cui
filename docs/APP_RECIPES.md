# Practical application recipes

The runnable [mail fixture](../examples/native_bundle/mail.c) uses the same host
as the two-file starter. Build it from the extracted native bundle:

```sh
make APP_SOURCES="main.c mail.c" TARGET=build/mail_demo
./build/mail_demo
```

It demonstrates three independent scroll states, scoped repeated avatars/tags,
clipped long subjects, a fixed trailing star action, message selection and a
Compose/Discard preview. It has no message editor, storage, network or delivery.
It uses a desktop pane layout; narrow windows clip rather than switch navigation.
The packaging test drives real Core pointer activation for Compose/Discard,
selection and starring, then builds and captures the native executable on Xvfb.

For a mail row, cap the growing subject column as well as clipping it: an
unwrapped child's minimum width can otherwise push the timestamp/star beyond
the viewport. Keep the action column fixed. Word-wrap the reader body inside a
width-constrained scroll viewport. Give each pane its own `cui_scroll_state`.
Use `cui_scrollable_end_ex` only after a successful `cui_scrollable_begin`.
The [layout guide](LAYOUT_GUIDE.md) is the canonical sizing reference.

Use stable record keys and local IDs for repeated content; see
[IDs and lifetimes](IDS_AND_LIFETIMES.md). Use exact icon registry names from
`cui_icon.h`, check lookup results, and retain text labels for actions when an
icon is unavailable. Emoji are not a portable substitute for missing artwork.

## Sidebar custom content callbacks

`cui_sidebar_content_fn` has **four arguments**, with the user pointer last:

```c
static void folder_label(cui_context *ui, const cui_theme *theme,
                         bool collapsed, void *user) {
    const char *label = user;
    if (!collapsed)
        cui_text(ui, cui_string(label), 14, theme->sidebar_foreground);
}
```

Assign it to `label_content` and put the live label pointer in `label_user` on a
`cui_sidebar_menu_item`. Its activation callback is a different signature:
`void on_activate(void *user)`. The menu's top-level `user` feeds activation,
while `label_user`, `icon_user` and `suffix_user` feed their respective content
callbacks. Do not cast a three-argument function to the content callback type;
use the exact typedef in `include/cui/cui_sidebar.h`. Content callbacks submit
children; they do not confer generic click handling on those children.
