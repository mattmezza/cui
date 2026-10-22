# Questionnaire geometry and numeric metrics

Include `cui_questionnaire_geometry.h` with matching CUI sources/library. This
additive API does not resize legacy options, choice views or state. NULL and
all-zero geometry preserve the original layout tree, text paint and reduction.
No layout/paint option changes model events or disabled eligibility.

Pass an independent descriptor to each `_ex` part. Root publishes its existing
size/density only: numeric metrics and refinements never implicitly inherit.
Existing keyboard policies can be passed independently; NULL retains the existing
route inheritance. Existing progress, named presentation and observers are
unchanged. The complete checked resolver lets the owner reuse numeric values
explicitly, including with custom children.

```c
#include "cui/cui_questionnaire_geometry.h"
void questionnaire_geometry_example(cui_context *ui, const cui_theme *theme,
                                    cui_questionnaire_state *state) {
    cui_questionnaire_options options = {.design_size = CUI_DESIGN_BASE};
    cui_questionnaire_geometry_options geometry = {
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_MINIMUM_HEIGHT,
        .metrics = {.minimum_height = 64},
        .card = {.fields = CUI_QUESTIONNAIRE_BOX_PADDING,
                 .padding = {.left = 12, .right = 16, .top = 8, .bottom = 8}},
        .answer = {.fields = CUI_QUESTIONNAIRE_TEXT_SIZE |
                            CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT,
                   .font_size = 18, .line_height = 28}};
    if (!cui_questionnaire_begin_ex(ui, 100, theme, state, options, NULL, &geometry))
        return;
    size_t item = state->current;
    if (cui_questionnaire_item_begin_ex(ui, 101, state, item, options, &geometry)) {
        if (cui_questionnaire_choices_begin_ex(ui, 102, state, item, options, &geometry)) {
            for (size_t choice = 0; choice < state->items[item].choice_count; ++choice)
                (void)cui_questionnaire_choice_control_ex(
                    ui, 200 + choice, theme, state, item, choice, options, NULL, &geometry);
            cui_questionnaire_part_end(ui);
        }
        cui_questionnaire_part_end(ui);
    }
    cui_questionnaire_end(ui);
}
```

Use application-owned collision-free IDs and schema lifetime as in the original
Questionnaire API. The example assumes a valid initialized model and submissions
inside a frame. No event facade, binding wrapper or automatic arbitrary text
cascade is supplied.

## Resolution and validation

Resolve existing legacy size/shared XS–9XL and density first. With shared size
INHERIT, nonzero `custom_scalar` supplies a complete source numeric profile;
non-INHERIT shared size wins. Scalar is finite (0,256]. Individual masked metrics
then override that profile, including zero gaps/padding/minimum. No additional
four-size enum or numeric inheritance cache is introduced. Lengths must be finite
[0,4096]; font sizes finite (0,512]. Width is grow(-1), FIT(0), positive fixed;
`fixed_zero_width` makes a masked zero width fixed zero. Scalar is independently
validated even when shared size wins. Unmasked values are ignored.

| Numeric custom V | Formula |
| --- | --- |
| root/item/choices/answer/content gaps | V / V / .5V / .625V / .125V |
| card horizontal/vertical padding; minimum height | .75V / .625V; 2.75V |
| indicator / radio mark / checkbox check | V / .5V / .875V |
| shortcut box / text | 1.25V / .625V |
| answer text / line box | V / 1.5V |
| secondary / title / progress text | .875V / 1.125V / .75V |
| card / shortcut radius | theme large / theme regular |

Custom secondary/title/progress line boxes are natural(0), reflecting source
branches that set size without a guaranteed explicit line box. Existing named
presentation/progress APIs remain independently configured; this helper does not
silently restyle them. Legacy helper values extract existing CUI paint, including
legacy heading line+4 and natural shortcut/progress lines. Shared scales retain
existing CUI geometry; their higher steps are CUI extensions. Fractions undergo
Core layout quantization, not arbitrary fractional-pixel parity.

Answer/secondary/shortcut refinements use the existing text masks. Resolved text
views include color/style. They affect the default paint branch of the existing
choice reducer. Effective answer first-line height aligns indicators/shortcuts;
natural0 uses the configured font metrics. Invalid/missing metrics reject before
interaction when a natural active answer requires them. No text/style cascade
into custom children or editors is implied.

The five box patches apply after built-in geometry/state paint. Card gap patches
only the actual answer row; outer gap remains0. Explicit disabled opacity paint
can be overridden without reenabling interaction. Ancestor opacity multiplies.
Existing indicator/shortcut styles still patch their default elements; content
style patches its wrapper, including around custom content. New card patches do
not leak into those children.

Unknown masks, nonfinite active values, invalid text styles and conflicting
fixed card height reject before bindings, input reduction, callbacks or model
mutation. Fixed height must be at least the resolved minimum and vertical
padding; no maximum-height field is offered. The checked resolver leaves output
untouched on failure. Entry bool meanings remain legacy: choice returns changed,
container returns accepted/opened, so false on choice is not an error report.

## Minimum bounds and callbacks

Opt-in cards use physical padding around a gap0 row, with a zero-width minimum
spacer and the real answer row. The same outer ID owns paint, pointer/key/AT,
focus ring and semantics; minimum blank area is interactive. FIT cards grow with
long content. Fixed heights may overflow; ancestor clipping clips paint and
pointer hits while semantic layout bounds can extend beyond the visible region.
Very narrow widths/adornments retain ordinary layout overflow. No silent resize.

New indicator/content/shortcut renderers take an immutable extended snapshot:
old choice view, fully resolved numeric metrics and three text views. New wins
over the corresponding old renderer. Old receives its unchanged legacy view;
custom numeric values are never misrepresented as a legacy size. All descriptors
are copied before children run. Callbacks run once in existing order and derived
namespaces; shortcut OFF suppresses both new and old shortcut children. Snapshot,
function and userdata are borrowed for synchronous submission only. Children
balance boxes, avoid model mutation/input dispatch, and do not retain views.
Drawn strings survive completed-frame consumption. Caller avoids namespace
collisions, as for existing composition. No retained attachment or dispatch layer.

Frozen source reference: `87d10ae5`,
`crates/component/src/questionnaire/components.rs:85-253,297-364,527-678,772-975`.
This covers bounded container/card/numeric/default-choice text geometry. Input
left-only padding, independent editor paint/shaping, Button/trailing actions,
Form role, arbitrary Styled inheritance and binding facades remain separate
required followups. All78 entries remain; this is not full Questionnaire parity.
