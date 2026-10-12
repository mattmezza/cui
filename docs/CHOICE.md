# Tabs and toggle groups

`cui_choice.h` contains allocation-free, caller-controlled Tabs and ToggleGroup
behavior, plus themed composable renderers. Every item has a stable nonzero ID,
label and disabled flag. Tabs selection uses an ID so reordering the caller's
array does not select a different panel.

```c
cui_choice_item choices[] = {
    {21, cui_string("Account"), false},
    {22, cui_string("Profile"), false},
};
cui_id selected = 21; /* persistent application state */
cui_tabs(ui, 20, &theme, choices, 2, &selected,
    (cui_tabs_options){.variant = CUI_TAB_UNDERLINE}, draw_panel, application);
```

`draw_panel(cui_context *, size_t index, void *)` runs synchronously only for the
selected enabled tab. Invalid or disabled selected IDs are retained as caller
state and display no panel. Applications decide how to recover when removing a
tab. Hidden panels and their children are not submitted, so they cannot remain
in the focus order.

Tabs support horizontal Left/Right or vertical Up/Down navigation, wrapping,
Home/End, disabled-item skipping, click/Enter/released Space selection and automatic
or manual activation. Manual mode moves focus using arrows and requires activation
to select a panel. Default, underline, pill, outline and segmented styles derive
colors and metrics from the supplied theme. The underline variant draws a bottom
line independently of the focus outline. `options.render` paints a custom body
for each tab, retaining its accessible label and interaction. It can compose icons
and content within the theme's control height. `prefix` and `suffix` callbacks add
application controls before and after the tab list in its orientation. Those
controls retain their own IDs and behavior and do not select a tab automatically.
Headless tests exercise custom bodies and both controls in all five variants and
all four themes, including disabled semantics and ordered keyboard selection.

As in GPUI Kit's documented closable-tab example, a custom tab body can compose a
separate small close button beside its label. Give the close button a distinct
stable ID, a meaningful accessible name/description and the tab's disabled state.
Record the requested tab ID during the callback and apply removal after `cui_tabs`
returns; the callback borrows the array. Applications own which neighbor becomes
selected and where focus returns. A close button activates independently of the
tab and can also be focused and activated with Enter. Tests remove an unselected
tab without changing selection, reject a disabled close button, remove a selected
tab by keyboard, verify withdrawn child semantics and apply an explicit empty
selection policy when only disabled tabs remain, across all variants and themes.

ToggleGroup accepts a caller-owned boolean per item. Options choose single or
multiple selection, whether the final selected item can be cleared, orientation,
and group disabled state. Arrow/Home/End traversal moves focus without selecting;
activation applies the selected-state rule. Return values report selection
changes. Callers should initialize state consistent with their selected options;
externally supplied multiple selections are not silently normalized.

The base functions paint nothing. Styled functions call those base functions and
submit theme-dependent boxes, which permits an independent design system without
forking keyboard behavior. Tabs expose tab, tab-list and panel roles with selected state and bidirectional
trigger/panel relationships. The focused (or selected, or first enabled) tab is
the group's single Tab stop; arrow traversal updates it. ToggleGroup exposes a
group and buttons with pressed state through checked. OS accessibility bridges
remain unfinished.

The tab list ID is the container ID XOR `0xcda0312561f50cab`; a panel ID is its
tab ID XOR `0x691a17db80e22f45`. These IDs must not overlap application children.
Labels are frame-copied by the core, and callbacks must not retain draw-list strings.

`tests/unit/test_choice.c` covers actual pointer activation, keyboard modes,
orientation, wrapping, disabled controls, ID-stable reorder, invalid selection,
toggle selection rules, and all five tab variants in all four themes. This is a
portable C11 vertical slice. GPUI Kit's overflow menu, drag reordering, built-in
close-button policy, maximum label width/ellipsis,
and gallery stories remain pending. Composing those controls beside this tab list
is possible, but is not claimed as equivalent built-in support. Full family parity
is not yet verified.

## List, Select and Menu

`cui_dropdown_button` composes two adjacent, independently focusable actions: a
primary button and a caret trigger backed by the hierarchical menu reducer. The
result reports primary activation and menu-item activation separately, and the
caller receives the selected menu action ID. Shared primary, secondary, outline,
ghost and danger variants, three sizes, selection, outer disabled state and custom
primary content apply to the joined control. `action_disabled` and
`action_loading` affect only the primary half, so alternate menu actions remain
available. Callers provide distinct stable container, action, trigger and popup
IDs. `cui_base_dropdown_button` exposes the same split behavior without paint.

The menu trigger owns expanded/controls semantics and restores focus after menu
activation. The primary half never opens the menu, and menu activation never
reports primary activation. Tests cover every variant, size and built-in theme,
joined geometry, accessible names, independent pointer/keyboard activation and
disabled/loading behavior.

The same header also supplies flat selectable lists and anchored popup controls:

```c
cui_choice_popup popup; /* persistent application state */
cui_choice_popup_init(&popup, 80); /* once; unique popup/scope ID */
cui_select(ui, 79, &theme, cui_string("Account"), cui_string("Choose account"),
    choices, count, &selected, &popup,
    (cui_choice_popup_options){.viewport = {0, 0, width, height}, .dt = dt});
```

`cui_list` accepts an application-owned `cui_list_state`. Activation updates the
selected item ID; moving the active cursor does not commit selection. Up/Down,
Home/End wrap through enabled options. Incremental typeahead matches label prefixes
with ASCII case folding and byte-exact matching for other characters; a repeated
single letter cycles matching labels. Pass elapsed seconds as `dt` to reset search
after 750 ms of inactivity. No locale-dependent global state is changed.

