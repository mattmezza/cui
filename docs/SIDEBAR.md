# Sidebar

The module follows the frozen GPUI Kit 0.7.1 `component/src/sidebar` family:
Sidebar, SidebarHeader, SidebarFooter, SidebarGroup, SidebarMenu,
SidebarMenuItem and SidebarToggleButton. There is no separate frozen base
Sidebar; CUI separates its pure cache/motion model and event behavior from
styled rendering.

`cui_sidebar_state_init` attaches a caller-owned cache with capacity 1..1024.
The cache and state must remain valid through `cui_end_frame`; they allocate no
memory. Entries, menu descriptors and callbacks are borrowed for their render
call. Each entry is a top-level virtual-list item with a unique wrapper ID,
positive estimated height, revision, content callback and user pointer. Wrapper
IDs must differ from the IDs of controls/components submitted by their callback.
Cached fields are inspection-only. Change an entry revision when content outside
the rendered range changes its height. Stable IDs retain measurements through
reordering; changing list count resets scroll position, matching source list
reset behavior.

The virtual list renders the visible range plus 30% of host-window height on
either side. Supply `viewport_height` for a sidebar inside a shorter pane;
otherwise the sidebar height is the fallback. Unknown entries use declared
estimates. Rendered entries receive actual measured heights after layout and
schedule a follow-up frame only when retained bounds change. Tall custom entries
can render their own inner virtual list. Header and footer stay outside the
scrolling body; the body handles wheel, draggable scrollbar and scrollbar
keyboard input through the existing Scroll primitives. First layout and changed
header/footer/available extent use retained geometry for scroll metrics, then
correct it in the measurement frame. The scrollbar's right edge follows parent
layout immediately, including percentage widths. The application drives frames
until `cui_needs_redraw` is false.

Defaults are width 255, left side, expanded, Icon collapse mode and a 200ms cubic
in-out transition. Icon collapse renders children at 48px immediately and animates
only the surrounding clip width. Offcanvas renders at the expanded target width,
clips/slides to 0, and removes children after the transition; their focus and
activation remain available while closing, as in source. None ignores collapsed
state. Left Offcanvas aligns the child to the end of its shrinking wrapper;
right Offcanvas aligns it to the start. Icon mode reverses those alignments.
Reopening during close cancels unmounting. Retargeting samples the current width
to avoid discontinuities; the frozen source instead starts at the previous
target. Reduced motion or duration 0 applies the target and unmount immediately.
`now_seconds` is caller-owned; backward timestamps hold the last sample. There
are no detached timers, and settled animation does not invalidate continuously.

A negative width grows within its parent; `width_percent=true` uses a fraction
in 0..1. These nonpixel widths skip width animation, matching source. Zero plain
width means the 255px default. Height 0 fills the parent. `custom_box` supplies
surface appearance; layout, collapse dimensions and padding remain controlled
by Sidebar. Source clears root padding, so CUI does too. The default background,
foreground, border and accent use the five Sidebar Theme tokens. The left side
has a right border; the right side has a left border.

Header/footer callbacks receive effective Icon-collapse state. The sidebar's
outer header inset is top 12/horizontal 12 expanded,8 collapsed. Footer uses
bottom 12/horizontal 12 expanded and top 8/horizontal 8/bottom 12 collapsed. Icon
mode adds 8px between header/body/footer. `cui_sidebar_header` and
`cui_sidebar_footer` provide the separate source 8px padded rounded hover/selected
surface. Their content owns layout and decisions about hiding its labels. An
optional `on_activate` is a pointer-only Group action for host dropdown/menu
composition; it introduces no Tab stop. Descendant controls keep activation
priority. Header/footer content gets a temporary Theme copy with `text` set to
the inherited Sidebar foreground/accent foreground. Do not retain that pointer.

`cui_sidebar_group` exposes a labelled Group and a 32px label row with font 12,
horizontal 8 inset and 70% Sidebar foreground. Icon collapse omits that row.
Group callbacks can compose `cui_sidebar_menu`; the menu uses 8px item gaps.
Menu items expose TreeItem semantics with accessible-name override, controlled
active selection, expanded/level/controls metadata, source 28px expanded rows,
8px padding/gap and rounded corners. Icon collapse centers the icon, omits label,
suffix, caret and descendants, and preserves submenu state. Nested descriptors
can retain their own collapsed flag; the top-level menu overrides it with the
menu's collapsed argument, as in source.

A submenu requires persistent `cui_sidebar_item_state`. `default_open` initializes
it once when the item first has children. Removing and re-adding children retains
that state. An optional `expanded` pointer supplies a controlled value; requests
go to `on_expand` and the owner updates the pointed value. `click_to_open` takes
precedence over `click_to_toggle`; neither is enabled by default. Row activation
always invokes `on_activate`, while a caret toggles expansion without invoking
the row callback. Validated queued activations are reduced in input order rather
than collapsed to a single action. The source disabled flag suppresses only row
activation and hover: the separate caret and suffix controls stay independently
enabled. `disable_caret` is an explicit additional option. The submenu rule sits
at 14px and children start at 25px, including its 1px border and 10px inset; children
have 4px gaps and 2px vertical padding.

Named icons use the actual context-owned Icon resource helper; no text glyphs
or rectangle placeholders represent them. The four panel toggle icons and
ChevronRight are in the default embedded bundle. The caret is 16px and rotates
90 degrees when open. Menu named icons default 16px; `icon_content` supports an
application-owned custom Icon/path/state/style. It receives the row foreground
through the temporary Theme's `text`. `label_content` and `custom_label_box`
bridge source label styling, with accessible text still supplied by the label
or accessibility override. Suffix content gets the same inherited foreground.
Callbacks own any further controls, loaders or persistent state they compose.
The current text API does not expose font weight, so source active-item
`font_medium` emphasis is represented by its accent colors, without fake bold.

Collapsed icon items use the real delayed Tooltip primitive. Supply
`tooltip_options.viewport` with the host viewport; the default side is Right,
including a right-side Sidebar, as in source. Default open delay is 500ms. An item
state is needed for this lifecycle; iconless items have no collapsed tooltip.
`context` runs after the item and can compose the existing ContextMenu helper
with application-owned menu state/actions. Header/footer host actions can open
those menus or other popup components; the module does not invent a second
menu manager. Popup helpers retain their own viewport/activation contracts.

The toggle is a 24px Small ghost button with a 16px panel-left/right-open/close
icon, controlled collapsed state and `on_change`. Supply an accessibility label:
the source toggle has no implicit accessible name. Its owner updates the
Sidebar's collapsed option. Ordinary Tab and Enter/Space activate item/caret
controls in CUI; the source row only supplies pointer click behavior, so row
keyboard activation is an explicit accessibility improvement. No invented
roving arrow-key tree policy replaces the source's composition. Sidebar widths
and row sizes follow source; it does not claim four selectable sizes where the
frozen API has no Sizable interface.

The browser gallery forwards pixel wheel deltas as precise CUI scroll events for Scrollable, Sidebar and VirtualList. Fractional deltas retain their distance rather than being multiplied by the widget's discrete wheel step. Native hosts can opt into the same contract with `cui_event.scroll_precise = true`; discrete wheel events continue to use the configured step. Sidebar's absolute content position follows its scroll model, while the scrollbar stays anchored in the viewport.
