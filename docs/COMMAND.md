# Command

Command is an inline command palette, composed from the real persistent Input,
virtual rows and a Scroll control. Rows fill the list inside its horizontal
insets and any visible scrollbar gutter. An application can host Command in a
Dialog or Overlay. This module follows the pinned GPUI 0.7.1
`component/src/command/{command,item,state}.rs` contracts; it does not supply a
native application Action registry or automatically create a modal dialog.

```c
char query_buffer[256] = {0};
cui_edit *query = cui_edit_create((cui_edit_config){
    .buffer = query_buffer, .capacity = sizeof(query_buffer)});
cui_command_row workspace[64];
cui_command_state state;
cui_command_init(&state, workspace, 64, query);

cui_command_item settings[] = {
    {.label = cui_string("Profile"), .icon = cui_string("user")},
    {.label = cui_string("Billing"), .disabled = true},
};
cui_command_entry entries[] = {
    {.kind = CUI_COMMAND_ITEM,
     .item = {.label = cui_string("Calendar")}},
    {.kind = CUI_COMMAND_SEPARATOR},
    {.kind = CUI_COMMAND_GROUP, .heading = cui_string("Settings"),
     .items = settings, .item_count = 2},
};
cui_command_options options = cui_command_default();
options.label = cui_string("Commands");
cui_command(ui, id, &theme, &state, entries, 3, options);
```

Initialize once, submit each frame, and destroy the editor after the state is no
longer rendered. Workspace and editor remain application-owned. Model-only use
can initialize with a null editor. The base source links without Core, Theme or
Edit implementations, and allocates nothing.

Entries, strings, keyword arrays and callback users are borrowed until the next
rebuild. Keep them alive through rendering and any direct model operation such
as `cui_command_confirm`. Rebuilding validates the worst-case unfiltered row
capacity before writing; include headings and every supplied separator in that
capacity. The module limits workspace and supplied items to 4096. Bad model or
capacity leaves the existing model unchanged. Measurement must produce finite,
positive outer row heights no greater than one million logical pixels.

Default filtering trims Unicode whitespace, lowercases the full UTF-8 query,
label and keywords using Unicode 17, and matches a substring. For example,
` PRÉF ` finds `Préférences`. In the gallery, try `ACCÈS` to find **Copy path**
via its French keyword. Combining marks and accents are significant.
For fuzzy matching, normalization, full case folding, or locale-specific
search, supply `options.matches`. Custom matchers still receive the original
query bytes after the legacy C `isspace` trim. Query callbacks receive the
raw query in either mode. All matching is allocation-free. Malformed UTF-8
bytes are treated literally at invalid scalar boundaries in the model helper.

Turning off filterable keeps all rows but still reports searchable query
changes. Turning off searchable hides the Input, retains all rows and suppresses
query callbacks, including for a programmatically modified hidden editor.
The built-in rows in the language bindings use the same matcher as their C
provider. The Unicode behavior requires a provider built from the updated source.

Original indices survive filtering: all ungrouped items use section zero in
input order. Groups use their group ordinal, offset by one when any ungrouped
item exists. Empty groups hide their headings. Leading, trailing and repeated
separators are omitted. Disabled items remain visible and count as matches,
but cannot be selected or confirmed.

Up/Down wrap through enabled items. Enter invokes the selected action first,
then the confirmation callback, including for items with no action. Pointer
entry changes the highlight without scrolling; a click confirms. Row pointer
capture is exercised across separate press, movement and release frames.
Keyboard changes request nearest scrolling; a model reinstall preserves an
eligible original path and does not move the viewport. Explicitly cleared
selection remains cleared until navigation or a query change. A changed query
resets to the first enabled result and calls selection before query. Callbacks
run synchronously after the model update; callers may inspect state, but must
not destroy the UI/editor/state or invalidate borrowed model storage while a
render call is in progress. Unlike GPUI deferred callbacks, CUI has no window
lease or liveness object: application dispatch owns that lifecycle boundary.

`action` and `binding` share an `action_user`; the host resolves a live binding
for the same executable action. The default row draws its resolved hint using
Kbd. A binding takes precedence over a checked mark. Custom content owns its
keybinding presentation, and checked custom items still receive the source's
trailing check. Application scope/keymap registration and platform-specific
Action dispatch remain host responsibilities.

Default list maximum is 300 logical pixels. Frame uses popover colors, a
one-pixel border and radius_lg; bordered=false removes that frame. List inset is
4, default rows use horizontal8/vertical6 padding and font14/line20; headings
use font12/line16, and separators occupy9. Theme density scales those metrics.
The default width is 380 logical pixels. Set `options.width = -1` to fill
the available parent width, for example inside a Dialog body. The search field,
list, and row hit bounds follow the current layout on the opening frame and
while resizing. A theme with `CUI_SCROLLBAR_HIDDEN` lets rows use the gutter
space as well.

Built-in labels use a clipped single line. For custom wrapping or variable
heights, supply `measure` and `content` callbacks. The measure callback receives
the available row width and font size and returns the complete outer height.
Inside a fixed-width, unclipped column such as a Dialog body, a growing
palette measures against the current width on opening and resizing. The
scrollbar gutter and list insets still apply. When a short viewport narrows the
scrollbar track, the custom row keeps its measured width by reserving the gutter
at `max_height`. The viewport still fits its content up to that maximum. A hidden
scrollbar reserves no space. Custom headers may extend wider than the list within
the palette's clip.

When a parent still needs to resolve the width, such as a row with other
growing siblings, Command starts with its scaled default width and then uses
the last allocation. The row width follows layout and a follow-up frame
updates custom heights.

When heights change, the virtual
list keeps a previously fully visible selection in view. Scrolling the
selection out of view leaves the viewport under the user's control, subject
to the usual content bounds. Reflow emits no new selection callback. Keep
measurement consistent with the content callback. Every flattened row is measured each submission; only visible
rows invoke content and submit semantic/paint nodes. This has bounded O(n)
measurement and avoids drawing all offscreen children, but does not reproduce
GPUI's typography/action-binding row measurement cache.

Header/footer/empty callbacks receive current state and render arbitrary CUI
content. Loading shows the actual Icon spinner beside Input and suppresses the
empty slot. Placeholder, empty message and accessible label can be supplied by
the application for localization. ListBox/Option semantics expose selection,
disabled state and busy status. Quick-action mode focuses the Command frame;
searchable mode focuses the actual Input. Row options have no Tab stop, while
application-provided child controls retain their own semantics.

For Dialog composition set `cui_surface_options.no_escape_dismiss=true`.
Command consumes the first Escape when a searchable query is nonempty. Empty
query Escape calls `on_cancel` and remains unconsumed; let that callback request
`cui_surface_request_close(ui, surface, CUI_SURFACE_CANCELLED)`. The next surface
submission applies the decision policy, removes the modal and restores trigger
focus. Do not immediately destroy/close the host from an action callback. The
headless suite exercises this actual Surface composition, including the two
Escape presses and focus restoration.

Link the base and component sources into normal CUI. Styled implementation
uses Components/Input, Edit, Scroll, Icon and Content/Kbd. Tests additionally
use Surface/Overlay for modal composition. A base-only test defines
`CUI_COMMAND_BASE_ONLY` and links `src/base/cui_command.c` plus math only.