Select and Menu use the same base list behavior with distinct semantic roles.
The trigger has an accessible name, expanded state and a controlled popup ID;
Select displays the committed choice label or the supplied placeholder. Menu
returns true for every action activation, including repeated activation of the
same command. Select returns true only when the selected ID changes. Applications
dispatch menu commands themselves.

Opening with click, Enter, released Space or Up/Down moves focus into the popup.
Selection closes it synchronously and restores the enabled trigger. Escape and
outside clicks dismiss without changing selection. The popup is a floating portal,
so a clipping parent does not cut it off. Placement flips/clamps against the
explicit logical viewport; the width defaults to 240 and fits that viewport.
The modal focus scope prevents Tab from reaching background controls while open.
A flat empty popup renders an explicit empty state and remains dismissible.

For long lists, only a bounded window of options is submitted and painted. Arrow
navigation, Home/End and typeahead bring the active item into that window in the
same frame. Wheel events inside the list or popup move this window in row units,
carry fractional deltas across frames, clamp at both ends and respect modal scope
ancestry. Scrollable listbox semantics expose physical row-height offsets and
maximum offsets, preserving selected-set and label relationships. The core routes
wheel events to the deepest eligible list; an inner list at its boundary allows
its enclosing scroll region to receive the next wheel event. Base-only callers
must provide the receiving semantic ID as `scroll_target`, its bounds and scroll
metadata; styled lists supply these automatically. They do not commit selection. If the focused option leaves the window,
focus moves to the first enabled visible option; Combobox keeps input focus while
moving its active suggestion. Disabled lists ignore scrolling. This provides paging
without allocating or registering
thousands of offscreen controls. It is separate from the retained variable-height
VirtualList model. Callers rendering a standalone list can set `visible_count` and
`visible_begin`; `state.first_visible` returns the window after cursor navigation.

List and popup options accept a `render(ui, item, index, user)` row-body callback.
It replaces the default label painting while retaining the item's accessible name,
disabled state, selected state, keyboard behavior and primary pointer activation.
The callback can compose icons, descriptions or other content within the fixed
theme control height. Multiple selection still paints its check indicator. The
index refers to the displayed array, including filtered Combobox arrays; use the
stable item ID to look up source data. The callback must not alter the borrowed
arrays or add separately activatable menu actions.

The popup ID is reserved for its scope and floating root; its internal list ID is
that ID XOR `0xd1bc905ee4216f11`. Item IDs must be globally unique and not overlap
trigger, popup or internal IDs. The API borrows all arrays and state for the call;
labels follow the core's frame-copy lifetime. Initialize state once, not per frame.

The headless choice test verifies selection, repeated menu dispatch, clipping
escape, synchronous focus restoration, outside/Escape dismissal, empty lists,
ASCII typeahead and keyboard paging over forty items with disabled endpoints.
Scroll tests cover all six single/multi List/Select/Combobox paths, fractional
wheel deltas, both clamps, out-of-bounds and nonfinite events, idle retention,
disabled input, skipped disabled rows, focus transfer and subsequent selection.
All six paths also test wheel+Enter, Enter+wheel and stale pointer activation in
one event batch.
Hierarchical descriptors and multiple selection are documented below. Remaining
list gaps include touch dragging, variable-height item delegates, searchable
Select, Unicode normalization/case folding and OS accessibility adapters. Tab currently stays in the open popup rather than dismissing and
advancing to the next background control. These are tracked parity gaps; rendering
and keyboard selection of the implemented subset are real.

## Combobox

`cui_combobox` combines a single-line `cui_edit`, an input with textbox semantics,
and an anchored suggestion list. `cui_base_combobox` supplies the same editing,
filtering and keyboard behavior without drawing. The query editor, choice array,
filtered scratch array, committed selected ID and popup state all belong to the
application. Allocate filtered scratch for at least the full item count; insufficient
capacity returns `CUI_EDIT_CAPACITY` before changing query or selection.

Typing opens suggestions and filters label substrings using ASCII case-insensitive
matching (other bytes match exactly). The input keeps focus while Up/Down and
Home/End move the suggestion cursor; Enter commits that cursor. Disabled options
are skipped. Pointer activation commits a suggestion and restores input focus.
Escape or outside clicks dismiss without committing, and an empty filtered result
renders an explicit message. Single and multi Combobox reuse the Input painter after reducing editor events
once. The query shows a measured caret and selected text, clips long content and
retains horizontal scrolling. Primary pointer presses and drags map to UTF-8 byte
boundaries using the previous painted viewport and scroll offset; keyboard select
all, replacement and undo continue to use the editor. Tests cover both modes in
all four themes, selected colors, Unicode drag ranges, one-time text insertion,
undo and pointer mapping after horizontal scrolling.

Queued text and Enter events retain their queue order. Text followed by Enter
commits from the newly filtered suggestions. Enter followed by text commits the
previous active suggestion, then applies the query and leaves its new suggestions
open. This matches separate frames while allowing platform adapters to submit an
input burst without changing which value is committed.

The query remains independent of the committed ID;
selection does not overwrite the query or destroy its undo history. Callers may
replace/select editor text using the editor API when their product needs that.

The tests exercise actual text events, filtering, focus retention, committed versus
query state, disabled/no-match cases, capacity errors, pointer selection and
Up/Down plus Enter in one event batch. List, Tabs, ToggleGroup and Accordion also
have batched navigation-plus-activation tests using the core's validated per-event
activation reports. Two same-frame Collapsible activations both take effect.

Combobox supports single and multiple selection. Remote query delegates,
query/label synchronization policies, Unicode collation, IME,
and the OS accessibility bridge remain incomplete.
The current suggestion reducer applies edits before filtering for the frame;
interleaving text edits with commits in a single batch remains an ordering gap.
Wheel navigation and keyboard commits are reduced in event order; a pointer
activation against the previous layout is discarded if scrolling moved the
window earlier in that batch.


