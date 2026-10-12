# Themes

A `cui_theme` C struct carries surface/background, text/muted, primary/on-primary, border/focus/destructive colors and typography/spacing/radius/control height. `cui_theme_light`, `dark`, `high_contrast`, and `terminal` return runtime values. Pass the theme to every styled call; copying and adjusting a token is an explicit per-component override.

The gallery offers light/dark and high contrast switches. The terminal system proves different presentation over the same base behavior. High contrast uses distinct foreground/border/focus values and larger type. System preference detection, a theme-file loader, comprehensive typography/shadow/motion/semantic token families and global density provider are pending. Reduced-motion support will require an actual animation scheduler; no animation is currently claimed.

Intent tokens `warning`, `success`, `info`, and `danger` can be used as filled
surfaces or as text/borders on the theme surface. `on_warning`, `on_success`,
`on_info`, and `on_danger` supply their filled-surface text colors. All four built-in
themes test at least 4.5:1 contrast for each pair and each intent color against its
surface. Applications extending a custom theme must set these new fields; the
fields append to the prior public structure rather than reordering existing ones.
