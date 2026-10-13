# Separator

`cui_separator_ex` composes a decorative horizontal or vertical one-pixel line. Use `cui_separator_defaults()` for a 160px horizontal solid line, then set the options for a dashed line, centered label or custom color.

```c
#include <cui/cui_components.h>

void draw_login_divider(cui_context *ui, const cui_theme *theme) {
    cui_separator_options options = cui_separator_defaults();
    options.length = 240;
    options.dashed = true;
    options.has_label = true;
    options.label = cui_string("Or continue with");
    cui_separator_ex(ui, cui_id_from(ui, "login-divider"), theme, options);
}
```

Dashes are 4px long with 2px gaps, starting at the beginning of the line and clipping the final dash to the explicit length. Set `vertical = true` to change the axis. The label sits at the center with the theme background behind it, muted text at 75% of the root font size, a root-font-sized line box, and horizontal/vertical padding of 8px/4px. `has_label` distinguishes no label from an explicitly empty label, which still paints the padded background. Labels and their background can extend past a short line.

The default line uses the theme border. Set both `custom_color = true` and `color` to override it; explicit alpha zero remains transparent. Keep label bytes valid through renderer completion. There is no retained state, callback or keyboard stop: submit the separator in each frame where it should appear.

Length zero selects 160px. Positive lengths up to 4096px are accepted. Negative/nonfinite/oversized lengths, missing label bytes, an invalid root font size or ID zero return false and submit nothing. Core retains its normal capacity-error behavior during valid submission. The length bound limits dashed geometry to at most 683 rectangles.

The implementation follows the axis, color, 4/2 dash and padded-label contracts in the [frozen Separator source](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/separator.rs). CUI takes an explicit resolved length rather than automatically inheriting the parent's full width/height. Arbitrary GPUI style refinements and renderer-native dashed vector paths are not represented by this adapter; Core boxes provide explicit composition. The existing `cui_separator` preserves its earlier horizontal, unlabelled API and metrics.

Headless tests check both axes and line styles, dash positions, themed centered labels including empty labels, and rejection without frame output. Separator is a reusable primitive; it does not add a 79th entry to the pinned component manifest.