## Multiple selection

`cui_base_list_multi`, `cui_list_multi`, `cui_select_multi`,
`cui_base_combobox_multi` and `cui_combobox_multi` use a caller-owned boolean per
source item. Multi Select and Combobox keep their popup open while toggling choices.
Each visible option exposes its selected state and a checked visual indicator.
Standalone and popup listbox roots expose `multiselectable`, vertical orientation
and the popup's relationship to its labelled trigger/input.
Escape/outside dismissal preserves the selected set. Selected values render as
pill buttons; activating a pill removes that enabled choice and returns focus to
the trigger/input. Disabled choices cannot be selected or removed by interaction.

Multi Combobox additionally takes a `bool *filtered_selected` scratch array with
the same capacity as its filtered item array. Filtering preserves selected choices
outside the current query, and toggling a filtered option modifies only its source
item. Query text and committed values remain independent application state. There
are no implicit string concatenations or hidden allocation limits on the source
set; visible popup windows are bounded as for single selection.

List options can supply a synchronous `on_activate(item_id, user)` callback. It
runs for every validated activation in input order, including repeated activation
of one item in a frame. Multi reducers use this ordered callback so two queued
activations toggle twice. Do not change the borrowed item arrays or their count
inside the callback. A true return value reports an activation even when two
toggles return the selected set to its prior value.

Tests cover repeated toggles, selection of multiple enabled items, accessible
selected flags, persistent popup state, removing chips, query filtering that hides
selected choices, pointer toggling of filtered suggestions and focus restoration.
The chip ID is its source item ID XOR `0x45f2b82e6c9bf447`; the chip container is
the trigger/input ID XOR `0x97c22bb751e0d56a`. Multi Select's outer container uses
`0x471d50d3bf973ac9`. Reserve those IDs as with other component internals.

## Hierarchical menus

`cui_menu_tree` renders actual menu descriptors instead of limiting entries to the
flat `cui_choice_item` array. Initialize `cui_menu_tree_state` once with a unique
popup ID; the application owns descriptor arrays and check/radio values.

```c
cui_menu_entry commands[] = {
    {.id = 101, .label = cui_string("Open"), .shortcut = cui_string("Ctrl+O")},
    {.id = 102, .kind = CUI_MENU_SEPARATOR},
    {.id = 103, .label = cui_string("Show sidebar"),
     .kind = CUI_MENU_CHECK, .checked = &sidebar_visible},
    {.id = 104, .label = cui_string("Export"), .kind = CUI_MENU_SUBMENU,
     .children = export_commands, .child_count = export_count},
};
if (cui_menu_tree(ui, 100, &theme, cui_string("File"), commands, 4,
        &last_action, &menu_state,
        (cui_menu_tree_options){.popup.viewport = {0, 0, width, height}})) {
    dispatch_command(last_action);
}
```

The reducer runs before painting. Normal actions, checked items, mutually selected
radio items, noninteractive section labels, separators and nested submenus have
real behavior and distinct visual treatment. Check/radio values are modified before
the activated action ID is returned. `keep_open_checks` keeps the menu open for
these actions; ordinary actions close the whole tree and restore the trigger.
The semantic roles distinguish normal, checkbox and radio menu items; checked
state, submenu expansion, controlled child menu IDs and semantic parent links are
reported independently of styling.

Up/Down and Home/End skip labels, separators and disabled entries. Right or
activation opens a submenu; Left returns to its parent. Escape closes the innermost
focused submenu before the root. Keyboard navigation and activation are processed
in event order, so Right followed by Enter in one batch activates the first child
rather than the submenu trigger. Per-level ASCII prefix search follows the list
behavior. Hovering a submenu opens it, and parent entries remain clickable while
children are open. Secondary pointer buttons do not dispatch ordinary menu actions.

Nested menus are separate floating portals with increasing paint layers. They
place beside their parent item and flip/clamp against the supplied viewport. Each
level paints a bounded keyboard window and exposes one active Tab stop. The root
focus scope traps background focus while child scopes preserve parent hit testing.
An empty submenu or one with invalid child storage is disabled rather than opening
an unreachable empty scope.

Tree depth is bounded by `CUI_MENU_DEPTH_MAX` (16 levels). Attempting a deeper
submenu leaves the existing tree open and sets `state.depth_limit`; there is no
unbounded recursion or allocation. Descriptor trees must be acyclic and all IDs
must be globally unique. A child scope ID is its parent entry ID XOR
`0x4b64d23e91a738c1`. Arrays and pointed-to values must remain valid for the call.
The state stores stable IDs and metrics, not borrowed descriptor pointers.

The tests run check/radio mutations, repeated toggles, labels/separator/disabled
skipping, three-level navigation and Escape restoration, batched child activation,
real pointer hover, parent clicking with a child open, typeahead and the 16-level
depth limit. Theme metrics supply menu density; text icons and shortcut hints are
caller-provided and do not install key bindings. Entry `render` callbacks can paint
custom row bodies synchronously while retaining the entry's accessible label and
interaction. They must fit the fixed row height; they do not add independent
menu actions. `checks_on_right` places checked-item glyphs after the label and before shortcut hints.
Popup options choose the preferred anchor side; placement still flips and clamps.
URL dispatch, kinetic gesture scrolling, arbitrary item heights and a native OS
accessibility bridge are still parity gaps. The frozen submenu triangle/grace
behavior is itself an upstream TODO.

## Context menus

`cui_context_menu` attaches the same descriptor tree to an already submitted box
ID, including a noninteractive receiving area. Call it after drawing that area.
A secondary pointer press inside its previous-frame bounds opens the menu at the
pointer without activating primary controls. Primary clicks keep their usual
behavior. The modal scope guard prevents a receiving area behind an unrelated
modal from opening; reanchoring its own open popup remains permitted. Opening
inside the current parent modal is permitted.

