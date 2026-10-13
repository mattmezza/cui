# Badge

`cui_badge_ex` attaches a count, dot or SVG icon to arbitrary child content.
Submit it every frame. The application owns the count and child actions; Badge
has no retained model. The corner marker does not change the child's measured
size or add a keyboard focus stop. A count of zero hides only the marker.

```c
#include <cui/cui_components.h>

typedef struct {
    const cui_theme *theme;
    size_t unread;
    bool opened;
} inbox_state;

static void draw_inbox(cui_context *ui, void *user) {
    inbox_state *inbox = user;
    if (cui_button(ui, cui_id_from(ui, "open-inbox"), inbox->theme,
                   cui_string("Inbox"), false))
        inbox->opened = true;
}

void draw_badged_inbox(cui_context *ui, inbox_state *inbox) {
    cui_badge_options options = cui_badge_defaults();
    options.count = inbox->unread;
    options.name = cui_string("Unread messages");
    cui_badge_ex(ui, cui_id_from(ui, "unread"), inbox->theme,
                  options, draw_inbox, inbox);
}
```

The defaults select the count variant, medium size and maximum 99. A count
above the maximum displays `99+`; setting maximum to zero deliberately displays
`0+` for positive counts. Full `size_t` counts are supported without allocation.
For a presence dot, set `variant = CUI_BADGE_DOT`. For an icon, set
`variant = CUI_BADGE_ICON` and `icon = cui_string("check")`. Icon names and paths
use the same configurable theme SVG loader as other CUI controls; applications
can supply their own assets. Set `custom_color = true` and `color` to override
the marker background, including transparent colors.

The four control sizes match the frozen source: icon diameters are 10px for
extra-small/small, 16px for medium and 24px for large; the dot is always 6px.
Count text is 10px with 2px padding and a 14px minimum width. The number's corner
offset changes with the size and displayed digit count. An icon sits at the
bottom right with a 1px border in the theme background; count and dot sit at
the top right. Ancestor clipping applies. A supplied `name` provides the status
semantic name; otherwise count text, “Notification” or “Status” is used. Name,
icon and callback data are borrowed during submission. Invalid variant, size or
nonempty null strings submit nothing and do not invoke the content callback.

The older `cui_badge` draws a standalone text pill. Use it for tags such as
“In review”; use `cui_badge_ex` for an indicator attached to content.

Headless tests cover first-frame layout, all sizes/variants, overflow text,
custom colors, icon draw commands, nonfocusable status semantics, zero-count
content preservation and invalid-input callback isolation. Arbitrary child
styles remain ordinary Core composition; there is no global text-style scope.
