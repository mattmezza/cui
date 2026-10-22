#include "../../../src/base/cui_design_metrics.h"
#include "cui/cui_components.h"
#include "cui/cui_questionnaire_keyboard.h"
#include <limits.h>
#include <math.h>
/* Independent f21f9a3d pre-change paint/reduction oracle. Route publication
 * omitted: these tests submit a standalone card without a root. */
static cui_box oracle_refine(cui_box b, const cui_questionnaire_slot_style *style) {
    if (!style)
        return b;
    cui_box v = style->box;
    uint32_t f = style->fields;
    if (f & CUI_QUESTIONNAIRE_SLOT_WIDTH) {
        b.width = v.width;
        b.fixed_width = v.width == 0;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_HEIGHT) {
        b.height = v.height;
        b.fixed_height = v.height == 0;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_PADDING)
        b.padding = v.padding;
    if (f & CUI_QUESTIONNAIRE_SLOT_GAP)
        b.gap = v.gap;
    if (f & CUI_QUESTIONNAIRE_SLOT_RADIUS)
        b.radius = v.radius;
    if (f & CUI_QUESTIONNAIRE_SLOT_BACKGROUND)
        b.background = v.background;
    if (f & CUI_QUESTIONNAIRE_SLOT_BORDER) {
        b.border = v.border;
        b.border_width = v.border_width;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_ALIGN) {
        b.align_x = v.align_x;
        b.align_y = v.align_y;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_OPACITY) {
        b.custom_opacity = true;
        b.opacity = v.opacity;
    }
    return b;
}
typedef struct {
    float root_gap, gap, content_gap, px, py, height, indicator, mark, shortcut, font, line;
} oracle_metrics;
static oracle_metrics oracle_measure(cui_questionnaire_options o) {
    if (o.design_size) {
        cui_design_metrics d;
        if (!cui_design_resolve(o.design_size, o.density > 0 ? o.density : 1, &d))
            return (oracle_metrics){0};
        return (oracle_metrics){2 * d.gap,
                                d.gap,
                                d.gap * .5f,
                                d.padding_x,
                                d.padding_y,
                                d.control_height,
                                d.indicator_size,
                                d.indicator_size * .5f,
                                d.indicator_size + 4,
                                d.font_size,
                                d.line_height};
    }
    switch (o.size) {
    case CUI_QUESTIONNAIRE_XSMALL:
        return (oracle_metrics){8, 6, 2, 8, 4, 28, 12, 5, 16, 12, 16};
    case CUI_QUESTIONNAIRE_SMALL:
        return (oracle_metrics){12, 8, 2, 10, 8, 36, 14, 6, 18, 14, 20};
    case CUI_QUESTIONNAIRE_LARGE:
        return (oracle_metrics){24, 12, 4, 16, 12, 48, 18, 10, 24, 18, 28};
    default:
        return (oracle_metrics){16, 10, 2, 12, 10, 44, 16, 8, 20, 16, 24};
    }
}
static cui_color oracle_opacity(cui_color color, float opacity) {
    color.a = (uint8_t)(color.a * opacity + .5f);
    return color;
}
static bool oracle_choice(cui_context *ui, cui_id id, const cui_theme *t,
                          cui_questionnaire_state *s, size_t item, size_t choice,
                          cui_questionnaire_options o,
                          const cui_questionnaire_keyboard_options *keyboard) {
    cui_questionnaire_keyboard_options k = {0};
    if (keyboard)
        k.shortcuts = keyboard->shortcuts;

    bool legacy_numbers = o.numeric_shortcuts;
    if (!ui || !id || !t || !s || !s->initialized || item != s->current || item >= s->count ||
        choice >= s->items[item].choice_count)
        return false;
    const cui_questionnaire_item *d = &s->items[item];
    const cui_questionnaire_choice *c = &d->choices[choice];
    bool disabled = o.disabled || s->storage.items[item].disabled ||
                    s->storage.choices[s->storage.items[item].choice_offset + choice].disabled;
    bool selected = cui_questionnaire_choice_selected(s, item, choice);
    cui_str label = c->label.length ? c->label : c->value;
    cui_interaction interaction = cui_interact(
        ui, id, d->multiple ? CUI_ROLE_CHECKBOX : CUI_ROLE_RADIO, label, disabled, selected, 0);
    bool changed = interaction.activated && !disabled;
    if (changed) {
        (void)cui_questionnaire_choose(s, item, choice, cui_frame_time(ui), cui_reduced_motion(ui));
        selected = cui_questionnaire_choice_selected(s, item, choice);
        cui_update_semantic(ui, id, selected, selected ? 1 : 0);
        cui_invalidate(ui);
    }
    oracle_metrics m = oracle_measure(o);
    bool invalid = cui_questionnaire_item_error(s, item).kind != CUI_QUESTIONNAIRE_VALID;
    size_t position = 0, total = 0;
    (void)cui_questionnaire_choice_position(s, item, choice, &position, &total);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.invalid = invalid,
                               .required = d->required,
                               .description = c->description,
                               .pos_in_set = position <= UINT_MAX ? (unsigned)position : 0,
                               .set_size = total <= UINT_MAX ? (unsigned)total : 0,
                               .pointer_activatable = true});
    char key = k.shortcuts == CUI_QUESTIONNAIRE_SHORTCUT_OFF
                   ? 0
                   : cui_questionnaire_shortcut(s, item, choice, legacy_numbers);
    cui_questionnaire_choice_view view = {.item = item,
                                          .choice = choice,
                                          .size = o.size,
                                          .design_size = o.design_size,
                                          .multiple = d->multiple,
                                          .selected = selected,
                                          .disabled = disabled,
                                          .invalid = invalid,
                                          .focused = cui_focused(ui) == id,
                                          .shortcut = key};
    float card_radius = o.size == CUI_QUESTIONNAIRE_XSMALL  ? t->radius
                        : o.size == CUI_QUESTIONNAIRE_LARGE ? t->radius * 2
                                                            : t->radius_lg;
    cui_color card_background =
        selected ? t->muted_background
                 : oracle_opacity(t->dark ? t->input : t->background, t->dark ? .2f : 0);
    if (interaction.hovered && !disabled)
        card_background = oracle_opacity(t->muted_background, .5f);
    cui_begin_box(ui, id,
                  (cui_box){.width = -1,
                            .padding = m.py,
                            .row = true,
                            .gap = 0,
                            .radius = card_radius,
                            .border_width = 1,
                            .border = view.focused ? t->focus
                                      : invalid    ? t->danger
                                      : selected   ? oracle_opacity(t->primary, .4f)
                                                   : t->input,
                            .background = card_background,
                            .custom_opacity = disabled,
                            .opacity = .5f});
    if (view.focused && !t->hide_focus_ring) {
        cui_color ring = t->focus;
        ring.a = 127;
        (void)cui_set_outline(
            ui, 3, ring, (cui_corner_radii){card_radius, card_radius, card_radius, card_radius});
    }
    /* Side spacers supply independent horizontal padding and the card's minimum
     * height without centering the indicator against multi-line descriptions. */
    cui_begin_box(
        ui, 0, (cui_box){.width = m.px - m.py, .fixed_width = true, .height = m.height - 2 * m.py});
    cui_end_box(ui);
    cui_begin_box(ui, 0, (cui_box){.width = -1, .row = true, .gap = m.gap});
    if (o.render_indicator) {
        cui_begin_box(ui, 0, (cui_box){0});
        cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.indicator) / 2)});
        cui_end_box(ui);
        o.render_indicator(ui, id ^ UINT64_C(0x751ec681a5920310), &view, o.render_user);
        cui_end_box(ui);
    } else {
        cui_begin_box(ui, 0,
                      (cui_box){.height = m.line,
                                .align_y = o.indicator_style ? CUI_ALIGN_START : CUI_ALIGN_CENTER});
        if (o.indicator_style) {
            cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.indicator) / 2)});
            cui_end_box(ui);
        }
        cui_begin_box(
            ui, 0,
            oracle_refine((cui_box){.width = m.indicator,
                                    .height = m.indicator,
                                    .radius = d->multiple ? t->radius / 2 : m.indicator / 2,
                                    .border_width = 1,
                                    .border = selected ? t->primary : t->input,
                                    .background = selected ? t->primary : t->input_background,
                                    .align_x = CUI_ALIGN_CENTER,
                                    .align_y = CUI_ALIGN_CENTER},
                          o.indicator_style));
        if (selected) {
            if (d->multiple)
                cui_checkmark(ui, m.indicator - 2, t->on_primary);
            else {
                cui_begin_box(ui, 0,
                              (cui_box){.width = m.mark,
                                        .height = m.mark,
                                        .radius = m.mark / 2,
                                        .background = t->on_primary});
                cui_end_box(ui);
            }
        }
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_begin_box(ui, 0,
                  oracle_refine((cui_box){.width = -1, .gap = m.content_gap}, o.content_style));
    if (o.render_content)
        o.render_content(ui, id ^ UINT64_C(0x1f0e69b892345708), &view, o.render_user);
    else {
        cui_text_with_line_height(ui, label, m.font, t->text, m.line);
        if (c->description.length)
            cui_text_with_line_height(ui, c->description, m.font > 14 ? m.font - 2 : 12, t->muted,
                                      m.line);
    }
    cui_end_box(ui);
    if (k.shortcuts != CUI_QUESTIONNAIRE_SHORTCUT_OFF && o.render_shortcut) {
        cui_begin_box(ui, 0, (cui_box){0});
        cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.shortcut) / 2)});
        cui_end_box(ui);
        o.render_shortcut(ui, id ^ UINT64_C(0x931ac84625c0a917), &view, o.render_user);
        cui_end_box(ui);
    } else if (key) {
        cui_begin_box(ui, 0,
                      (cui_box){.width = o.shortcut_style ? 0 : m.shortcut,
                                .height = m.line,
                                .align_y = o.shortcut_style ? CUI_ALIGN_START : CUI_ALIGN_CENTER});
        if (o.shortcut_style) {
            cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.shortcut) / 2)});
            cui_end_box(ui);
        }
        cui_begin_box(
            ui, 0,
            oracle_refine((cui_box){.width = m.shortcut,
                                    .height = m.shortcut,
                                    .background = t->background,
                                    .border_width = 1,
                                    .border = t->input,
                                    .radius = o.size == CUI_QUESTIONNAIRE_XSMALL  ? t->radius / 2
                                              : o.size == CUI_QUESTIONNAIRE_LARGE ? t->radius_lg
                                                                                  : t->radius,
                                    .align_x = CUI_ALIGN_CENTER,
                                    .align_y = CUI_ALIGN_CENTER},
                          o.shortcut_style));
        float shortcut_font = o.design_size                        ? m.font * .625f
                              : o.size == CUI_QUESTIONNAIRE_XSMALL ? 8
                              : o.size == CUI_QUESTIONNAIRE_SMALL  ? 9
                              : o.size == CUI_QUESTIONNAIRE_LARGE  ? 12
                                                                   : 10;
        cui_text_ex(ui, (cui_str){&key, 1}, shortcut_font, t->muted, 0,
                    (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM, .monospace = true});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui); /* answer row */
    cui_begin_box(ui, 0, (cui_box){.width = m.px - m.py, .fixed_width = true});
    cui_end_box(ui);
    cui_end_box(ui); /* card */
    return changed;
}