The menu root has the supplied accessible name. Check/radio, submenu navigation,
Escape and action dismissal use the same reducers as the button menu. Dismissal
restores the previously focused enabled control, including after reanchoring.
`cui_context_menu_open` also provides explicit application-controlled opening with
an anchor rectangle and a restore-focus ID. The application must draw the context
menu each frame after opening it. Touch long-press and an automatic keyboard
context-menu gesture are not implemented.

Headless tests cover secondary press/release routing, primary activation,
coordinate placement, reanchoring, nested action dispatch and focus restoration,
as well as custom row drawing, right-side checks and preferred popup placement.

## DropdownButton property inheritance and ghost surface

`inherit_action_variant` and `inherit_action_size` mark the corresponding outer
property as unset. Both halves then use `action_variant` and `action_size`.
With these flags false, the outer `variant` and `size` take precedence, including
when explicitly set to the zero-valued defaults. Existing zero-initialized options
retain their original defaults.

A ghost pair surfaces both enabled halves when either half is hovered and retains
that surface while its menu is open. The hovered half receives the stronger hover
color. Escape restores the transparent resting appearance when the pointer has
left. Disabled pairs remain transparent. Headless tests inspect the draw list after
real pointer movement, menu activation and Escape, and verify inheritance and
outer overrides. The two halves use independent outer-corner radii and border edges.

`hide_action` or `hide_menu` omits that half from rendering, semantics and focus
registration. At least one half must remain visible. Hiding the menu half closes
an existing menu and clears submenu depth. The remaining half retains pointer
and keyboard activation. The base function accepts a zero action or trigger ID
for the same omission; both zero is invalid. Headless tests cover menu-only action
dispatch, action-only pointer and Enter activation, absent semantic nodes and
changing to action-only while a menu is open.

DropdownButton `anchor` selects the popup corner: top corners open below the
trigger, bottom corners open above it, and right corners align the right edges.
The default is top right. Popup placement flips vertically and clamps against
the viewport when the preferred corner cannot fit. The menu's `popup.side` is
superseded by this explicit corner. General menu trees can independently request
right-edge alignment through `popup.align_end`. Four-corner pointer-opening
headless tests verify the resulting popup rectangles.

Set `tooltip_id`, `tooltip`, `tooltip_state` and `tooltip_options` to retain an
action-half tooltip alongside the split menu. The tooltip shares the existing
Tooltip delayed hover/focus lifecycle and Escape suppression; its trigger is
set internally to the action ID. Loading, disabled or omitted action halves and
an open split menu suppress it. State remains application-owned. Tests cover
real hover delay, painted tooltip text, Escape, keyboard focus and loading/menu
suppression. This reuses the Tooltip component rather than introducing a second
hover state machine.

## Tab label width

`max_width` caps long built-in tab labels and appends a measured Unicode ellipsis.
Short labels keep their natural width. Label prefixes end at UTF-8 codepoint
boundaries, and the semantic name keeps the full original label. The five tab
variants share this behavior. Custom `render` callbacks own their body layout and
are not truncated, so nested close buttons retain their composition contract.
Drag reordering uses application-owned requests described below. UTF-8
truncation preserves codepoints but does not promise grapheme-cluster boundaries.

## Bounded tab scrolling

Provide an application-owned `cui_tab_scroll_state` and positive `bar_extent`
to clip the tab strip to that width (or height for vertical tabs). Wheel input
inside the previous viewport scrolls along the tab axis: horizontal
wheel deltas for a horizontal strip and vertical deltas for vertical tabs. One wheel unit moves 32 pixels. Off-viewport,
modal-blocked and disabled scrolling is ignored, and the offset clamps to the
current content extent. Prefix and suffix slots remain outside the translated
viewport. Manual scrolling survives selection and label changes; applications
can set `offset` directly to reveal an explicit target. No automatic selection
reveal is imposed. Accessibility exposes the current offset and maximum.

The base reducer consumes wheel input and suppresses pointer activation against
pre-scroll geometry in a scrolling event batch. Standalone base users provide
the state's viewport, receiving ID and maximum themselves. Built-in labels have
immediate measured extents; custom bodies use previous-frame tab widths. Tests
cover both axes, translation, fixed slots, selection preservation, clamps,
disabled and off-viewport input and removing tabs while scrolled. Optional native-painted scrollbars are described below.

## All-tabs overflow menu

Initialize `cui_tab_overflow` with `cui_tab_overflow_init`, unique popup/trigger
IDs and application-owned scratch `cui_menu_entry` storage with capacity at least
the tab count. Set its popup options and pass its address as `overflow` alongside
a bounded scroll viewport. The fixed menu trigger appears when content exceeds
`bar_extent`. It lists every full label, skips disabled tabs and changes the
stable selected ID before tab painting and panel rendering. Standard menu
keyboard navigation, dismissal and focus restoration apply. Fitting all tabs or
insufficient scratch capacity hides the trigger and closes an existing menu;
capacity is checked before any scratch writes. The implementation allocates no
per-frame descriptor storage. Menu item IDs derive from tab IDs XOR
`0x8322717be8bd65e1`, and must remain globally unique.

Tests open the overflow menu through real pointer input, inspect full labels,
select an offscreen enabled tab through batched Down/Enter, assert same-frame
selected semantics and panel, restore trigger focus and close the popup when
the bar expands. Insufficient-capacity storage is also checked.

## Tab drag reorder requests

