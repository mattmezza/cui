# Themes

A `cui_theme` C struct carries surface/background, text/muted, primary/on-primary, border/focus/destructive colors and typography/spacing/radius/control height. `cui_theme_light`, `dark`, `high_contrast`, and `terminal` return runtime values. Pass the theme to every styled call; copying and adjusting a token is an explicit per-component override.

The gallery offers light/dark and high contrast switches. The terminal system proves different presentation over the same base behavior. High contrast uses distinct foreground/border/focus values and larger type. System preference detection, a theme-file loader, comprehensive typography/shadow/motion/semantic token families and global density provider are pending. Reduced-motion support will require an actual animation scheduler; no animation is currently claimed.
