# Themes

A `cui_theme` C struct carries surface/background, text/muted, primary/on-primary, border/focus/destructive colors and typography/spacing/radius/control height. `cui_theme_light`, `dark`, `high_contrast`, and `terminal` return runtime values. Pass the theme to every styled call; copying and adjusting a token is an explicit per-component override.

The gallery offers light/dark and high contrast switches. The terminal system proves different presentation over the same base behavior. High contrast uses distinct foreground/border/focus values and larger type. System preference detection, a theme-file loader, comprehensive typography/shadow/motion/semantic token families and global density provider are pending. Reduced-motion support will require an actual animation scheduler; no animation is currently claimed.

Intent tokens `warning`, `success`, `info`, and `danger` can be used as filled
surfaces or as text/borders on the theme surface. `on_warning`, `on_success`,
`on_info`, and `on_danger` supply their filled-surface text colors. All four built-in
themes test at least 4.5:1 contrast for each pair and each intent color against its
surface. Applications extending a custom theme must set these new fields; the
fields append to the prior public structure rather than reordering existing ones.

Each theme has `buttons[CUI_THEME_BUTTON_*]` palettes with independent filled
normal/hover/active colors, foreground, outlined normal/hover/active colors,
outlined foreground and borders. Default, primary, secondary and the four intent
palettes can be overridden independently. Link normal/hover/active foregrounds,
input background/border, secondary foreground and accent colors are separate.
Built-in constructors derive these initial values from CUI's own palette; custom
themes must initialize the extended fields. These are solid-color tokens; gradient
backgrounds and shadows remain separate rendering capabilities.

`spring_control` provides response seconds, damping ratio, epsilon and travel
permission (defaults: 0.18 seconds, 1, 0.001, true) for component motion. A theme's
motion settings are configuration; components still own their retained motion
state and the host supplies time/redraw scheduling.