Pass a persistent `cui_tab_drag_state` as `drag` to enable pointer reordering.
A primary press on an enabled tab starts a candidate; moving four pixels starts
an active drag. The styled strip paints an insertion marker at the hovered tab's
midpoint-selected boundary. Dropping emits `requested`, `moved` and `before` as
stable IDs (`before == 0` means append). Apply the array change after the frame;
CUI never mutates the application's item storage. Selection remains the same ID
and panel routing follows that ID after reordering. `requested` clears on the
next submission. Ordinary clicks below the threshold retain selection behavior.

Escape, removing/disabling the source, disabling the strip, lost capture and
scrolling cancel the candidate. Drops outside the tab bounds or scroll viewport
and no-op insertions emit no request. Nested controls do not start a tab drag.
Tests cover both axes, insertion painting, append requests, application-applied
reorder/panel stability, Escape, disabled tabs and ordinary pointer activation.
Cross-strip drag/drop, autoscroll and keyboard reordering remain unimplemented.

DropdownButton rounds the action's left corners and trigger's right corners,
leaving their joining corners square. The action owns the shared seam border;
the trigger omits its left border, avoiding a doubled seam. Focus border widths
follow the same ownership. Per-corner/per-edge core primitives carry this shape
through renderer adapters. As upstream does, these half-specific corner rules
also apply when a half is omitted. Draw-list tests inspect all corners and border
edges across all five variants, four sizes and four supplied themes.

Built-in tab ellipsis finds a fitting codepoint prefix with binary search rather
than remeasuring every shorter prefix. A 4096-byte label test instruments the
configured font measurer and asserts fewer than 64 total measurement calls while
verifying the capped width and rendered ellipsis. Width search assumes monotonic
prefix advances; advanced grapheme/bidi truncation remains a text-layer gap.

DropdownButton also supports `CUI_DROPDOWN_BUTTON_XSMALL` with the frozen
20-logical-pixel control height. `outline` is independent of primary, secondary or danger style:
it uses a low-opacity variant-colored surface, stronger hover/selected surfaces,
variant-colored text/border and muted disabled styling. The existing OUTLINE
variant remains available for the neutral outline. Tests cover the fourth size
across all theme/variant pairs, plus real hover/action activation and selected/
disabled outline surfaces. All named upstream Button variants are now exposed.

`action_selected` preserves an inner action button's selected appearance and
surfaces both halves, combined with outer `selected`. `action_outline` preserves
an action-only outline; outer `outline` applies to both halves. Visual selection
is presentation only: it does not set checked, selected or toggled accessibility
state on these action buttons. The menu trigger reports its actual expanded
state independently. Tests inspect inherited appearance, independent foregrounds,
outer outline precedence and expanded/unchecked semantics after pointer opening.

Set scroll-state `reveal_id` to request revealing a stable tab ID on the next
submission. The request uses measured content geometry, leaves selection alone,
and clears once applied; an unknown ID clears without moving. Oversized tabs
align their leading edge. Custom-body requests wait until a previous-frame width
is available. `reveal_selected` optionally reveals selection changes after base
input reduction and before painting, so keyboard/overflow activation reveals its
new tab in the same frame. The default keeps manual scrolling independent of
selection, matching the upstream explicit scroll-handle contract. A selection
that stays unchanged never resets manual position. `cui_tab_scroll_reveal` also
provides the portable headless geometry operation directly. Tests cover clamps,
invalid geometry, explicit stable-ID reveal, default preservation, same-frame
keyboard reveal and unchanged-selection/manual-offset preservation.

Set Tabs `scrollbar` with a bounded scroll state to show a six-pixel track over
the viewport's trailing cross-axis edge when content overflows. Thumb size follows
the visible/content ratio with a 16-pixel minimum. Primary pointer drag retains
the grab position inside the thumb; track presses center the thumb at the pointer.
The scrollbar supports axis arrows, Home/End and PageUp/PageDown, participates in
focus order and exposes role, orientation, range, value and controlled viewport.
Wheel input over the track uses the same bounded strip reducer. Cancel/Escape,
disabling or removing overflow ends dragging. Painting and input remain portable
CUI operations; this does not claim a platform-native OS scrollbar.

Tests cover both axes, painted thumbs, primary capture/drag to the end, semantic
ranges, keyboard paging, disabled input, wheel routing over the track, input
cancellation and removal when the tabs fit. The track overlays the last six
pixels of the tab viewport and first uses previous-frame viewport geometry.

Dropdown Button also provides warning, success and info filled variants using
the theme intent and corresponding on-intent tokens. Danger uses `on_danger`.
Orthogonal outline uses each intent color for the label/border and translucent
background; selected intent buttons retain their intent. Tests cover all four
themes, filled/outline labels and actual action/menu activation across eight
variants and four sizes.

Default, Link, Text and Custom Dropdown variants complete the named presentation
choices. Default uses its configurable theme Button palette, Link uses its
normal/hover/active theme Link colors with a
measured underline, and Text uses a transparent background with a slightly faded
resting label. Link/Text stay transparent on hover and selection. Custom accepts
portable `color`, `foreground`, `hover` and `active` colors shared by both halves;
selection and held pointer/Space capture use active. Custom normal uses color
with .2 opacity; hover and active use the corresponding supplied colors, including
when outlined. A custom content callback owns its
label decoration; built-in Link action labels are underlined, while the caret
retains its icon presentation.
Custom shadows and rich gradient backgrounds remain unfinished.

Link refines both halves to the Link semantic role while retaining the split
action and menu activation contract. Custom active color survives held capture
across frames and reverts after release/cancellation; pressed colored outlines
use the selected alpha level. Tests verify roles, pointer cancellation and Space
release activation in all themes. Link decoration reserves each half ID XOR
`0xc8134e39119b750d` and `0xc8134e39119b751d` for its label group/underline.

