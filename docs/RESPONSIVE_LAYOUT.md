# Responsive containers

Use `cui/cui_responsive.h` to make layout decisions from a container's current
logical width. Named mobile-first breakpoints resemble familiar utility-class
systems: BASE, SM (640), MD (768), LG (1024), XL (1280), and 2XL (1536).
This header-only utility needs no backend, allocation, or additional library.

```c
#include "cui/cui_responsive.h"

/* Product-specific tokens. Keep them ordered; units are logical pixels. */
const cui_breakpoints app_breakpoints = {
    .sm = 560, .md = 720, .lg = 1000, .xl = 1280, .xxl = 1536
};
cui_responsive container = cui_responsive_query(available_width, &app_breakpoints);
bool persistent_nav = cui_responsive_at_least(container, CUI_BREAKPOINT_LG);
/* Selector order: query, breakpoint, base value, expanded value. */
float padding = cui_responsive_value(container, CUI_BREAKPOINT_MD, 12, 24);

cui_begin_box(ui, 100, (cui_box){.width = -1, .row = persistent_nav, .gap = 12});
if (persistent_nav || app.drawer_open)
    submit_navigation(ui, &app);
submit_main(ui, &app, padding);
cui_end_box(ui);
```

Pass NULL for standard thresholds. `at_least` includes the threshold itself.
Thresholds must be finite, nonnegative, and nondecreasing; equal thresholds skip
intermediate classes. Widths must be finite and nonnegative. Invalid input
returns `valid=false`, width zero, and BASE; **every** `at_least` predicate then
returns false, including BASE. Check `valid` before submitting a container.
`value` chooses its base argument for invalid queries. It does not validate the
two caller-provided values or change their units.

For content constraints, `cui_responsive_fits(query, first_min, second_min, gap)`
answers whether two panes can sit side by side. Query the main area's width after
subtracting sidebar and spacing, rather than reusing the window breakpoint:

```c
cui_responsive main = cui_responsive_query(main_inner_width, NULL);
bool row = cui_responsive_fits(main, 260, 260, 12);
cui_begin_box(ui, 200, (cui_box){.width = -1, .row = row, .gap = 12});
submit_summary(ui);
submit_activity(ui);
cui_end_box(ui);
```

`fits` rejects negative/nonfinite minima and gaps. Its comparisons avoid overflow
when adding large finite dimensions. Breakpoints select policy; normal CUI GROW
(`-1`), explicit dimensions, padding, rows and columns perform layout. Recompute
queries each frame, including when only a nested container changes width. Width
queries do not read a window, multiply by display density, or schedule work.

The `examples/responsive_dashboard` fixture keeps one `dashboard_app` state while
resizing: navigation stays beside content at LG, becomes a Menu disclosure below
LG, and the cards stack when their minimum widths no longer fit. The disclosure
occupies layout space above the main content, so it cannot overlap active content
beneath it. Hidden navigation is omitted from submission, including its input and
semantics. Closing it keeps page/account state. Applications that want an overlay
drawer should additionally use the existing modal/overlay scope APIs.

This helper supplies width policies, not CSS media-query or browser-layout parity.
The example and tests cover live container resizing, boundary crossings,
navigation/account callbacks, state preservation, and clicks at removed targets.
