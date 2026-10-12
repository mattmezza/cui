# Status components

Tag follows the frozen 0.7.1 Tag implementation: `cui_tag_options_default()` is
secondary, medium, filled. Primary/danger/success/warning/info use semantic theme
colors and readable foregrounds. Outline uses a transparent background and intent
foreground. Secondary uses the theme surface/text pair and muted outline text.
Custom colors preserve all three application colors. Palette tags accept the exact
50/200/300/600/800/950 scale colors and explicit dark mode, so applications can
supply their named palette without silently substituting a different palette.
`cui_tag_named_palette` supplies all 21 named frozen palettes, including white
and black, from the pinned default-colors.json six required scale colors.

Small and extra-small use 10px text, 6px horizontal/2px vertical padding, and half
theme radius; medium uses 12px text and 10px/4px padding; large uses 14px text.
Typography scales with the theme font size (14px corresponds to a 16px root).
Explicit radius zero is supported; pill rounding takes priority. Hover reduces
paint alpha to 90 percent. Tags have no built-in click or keyboard action.

`cui_tag` submits a passive label semantic and text. For arbitrary rich children,
call `cui_begin_tag`, submit children using its returned style, then call
`cui_end_tag` with the same ID/style. The style is a plain value owned by the caller;
no callbacks or strings are retained. Begin failure must not be paired with end.
The component style resolver allocates nothing and has no theme implementation
function link dependency. Styled tags require core and status base defaults.

Tag and Marker are implemented in this checkpoint.

Marker accepts up to 32 ordered children: typed icon, typed text, rich content,
and direct custom children. Callback IDs are reserved child IDs, not the enclosing
slot ID. Callback/user/string lifetimes last through the render call. The retained
`cui_marker_state` must remain alive through `cui_end_frame`, because tracked text
bounds update after layout. Initialize it to zero. Explicit widths for custom
children let separator/alignment geometry reserve their space; callback layouts
remain application owned. Default width is 480; set it to the host container width.

Plain and Border start-align by default; Separator centers. Explicit start/end
alignment leaves only the divider on the opposite side. Border adds an 8px bottom
inset and a single bottom edge. Typed icon slots suppress automatically inserted
spinner loading; direct children do not. Typed text shimmer uses twelve nested
clipped glyph overlays, matching the frozen source's twelve-layer feathered band.
Relative spread defaults to 0.3 and clamps to 0.05–1; absolute spread clamps to at
least 1px. Duration defaults to two seconds, with a minimum 1ms. Reverse and once
are supported. The default highlight uses the theme's foreground/background endpoint;
applications can supply exact custom highlight colors. Rich-only content receives
its 0.6–1 opacity pulse in the callback; apply it to that content's paint alpha.
Direct children and icon slots receive opacity 1. Reduced motion paints static text
and provides no animation wake deadline. The base plan returns the next absolute
60Hz host wake while animation runs; the host schedules and invalidates at that
deadline. Passive Marker emits no semantic node by default; an explicit role/name
creates a stable-ID node (for example Status).

Status base now owns only theme-independent defaults/motion validation. Tag style
resolution and palette choices live in components. Marker styled rendering adds
existing `cui_content` spinner dependencies. The base links only math.
Notification is not implemented yet. Text wrapping and inherited font refinements
are bounded by the existing core text API; custom content can provide its own layout.