Hierarchical Menu now routes wheel scrolling to the semantic menu scope under
its pointer, including independently scrollable child popups. Fractional wheel
steps accumulate per level; row windows clamp at both ends and expose pixel
scroll offsets/ranges. If the active row leaves the window, the first enabled
visible row becomes active and retains menu focus. Scrolling a parent closes
its child panels, avoiding anchors to removed rows. Keyboard traversal still
reveals the active row; wheel-then-release batches cannot activate stale rows.
Tests cover child/root routing, disabled/separator skipping, clamp/fractional
movement, Home/End reveal, outside/disabled input and stale-release suppression.
Touch inertia and a visible Menu scrollbar remain gaps. The frozen
source's submenu hover-out grace handling itself has a TODO; no full Menu parity
is claimed.

Opt-in Menu `drag_scroll` lets primary-pointer drags move the scrollable level
where the press began. Movement beyond four pixels starts the gesture; fractional
row travel accumulates, and the release cannot activate a menu row even across
frames. Cancel, disabled/closed menus and disabling the option clear gesture
state. Ordinary clicks below the threshold retain activation. The gesture also
works for touch adapters that produce the same pointer events; CUI currently has
no device discriminator, so this is not a claim of native touch inertia or
multitouch. Tests cover parent/child gestures, held state, cancellation, sub-row
movement, stale release and ordinary clicking.

Re-entering an already open submenu trigger preserves its child scroll window
and active row instead of rebuilding that level. This also retains deeper open
paths; changing to another parent or scrolling a parent still closes/replaces
the affected child. A pointer test verifies re-entry after child wheel scrolling.

Dropdown now consumes `theme.buttons` independently configurable filled and
outlined normal/hover/active backgrounds, foregrounds and borders for each
intent and Default. OUTLINE selects the Default outline palette. Default and
outlined buttons draw borders; filled intents follow the reference borderless
appearance. Link uses its three theme foreground tokens; Text uses .9/1/.7
foreground opacity for normal/hover/active. Ghost pair hover/menu-held idle
surface uses half-opacity accent while the hovered half uses full accent, and
selected uses the secondary active surface. Disabled styles derive opacity from
the normal tokens, and loading preserves normal color with .8 opacity while
blocking action activation. Arbitrarily distinct palette tests prove that hover,
held active, selected, outlined and disabled states use the configured values
without deriving their RGB colors from another state. Custom normal/outline-normal mixing toward transparent uses .2 alpha and
outline border mixing uses .4 alpha, matching the source factor-first convention.
For a fully transparent target, premultiplied Oklab mixing preserves RGB, so
these alpha operations are equivalent. Rich backgrounds and custom shadow
painting remain explicit gaps.

Tab `max_width` now caps custom-rendered bodies as well as built-in labels.
The core natural-width constraint leaves short bodies untouched on the first
frame and clips long bodies without a measurement flash. Custom callbacks still
own text ellipsis/decoration inside their body. Scroll range/reveal calculations
use the capped tab width. Headless tests cover short/long custom content from the
first frame across all variants, retained scroll geometry and actual selection.

Optional per-item `cui_tab_parts` arrays (`parts`/`parts_count`) provide prefix,
suffix and icon callbacks. Declare positive `prefix_width`/`suffix_width` to
reserve their full widths while the built-in middle label absorbs truncation.
Callbacks receive the effective disabled item and remain inside the tab; suffix
controls can handle their own activation without selecting the tab. Slot user
pointers are borrowed. With zero/invalid widths slots fit naturally, but automatic
intrinsic width reservation under a cap remains unfinished. Icon mode replaces
the normal/custom body, reserves a theme-height-based icon area and exempts the
tab from max_width, as in the reference icon-only behavior. Scroll/reveal extents
include declared slots and icon exemptions; intrinsic/custom bodies use retained
geometry. Tests cover every variant, prefix width, suffix placement, ellipsis,
custom body composition, slot clicks, effective disabled propagation, array bounds
and icon exemption. Slot wrappers reserve tab IDs XOR `0xacf7823ab98e17d0`,
`0xacf7823ab98e17e0`, `0xacf7823ab98e17f0`, `0xacf7823ab98e1710`.

List offers `on_confirm(item, secondary, user)` after the existing activation
callback, and `on_cancel(user)` for focused Escape without changing selection.
Both use `activation_user` and preserve input order. `secondary_modifiers` selects
the portable host policy (zero defaults to CTRL|META); modified Enter and primary
click confirmation expose that flag. Optional `track_secondary` paints an
independent context-click highlight without committing/confirming. A later
confirmation, Escape, removed/hidden/disabled row or disabling the option clears
that highlight. Empty lists retain focused cancellation. Tests cover callback
order, modified/unmodified Enter, modified primary release, context highlighting,
disabled behavior and empty cancellation.

Tabs now default to the frozen Medium metrics rather than generic Button control
height. `size` selects Medium/Small/Large/XSmall; `density` defaults to theme
font_size/16 and scales logical pixels, and positive `height` is an explicit
styled height override. Public `cui_tab_size_metrics` gives outer/inner heights,
horizontal padding, font and icon glyph sizes for composing callbacks. At density
one outer heights XS/S/M/L are 20/24/32/36 (Underline 26/30/36/44), and icon
glyphs 10/14/16/16. The icon area uses the variant's inner height times 1.25, with
its cap exemption preserved. Horizontal spacer layout avoids introducing vertical
padding. Inter-tab gaps follow the variant/size table (Medium: Tab0, Outline12,
Pill4, Segmented2, Underline16), while prefix/body/suffix gaps are four logical
pixels. Panels retain supplied
theme styles. Tests cover all twenty variant/size combinations, exact icon area
and glyph bounds, scroll extent, explicit density and height override. This is a
visible default change: a standard 42px Button theme now has 32px Medium Tabs.
Automatic intrinsic slot reservation remains a separate parity gap.

