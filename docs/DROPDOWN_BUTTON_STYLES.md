# DropdownButton paint refinements

Include `cui_dropdown_button_styles.h` with matching rebuilt library sources.
The independent header leaves the legacy options/state layout unchanged. NULL
or an all-zero descriptor calls the existing paint path. This slice adds paint,
not metadata forwarding, arbitrary layout, or a binding facade.

```c
#include "cui/cui_dropdown_button_styles.h"
static void action_paint(void *user, const cui_button_style_info *info,
                         const cui_button_style_paint *current,
                         cui_button_paint_patch *patch) {
    (void)user;
    (void)info;
    (void)current;
    patch->fields = CUI_BUTTON_PAINT_FOREGROUND;
    patch->paint.foreground = (cui_color){250, 240, 210, 255};
}
void draw_commands(cui_context *ui, const cui_theme *theme,
                   cui_menu_tree_state *menu, cui_id *command) {
    static const cui_menu_entry entries[] = {
        {.id = 104, .label = {"Export", 6}, .kind = CUI_MENU_ACTION}};
    const cui_dropdown_button_styles paint = {
        .paint_policy = CUI_DROPDOWN_BUTTON_PAINT_BUTTON,
        .action = {.instance = action_paint}};
    (void)cui_dropdown_button_styled_ex(
        ui, 100, 101, 102, theme, cui_string("Save"), cui_string("More commands"),
        entries, 1, command, menu,
        (cui_dropdown_button_options){.width = 120,
                                     .variant = CUI_DROPDOWN_BUTTON_PRIMARY},
        &paint);
}
```

Initialize the caller-owned menu once with `cui_menu_tree_init(menu, 103)`.
Names, menu entries, callbacks and userdata follow the existing submission and
menu ownership rules. Style descriptors are copied before reduction; callback
arguments are borrowed for the synchronous call. Callbacks must be pure: no CUI
calls, input consumption, owner/options/style mutation or reentry. Only the
output patch changes the submitted paint. The legacy body still runs once,
after the same Base/event reduction; this API does not invoke Button reduction.

Each visible half gets an independent Button-style instance callback, then its
eligible pointer/selected/disabled callback. The instance patch is captured once
and replayed after selected and disabled, including an explicit transparent
color or zero radius/opacity. Loading applies its .8 opacity after replay;
focus is last. Hidden halves have no callbacks. Invalid callback patches are
discarded atomically; accepted input remains accepted. Geometry, clip, and border
width are inspectable but cannot be patched. To replace joined corners use the
CORNERS mask, since RADIUS alone retains the existing custom corner record.

The BUTTON policy uses existing CUI palette tokens with frozen Component state
ordering. Selected/open/loading/disabled suppress pointer refinements; only the
trigger treats an open menu as selected. Ghost action held-group backgrounds
use half hover alpha (dark: quarter accent alpha). Dark ghost hover uses half
accent alpha. Outlined state wins disabled paint, including transparent special
variants. Disabled filled intents use their normal token for both background and
border; Secondary retains alpha (source opacity clamps its 1.5 multiplier to 1).
Custom transparent mixes preserve RGB and scale alpha, with CUI's existing byte
truncation. Source theme token values and sub-byte color precision differ.

LEGACY policy retains old palette/state rules with optional refinements. A
nonzero descriptor with no callbacks may still request container paint. Container
patches accept the Button mask vocabulary except FOREGROUND: there is no implicit
text cascade. Invalid policy/container patches reject before state or input
reduction. Unknown masks and nonfinite/negative radius/corners or opacity outside
0..1 are invalid. An unmasked member is ignored.

Custom shadows require BUTTON, CUSTOM, `custom_shadow=true`, and
`host_shadows_enabled=true`. Both gates default false; they do not imply a global
theme setting. Enabled halves receive `CUI_SHADOW_XS` with resolved corners;
disabled suppresses it, loading retains it. The host renderer must support CUI
shadow commands. Focus paint uses focus-visible and `theme.hide_focus_ring` in
BUTTON; LEGACY preserves the old focus rule. No full DropdownButton parity is
claimed: metadata/name/tab/toggle forwarding remains a separate slice.

Canonical builds register the styles unit test, optional compiled guide and C++
declaration check. The portable WASI runner exposes `dropdown_button_styles`
and `dropdown_button_styles_{all,legacy,styles}_distribution` suites.
Standalone users may define `CUI_DROPDOWN_BUTTON_STYLES_IMPLEMENTATION` before
including `dist/cui_dropdown_button_styles.h`; it delegates to Choice once.
The legacy `CUI_CHOICE_IMPLEMENTATION` entry also defines these additive
functions, and `CUI_IMPLEMENTATION_ALL` includes their implementation. Include
the explicit styles header for declarations. Link the remaining required CUI
modules as usual; these entry points do not create a second Button reducer.
Source SDKs include the header and implementation. Python, JS/TS, Go and Rust
builders do not expose this style facade or its borrowed callbacks.
