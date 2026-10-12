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
The base style resolver allocates nothing and has no theme implementation link
dependency. Styled tags require core and the status base implementation.

Marker and Notification are not implemented in this checkpoint.