Segmented bars reserve horizontal XS/S/M/L padding of 2/3/4/4 logical pixels,
outside the scroll viewport and inside the bar prefix/suffix. Underline bodies
reserve top margins 1/2/3/5 and bottom margins 2/3/4/6, centered within the outer
height; the selected underline is a bottom border and consumes no body layout.
Headless bounds tests cover each size, both margins and the padded viewport.

Tabs consume the independent tab foreground, active foreground, active surface
and bar tokens. Outline and Pill use their source-specific secondary hover and
foreground policies; disabled selected Pill colors halve opacity. Default tabs
paint side borders, Underline paints only its bottom edge, and selected Segmented
bodies paint an inset background with size-specific radius. A pointer-routed
headless test customizes tokens and checks normal, hover, selected and disabled
foregrounds for every variant. Segmented raised shadows still require a renderer
primitive; the optional animated presentation is described below.

Caller-owned `cui_tab_motion` and `cui_tab_motion_step` provide the retained
left/width springs used by the frozen indicator. Pass `theme.spring_move` and
frame delta; first layout snaps, later targets retain both velocities, and
reduced motion, disabled travel or response zero snaps. Invalid geometry leaves
state untouched. Exact analytic steps handle under-, critically- and overdamped
settings without frame-subdivision dependence. Optional
`cui_tabs_options.animation` wires this state to the shared indicator for
horizontal Pill, Underline and Segmented strips. Supply frame `dt`; state and
its retained bounds must live through `cui_end_frame`. First layout uses static
selected visuals, then an underlay paints below labels with inherited clipping.
Selection switches preserve velocity, scrolling adjusts box-relative position,
and reduced motion snaps. Enabled selected static surfaces are suppressed while
the indicator is ready; disabled selected tabs retain their static styling.
Pill built-in text fades for 200ms with ease-in-out-cubic and linear HSL channels
on later switches; custom body callbacks retain their own text presentation.
Tests route clicks, check initial fallback and paint order, reverse retargets,
scroll displacement, disabled fallback, reduced motion, grayscale and colored
HSL fading. Full-bar background/radius includes prefix and suffix, and the bar
centers its children vertically. Segmented shadows remain a separate gap.

A configured bar suffix or overflow menu now reserves the frozen 12-logical-pixel
`last_empty_space` tail inside the scroll content, including the inter-item gap.
It remains present when an overflow menu is configured but currently hidden.
`last_empty_space_width` overrides its logical width; the accompanying custom
flag permits explicit zero. An optional callback and user pointer compose a
custom body within this fixed reservation. Arbitrary intrinsic tail widths remain
a measurement-contract gap; explicit widths keep first-frame scrolling exact.
Headless tests check tail placement, range and density, wheel-to-end plus button
routing in a custom tail, and event suppression for an explicit zero-width tail.
This changes a 100px tab with suffix in a 110px viewport from no overflow to a
2px scroll range, as the 12px tail now participates in the range.

Per-item parts also expose `omit_leading_border`, matching the frozen first
Default tab's `tab_bar_prefix(false)` policy. It removes only that first tab's
left edge; later tabs and other variants keep their own edge policy. Rendered
edge tests cover default, opted-out, disabled selected and a later selected tab.

Loading preserves the action label and custom body instead of replacing them
with an ellipsis. It keeps the action inert while the menu remains independently
usable. Headless tests verify the custom callback runs during loading, layout
width remains stable, built-in labels remain visible and action clicks are
blocked. Loading now applies inherited opacity of 0.8 to the entire action
subtree, including caller-colored text/shapes, bitmap checkmarks and nested
opacity. It preserves base palette alpha and applies the fade exactly once;
a disabled loading action keeps disabled styling without that extra fade.
Tests cover all twelve variants, custom text/shapes and nested bitmap opacity.
Icon-to-loader substitution still needs a portable icon primitive.

Dropdown now uses the frozen button metrics: XS/S/M/L heights 20/24/32/32,
horizontal padding 4/8/10/12, fonts 12/14/14/16, content gaps 4/4/8/8, and
caret areas 12/14/16/16. Public `cui_dropdown_button_size_metrics` exposes these
logical metrics. `density` defaults to theme font_size/16; positive `height`
overrides styled height while preserving caret/button widths and typography.
Horizontal spacer padding avoids vertical padding, and labels reserve the source
1.25 line height. Link/Text fit label/custom content and caret areas without
padding or fixed height; a styled height override still applies to them.
Tests cover every theme/variant/size combination, exact spacing/font/caret bounds,
density and height overrides, and Link action routing. This changes a standard
42px Button theme's Dropdown Medium height to 32px. Menu rows use their
separately documented source metrics. The caret remains a text fallback pending a proper
portable icon primitive; intrinsic custom-body sizing and built-in capped-label
ellipsis remain separate presentation gaps.

Choice selection marks now use the shared antialiased `cui_checkmark` bitmap,
not ASCII checkbox/radio strings. Popup Menu checked and radio entries paint the
source 12px check glyph. A left glyph/icon column aligns every row whenever any
left glyph/icon exists; explicit icons win over left checks. Right checks appear
only on checked rows and coexist with left icons. Flat multiselect Lists reserve
a trailing 20px slot with a 14px muted check; Select and Combobox reserve a
trailing 12px foreground check slot, including single-selection popups. Metrics
scale with theme font_size/16. Unchecked slots stay blank. Existing roles,
checked/selected states and radio exclusivity remain semantic properties.
Headless tests route check/radio toggles and disabled clicks, inspect left/right
placement, icon precedence and tints, and verify selection/check bounds and
activation in all five List/Select/Combobox modes. Shared glyph pixel rendering
is covered by the core/component/backend owner; Choice tests inspect IMAGE
commands and routing. Popup menus use accent/accent_foreground for enabled
active or hovered rows; disabled rows suppress that highlight. Select and
Combobox rows use accent for the active cursor while retaining the normal text
foreground, and unselected enabled hover uses accent at 70% alpha. Other rows
leave their background transparent. Routed keyboard tests inspect row fills
and check/text tints. Ordinary List rows use list_active (or accent when list_active_highlight is
false) for the active cursor, list_hover for inactive unconfirmed hover, and
selection for the secondary-selected outline. Their foreground remains text;
disabled rows use muted and suppress active/secondary paint. Popup panels use
popover, and ordinary hierarchical menu labels use popover_foreground. These
colors are independently configurable and tested with distinct token values.
Row metric policies are documented below; other remaining gaps stay explicit.

