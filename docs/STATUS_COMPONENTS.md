# Status components

Tag follows the frozen 0.7.1 Tag implementation: `cui_tag_options_default()` is
secondary, medium, filled. Primary/danger/success/warning/info use semantic theme
colors and readable foregrounds. Outline uses a transparent background and intent
foreground. Secondary uses independent secondary/secondary_foreground theme tokens and muted outline text.
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
are supported. The default highlight mixes 20% muted text with 80% of the light/dark theme endpoint in alpha-premultiplied Oklab. If the endpoint has similar HSL lightness, it switches to the opposite endpoint. Applications can override the highlight color. Rich-only content receives
its 0.6–1 opacity pulse in the callback; apply it to that content's paint alpha.
Direct children and icon slots receive opacity 1. Reduced motion paints static text
and provides no animation wake deadline. The base plan returns the next absolute
60Hz host wake while animation runs; the host schedules and invalidates at that
deadline. Passive Marker emits no semantic node by default; an explicit role/name
creates a stable-ID node (for example Status).

Status base now owns only theme-independent defaults/motion validation. Tag style
resolution and palette choices live in components. Marker styled rendering adds
existing `cui_content` spinner dependencies. The base links only math.
Notification manager and rendering are implemented below. Text wrapping and inherited font refinements
are bounded by the existing core text API; custom content can provide its own layout.


## Notification

Initialize `cui_notification_state` with application-owned entry storage (1–128
entries), settings, and optional host hooks. Initialize each descriptor with
`cui_notification_default`, assign a nonzero `{type,key}` ID, then push it with
monotonic host seconds. Distinct keys can share an application type; exact-ID
close and type-wide close are separate operations. Anonymous IDs are generated
by the application. A repeated ID replaces its entry, moves it to newest, and
restarts enter and timeout; replacement does not fire the old close callback.
Capacity rejection and rejected host delivery leave the manager unchanged.

Descriptors, strings, callback code, and callback users are borrowed. They remain
valid while an entry is mounted, and while its system registry record remains
registered. An auto-hidden in-app toast can still have an OS record, so its close
callback does not by itself end these borrowed lifetimes. An explicit dismiss,
clear, replacement, or registry aging ends the corresponding registry borrow.
Do not initialize, push, or advance the same manager from a render callback;
queue structural changes until `cui_end_frame` completes, so tracked retained
bounds still refer to the same entries. Callbacks may request
`cui_notification_dismiss`; action callbacks choose whether to dismiss.
Custom rendering receives a reserved ID, font/color, and opacity; multiply custom
paint alpha by that opacity. No callback or label is retained by core beyond its
ordinary frame semantics.

The pure base manager matches frozen lifecycle rules: 400ms enter, five seconds
present for autohide, then 200ms exit. Each advance crosses at most one phase;
excess time at a phase boundary is discarded. Present timers pause together when
any stack is hovered or contains keyboard focus, including arbitrary custom
children through core focus-scope ancestry. Enter/exit continue while paused.
Window activity is not a pause input. Persistent-only settled stacks have no
wake deadline. Close callbacks fire after exit removal, once. Manual dismissals
remain mounted while ending; visible selection is the newest global
`max_visible` active entries plus every ending entry. It is then grouped into
separate stable stacks for all eight supported anchors.

Render once per frame after application content with `cui_notifications`, a
stable overlay ID, explicit viewport, and host seconds. Schedule the returned
absolute wake deadline and invalidate at that deadline. Per-card placement
can override global settings. Defaults are top right, width 382, ten visible
active entries, margins 16, and top margin 48 (16 plus the reference 32px title
bar); hosts without a title bar can set top margin 16. Auto height is measured
through retained bounds and schedules a follow-up frame when it changes; fixed
height is optional. Entry arrays and state must remain alive through frame end.

Collapsed stacks show three layers, 14px peeks, and 5 percent width reduction
per layer; expanded stacks use actual heights and 14px gaps. A taller rear card
contributes to collapsed stack height. Stack height, item offset, width, and
visibility use bounded critically damped springs with reference 400ms response,
0.1px geometry settling and 0.001 visibility settling. Enter/exit use the
reference cubic Bezier curve and 16px directional slide. Reduced motion uses
settled geometry and static transition paint while retaining lifecycle deadlines.
Body clicks dismiss before calling the click handler. Close buttons appear on
hover or expanded keyboard focus. Middle clicks require matching down/up inside
the card; cancelled captures do not dismiss. Built-in action buttons keep their
own focus and do not automatically dismiss. Titles, messages, custom content,
custom action controls, typed intent icons and custom icon text compose together.
Typed intent overrides custom icon, as in the reference.

To use OS delivery, explicitly configure portable host `send`, `retract`, and
`activate` callbacks and select System or Both delivery. No native OS adapters are
provided by this module. A failed/missing send callback rejects the push. Host
code owns OS availability, stable namespaced tags, activation, and cross-window
ownership. Report OS responses through `cui_notification_system_click`; unknown
or aged-out IDs are ignored. A separate registry retains the newest 100 system
records, including records whose in-app toast auto-hid. Auto-hide does not retract
OS delivery. Explicit exact/type/clear dismissal retracts it; response activates
the host window, retracts, closes any in-app toast, and calls the click handler.

Base status links only math; styled Notification additionally uses core,
existing components buttons, content spinner, and status base. No allocation,
platform, renderer, or default-theme implementation is required by the base.
Renderer limits remain explicit: shadows are two bounded eight-layer rounded
rect approximations of the reference blur/spread/offset, intent icons use portable
text symbols and title font weight uses the current core font. Custom content can supply more
specific icons/typography. Core text wrapping refinements remain limitations described above; this does not establish full native
platform or renderer parity.

Notification cards consume independent `theme.popover` and `theme.radius_lg` tokens for card fill, corners and shadow shape. Marker default highlight now uses the frozen perceptual blend and lightness fallback; exact custom highlight remains supported.


Standalone Shimmer uses `cui_shimmer_text` with caller-owned retained bounds and
`cui_shimmer_options_default`. It paints the unchanged base glyphs plus twelve
clipped highlight layers, using the same alpha-aware perceptual highlight as
Marker. Custom highlight alpha multiplies layer opacity. Duration, relative or
absolute spread, reverse, once and reduced motion follow the frozen source;
`cui_base_shimmer` returns the next absolute host deadline, or zero when settled.
The host supplies elapsed animation seconds and schedules redraws. Explicit IDs
allow identical sibling labels to animate independently. Font size/color may be
set per instance; otherwise theme text styling is used. Optional maximum width
clips the single line without changing glyph positions. Default Label semantics
can be disabled with role None. State must live through `cui_end_frame`.

Standalone Shimmer remains partial: inherited styled-text runs, wrapping,
truncation/ellipsis and font weight depend on the future rich text pipeline.
Reduced-motion text remains visible and settles without continuous redraw.