Default and Underline bars paint their full-width one-pixel baseline as an
anchored underlay, after the bar background and before tab children. Its width
and bottom position resolve from the same frame's final bar bounds. Selected
Underline edges and animated indicators therefore paint over the baseline.
Headless tests change prefix width and height on consecutive frames and verify
immediate baseline resizing, bottom anchoring and paint order.

ToggleGroup's existing convenience API retains its exclusive/multiple,
allow_empty and vertical keyboard policy, and paints each item through the
shared Toggle renderer. It registers item semantics only once. Its default
Medium item geometry is now the source Toggle 32px height and 14px text, with
ghost accent styling, rather than the old ordinary Button theme geometry.
The richer ToggleGroup_ex API provides independent size, variant, segmented
styling and controlled requests. Routed tests cover rejected empty selection,
exclusive changes, allowed clearing, same-batch navigation/activation and
whole-group disablement through the convenience API.

Hierarchical Menu options now expose size, density and max_height. Medium,
Large and XSmall use the source 26 px row height; Small uses 20 px. Density defaults
to theme.font_size/16 and scales these logical metrics. Rows use 14 px text,
12 px icon slots, 8 px horizontal padding, 4 px outer padding and 2 px vertical gaps.
The radius caps at 8 px and Small halves it. A separator reserves 6 px (2 px line
plus two 2 px margins); group labels reserve 14*1.618034 px. The natural line-height
factor comes from the pinned gpui-pre 0.3.8 TextStyle default, rather than a
Tailwind line-height assumption. The explicit core text line-height API preserves
fractional font and line-height values through layout and painting.
Default popup height caps at min(viewport.height/2,450 px); positive max_height
sets an explicit cap. Popup fitting, semantic scroll offsets, submenu anchors,
pointer hover routing and wheel tail limits account for nonuniform rows.
Tests cover every size in four themes at densities 1/2, padding/font/bounds,
submenus after separators/group labels, and one-event wheel movement to the
true tail of a mixed-height menu. Source scroll still uses integer row windows;
partial rows and continuous pixel offsets, natural intrinsic popup width,
negative separator horizontal margins, custom arbitrary-height bodies and
popover shadows remain gaps. Existing explicit popup.width remains supported.

Dropdown variant ordinals now match the source default: Default 0, Primary 1,
Secondary 2, Danger 3, Outline 4, Ghost 5, Warning 6, Success 7, Info 8, Link 9, Text 10,
Custom 11. Zero-initialized options therefore use Default rather than Primary.
This is an explicit enum-value migration: named constants continue to select
the same variants, but persisted integers, foreign bindings and ordinal UI
selectors must be remapped. Previous values were Primary 0, Secondary 1,
Outline 2, Ghost 3, Danger 4, Warning 5, Success 6, Info 7, Default 8; Link/Text/Custom
values remain unchanged. A custom Default token test verifies the zero-options
presentation directly.

Ordinary List rows now use the source 16 px font,12 px horizontal/4 px vertical
padding, 4 px body/check gap and natural height 16*1.618034+8 px. List has no source
size variant. Positive options.height overrides the fixed row height; density
scales logical metrics and defaults to theme.font_size/16. Its multiselect check
reserves 20 px and paints a 14 px muted glyph. Inactive rows are transparent; the
secondary-selected selection outline is the only row border.

Select and Combobox rows expose Medium/Small/Large/XSmall/Custom sizing.
Their fonts are 14/14/16/12 px; horizontal padding is 7/5/9/1 px and vertical
padding 4/2/8/4 px. Custom sizing uses font=custom_size*0.875 with 5 px horizontal
and 4 px vertical padding. Natural height is font*1.618034+2*vertical_padding.
All searchable rows reserve a 12 px check slot and 4 px body/check gap. Density
scales these logical values, including fractional fonts and glyph sizes.
Positive row_height overrides row height while preserving the font policy.
Popup padding is 4 px and the default total height cap is 20 rem=320 px, scaled by
density; positive max_height changes that cap. Popup fitting, cursor reveal and
semantic scroll offsets use the resulting row heights. Headless tests cover all
five sizes across Select/Combobox single/multiple modes, fractional custom fonts
and density, padding/gap/check geometry, explicit row-height overrides, routed
selection, default/custom popup caps and End reveal/activation. Existing wheel,
disabled and custom-body routing tests remain in the full suite.

The fixed-height convenience row APIs still do not measure arbitrary custom
content height or variable multiline body heights. Scroll windows remain whole
rows rather than continuous pixel offsets/partially visible rows. Custom body
callbacks choose their own text presentation. Natural popup width, portal shadow
and remaining icon/loader substitution remain separate gaps. Legacy flat Menu
shares the flat popup infrastructure; hierarchical Menu has its dedicated
source row metrics and nonuniform separator/label accounting.

Searchable Select and Combobox option rows span the popup interior, including their pointer targets and highlight backgrounds. This applies to single and multiple selection at every supported row size.
