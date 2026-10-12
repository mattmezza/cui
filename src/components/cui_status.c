/* SPDX-License-Identifier: MIT */
#include "cui/cui_status.h"
#include <math.h>
static cui_color cui_status_alpha(cui_color c, float opacity) {
    c.a = (uint8_t)(c.a * opacity);
    return c;
}
bool cui_tag_resolve(const cui_theme *t, cui_tag_options o, bool hovered, cui_tag_style *out) {
    if (!t || !out || o.variant > CUI_TAG_PALETTE || o.variant < CUI_TAG_SECONDARY ||
        o.size > CUI_STATUS_LARGE || o.size < CUI_STATUS_MEDIUM || !isfinite(o.radius) ||
        o.radius < 0 || !isfinite(t->radius) || t->radius < 0 || !isfinite(t->font_size) ||
        t->font_size <= 0)
        return false;
    cui_color bg = t->surface, fg = o.outline ? t->muted : t->text, border = t->border;
    switch (o.variant) {
    case CUI_TAG_PRIMARY:
        bg = border = t->primary;
        fg = o.outline ? bg : t->on_primary;
        break;
    case CUI_TAG_DANGER:
        bg = border = t->danger;
        fg = o.outline ? bg : t->on_danger;
        break;
    case CUI_TAG_SUCCESS:
        bg = border = t->success;
        fg = o.outline ? bg : t->on_success;
        break;
    case CUI_TAG_WARNING:
        bg = border = t->warning;
        fg = o.outline ? bg : t->on_warning;
        break;
    case CUI_TAG_INFO:
        bg = border = t->info;
        fg = o.outline ? bg : t->on_info;
        break;
    case CUI_TAG_CUSTOM:
        bg = o.background;
        fg = o.foreground;
        border = o.border;
        break;
    case CUI_TAG_PALETTE:
        bg = o.palette.dark ? cui_status_alpha(o.palette.scale950, .5f) : o.palette.scale50;
        border = o.palette.dark ? cui_status_alpha(o.palette.scale800, .5f) : o.palette.scale200;
        fg = o.palette.dark ? o.palette.scale300 : o.palette.scale600;
        break;
    default:
        break;
    }
    bool small = o.size == CUI_STATUS_SMALL || o.size == CUI_STATUS_XSMALL;
    /* Reference rem typography: 10/12/14px at the standard 16px root. */
    float root = t->font_size / 0.875f;
    cui_tag_style s = {.background = o.outline ? (cui_color){0} : bg,
                       .foreground = fg,
                       .border = border,
                       .font_size = root * (small                        ? .625f
                                            : o.size == CUI_STATUS_LARGE ? .875f
                                                                         : .75f),
                       .padding_x = small ? 6 : 10,
                       .padding_y = small ? 2 : 4,
                       .radius = o.pill            ? 100000
                                 : o.custom_radius ? o.radius
                                 : small           ? t->radius / 2
                                                   : t->radius};
    if (hovered) {
        s.background = cui_status_alpha(s.background, .9f);
        s.foreground = cui_status_alpha(s.foreground, .9f);
        s.border = cui_status_alpha(s.border, .9f);
    }
    *out = s;
    return true;
}

/* Six required scales from frozen component/theme/default-colors.json. */
bool cui_tag_named_palette(cui_status_color name, bool dark, cui_tag_palette *out) {
    static const cui_color colors[21][6] = {{{255, 255, 255, 255},
                                             {255, 255, 255, 255},
                                             {255, 255, 255, 255},
                                             {255, 255, 255, 255},
                                             {255, 255, 255, 255},
                                             {255, 255, 255, 255}},
                                            {{0, 0, 0, 255},
                                             {0, 0, 0, 255},
                                             {0, 0, 0, 255},
                                             {0, 0, 0, 255},
                                             {0, 0, 0, 255},
                                             {0, 0, 0, 255}},
                                            {{250, 250, 250, 255},
                                             {229, 229, 229, 255},
                                             {212, 212, 212, 255},
                                             {82, 82, 82, 255},
                                             {38, 38, 38, 255},
                                             {10, 10, 10, 255}},
                                            {{249, 250, 251, 255},
                                             {229, 231, 235, 255},
                                             {209, 213, 219, 255},
                                             {75, 85, 99, 255},
                                             {31, 41, 55, 255},
                                             {3, 7, 18, 255}},
                                            {{254, 242, 242, 255},
                                             {254, 202, 202, 255},
                                             {252, 165, 165, 255},
                                             {220, 38, 38, 255},
                                             {153, 27, 27, 255},
                                             {69, 10, 10, 255}},
                                            {{255, 247, 237, 255},
                                             {254, 215, 170, 255},
                                             {253, 186, 116, 255},
                                             {234, 88, 12, 255},
                                             {154, 52, 18, 255},
                                             {67, 20, 7, 255}},
                                            {{255, 251, 235, 255},
                                             {253, 230, 138, 255},
                                             {252, 211, 77, 255},
                                             {217, 119, 6, 255},
                                             {146, 64, 14, 255},
                                             {69, 26, 3, 255}},
                                            {{254, 252, 232, 255},
                                             {254, 240, 138, 255},
                                             {253, 224, 71, 255},
                                             {202, 138, 4, 255},
                                             {133, 77, 14, 255},
                                             {66, 32, 6, 255}},
                                            {{247, 254, 231, 255},
                                             {217, 249, 157, 255},
                                             {190, 242, 100, 255},
                                             {101, 163, 13, 255},
                                             {63, 98, 18, 255},
                                             {26, 46, 5, 255}},
                                            {{240, 253, 244, 255},
                                             {187, 247, 208, 255},
                                             {134, 239, 172, 255},
                                             {22, 163, 74, 255},
                                             {22, 101, 52, 255},
                                             {5, 46, 22, 255}},
                                            {{236, 253, 245, 255},
                                             {167, 243, 208, 255},
                                             {110, 231, 183, 255},
                                             {5, 150, 105, 255},
                                             {6, 95, 70, 255},
                                             {2, 44, 34, 255}},
                                            {{240, 253, 250, 255},
                                             {153, 246, 228, 255},
                                             {94, 234, 212, 255},
                                             {13, 148, 136, 255},
                                             {17, 94, 89, 255},
                                             {4, 47, 46, 255}},
                                            {{236, 254, 255, 255},
                                             {165, 243, 252, 255},
                                             {103, 232, 249, 255},
                                             {8, 145, 178, 255},
                                             {21, 94, 117, 255},
                                             {8, 51, 68, 255}},
                                            {{240, 249, 255, 255},
                                             {186, 230, 253, 255},
                                             {125, 211, 252, 255},
                                             {2, 132, 199, 255},
                                             {7, 89, 133, 255},
                                             {8, 47, 73, 255}},
                                            {{239, 246, 255, 255},
                                             {191, 219, 254, 255},
                                             {147, 197, 253, 255},
                                             {37, 99, 235, 255},
                                             {30, 64, 175, 255},
                                             {23, 37, 84, 255}},
                                            {{238, 242, 255, 255},
                                             {199, 210, 254, 255},
                                             {165, 180, 252, 255},
                                             {79, 70, 229, 255},
                                             {55, 48, 163, 255},
                                             {30, 27, 75, 255}},
                                            {{245, 243, 255, 255},
                                             {221, 214, 254, 255},
                                             {196, 181, 253, 255},
                                             {124, 58, 237, 255},
                                             {91, 33, 182, 255},
                                             {30, 27, 75, 255}},
                                            {{250, 245, 255, 255},
                                             {233, 213, 255, 255},
                                             {216, 180, 254, 255},
                                             {147, 51, 234, 255},
                                             {107, 33, 168, 255},
                                             {59, 7, 100, 255}},
                                            {{253, 244, 255, 255},
                                             {245, 208, 254, 255},
                                             {240, 171, 252, 255},
                                             {192, 38, 211, 255},
                                             {134, 25, 143, 255},
                                             {74, 4, 78, 255}},
                                            {{253, 242, 248, 255},
                                             {251, 207, 232, 255},
                                             {249, 168, 212, 255},
                                             {219, 39, 119, 255},
                                             {157, 23, 77, 255},
                                             {80, 7, 36, 255}},
                                            {{255, 241, 242, 255},
                                             {254, 205, 211, 255},
                                             {253, 164, 175, 255},
                                             {225, 29, 72, 255},
                                             {159, 18, 57, 255},
                                             {76, 5, 25, 255}}};
    if (!out || name < CUI_STATUS_COLOR_WHITE || name > CUI_STATUS_COLOR_ROSE)
        return false;
    *out = (cui_tag_palette){colors[name][0],
                             colors[name][1],
                             colors[name][2],
                             colors[name][3],
                             colors[name][4],
                             colors[name][5],
                             dark};
    return true;
}

bool cui_begin_tag(cui_context *ui, cui_id id, const cui_theme *t, cui_tag_options o,
                   cui_tag_style *out) {
    cui_tag_style s;
    if (!ui || !cui_tag_resolve(t, o, cui_hovered(ui, id), &s))
        return false;
    /* Separate horizontal/vertical padding using inner composition, since the
     * core box exposes uniform padding. */
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .padding = s.padding_y,
                            .radius = s.radius,
                            .background = s.background,
                            .border = s.border,
                            .border_width = 1,
                            .align_y = CUI_ALIGN_CENTER});
    cui_begin_box(ui, id ^ UINT64_C(0xe937dbc482156a30),
                  (cui_box){.width = s.padding_x - s.padding_y, .height = 1});
    cui_end_box(ui);
    cui_begin_box(ui, id ^ UINT64_C(0xa037dbc482156a30),
                  (cui_box){.row = true, .align_y = CUI_ALIGN_CENTER});
    if (out)
        *out = s;
    return true;
}
void cui_end_tag(cui_context *ui, cui_id id, const cui_tag_style *s) {
    if (ui && s) {
        cui_end_box(ui);
        cui_begin_box(ui, id ^ UINT64_C(0xb037dbc482156a30),
                      (cui_box){.width = s->padding_x - s->padding_y, .height = 1});
        cui_end_box(ui);
        cui_end_box(ui);
    }
}
bool cui_tag(cui_context *ui, cui_id id, const cui_theme *t, cui_str text, cui_tag_options o) {
    cui_tag_style s;
    if (!cui_begin_tag(ui, id, t, o, &s))
        return false;
    cui_interact(ui, id, CUI_ROLE_LABEL, text, false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_text(ui, text, s.font_size, s.foreground);
    cui_end_tag(ui, id, &s);
    return true;
}
#include "cui/cui_content.h"
static cui_id cui_status_child_id(cui_id id, size_t i) {
    return id ^ (UINT64_C(0x95d7e38a461bc203) + UINT64_C(0x9e3779b97f4a7c15) * (i + 1));
}
static void cui_status_spacer(cui_context *ui, cui_id id, float width) {
    cui_begin_box(ui, id, (cui_box){.width = width, .height = 1, .fixed_width = true});
    cui_end_box(ui);
}
static void cui_status_shimmer(cui_context *ui, cui_id id, cui_rect b, cui_str text, float font,
                               cui_color color, float phase, float spread, bool absolute,
                               bool dark) {
    if (b.w <= 0 || b.h <= 0)
        return;
    float half = absolute ? fmaxf(1, spread) : b.w * fmaxf(.05f, fminf(spread, 1)),
          padding = half / b.w + .05f, center = (phase * (1 + 2 * padding) - padding) * b.w;
    color.a = (uint8_t)(255 * (1 - powf(1 - (dark ? .6f : .75f), 1.0f / 12)));
    for (size_t layer = 0; layer < 12; layer++) {
        float radius = half * (1 - (float)layer / 12), left = fmaxf(0, center - radius),
              right = fminf(b.w, center + radius);
        if (right <= left)
            continue;
        cui_begin_portal(ui, cui_status_child_id(id, layer),
                         (cui_box){.width = right - left,
                                   .height = b.h,
                                   .fixed_width = true,
                                   .fixed_height = true,
                                   .clip = true,
                                   .offset_x = -left,
                                   .inert = true},
                         (cui_rect){b.x + left, b.y, right - left, b.h}, 2);
        cui_text(ui, text, font, color);
        cui_end_box(ui);
    }
}
bool cui_marker(cui_context *ui, cui_id id, const cui_theme *t, cui_marker_state *state,
                const cui_marker_child *children, size_t count, cui_marker_options o) {
    cui_marker_plan p;
    if (!ui || !t || !state || !cui_base_marker(o, children, count, &p))
        return false;
    if (p.animated && (!state->initialized || state->last_phase != p.phase))
        cui_invalidate(ui);
    state->initialized = true;
    state->last_phase = p.phase;
    float widths[CUI_MARKER_MAX_CHILDREN], total = p.spinner ? 16 : 0, font = t->font_size;
    size_t slots = count + (p.spinner ? 1 : 0);
    for (size_t i = 0; i < count; i++) {
        widths[i] = children[i].kind == CUI_MARKER_TEXT
                        ? cui_measure_text(ui, children[i].text, font).width
                    : children[i].width > 0               ? children[i].width
                    : children[i].kind == CUI_MARKER_ICON ? 16
                                                          : 0;
        total += widths[i];
    }
    if (slots)
        total += 8 * (slots - 1);
    float remaining = fmaxf(0, o.width - total),
          line = o.variant == CUI_MARKER_SEPARATOR
                     ? remaining / (p.alignment == CUI_MARKER_ALIGN_CENTER ? 2 : 1)
                     : 0;
    cui_begin_box(ui, id,
                  (cui_box){.width = o.width,
                            .border = t->border,
                            .custom_border_edges = o.variant == CUI_MARKER_BORDER,
                            .border_edges = {.bottom = o.variant == CUI_MARKER_BORDER ? 1 : 0}});
    cui_begin_box(ui, cui_status_child_id(id, 103),
                  (cui_box){.row = true,
                            .width = o.width,
                            .gap = 8,
                            .align_y = CUI_ALIGN_CENTER,
                            .height = slots ? 0 : 16});
    if (o.role != CUI_ROLE_NONE) {
        cui_interact(ui, id, o.role, o.name, false, false, 0);
        cui_set_tab_stop(ui, id, false);
    }
    if (o.variant == CUI_MARKER_SEPARATOR && p.alignment != CUI_MARKER_ALIGN_START) {
        cui_begin_box(ui, cui_status_child_id(id, 100),
                      (cui_box){.width = fmaxf(0, line - 8),
                                .height = 1,
                                .fixed_width = true,
                                .background = t->border});
        cui_end_box(ui);
    } else if (o.variant != CUI_MARKER_SEPARATOR && p.alignment != CUI_MARKER_ALIGN_START)
        cui_status_spacer(
            ui, cui_status_child_id(id, 100),
            fmaxf(0, remaining / (p.alignment == CUI_MARKER_ALIGN_CENTER ? 2 : 1) - 8));
    if (p.spinner)
        cui_spinner(ui, cui_status_child_id(id, 101), t, 16, p.phase, o.reduced_motion);
    for (size_t i = 0; i < count; i++) {
        const cui_marker_child *c = &children[i];
        cui_id child = cui_status_child_id(id, i);
        cui_begin_box(ui, child,
                      (cui_box){.width = widths[i],
                                .fixed_width = widths[i] > 0,
                                .height = c->kind == CUI_MARKER_ICON ? 16 : 0,
                                .row = true,
                                .align_x = p.alignment == CUI_MARKER_ALIGN_END ? CUI_ALIGN_END
                                           : p.alignment == CUI_MARKER_ALIGN_CENTER
                                               ? CUI_ALIGN_CENTER
                                               : CUI_ALIGN_START,
                                .align_y = CUI_ALIGN_CENTER});
        if (c->kind == CUI_MARKER_TEXT) {
            cui_text(ui, c->text, font, t->muted);
            cui_track_bounds(ui, child, &state->text_bounds[i]);
        } else if (c->render)
            c->render(ui, cui_status_child_id(child, 49), c->user, font, t->muted,
                      c->kind == CUI_MARKER_CONTENT ? p.rich_opacity : 1);
        else if (c->text.length)
            cui_text(ui, c->text, font, t->muted);
        cui_end_box(ui);
        if (c->kind == CUI_MARKER_TEXT && o.loading && !o.reduced_motion &&
            o.loading_style == CUI_MARKER_SHIMMER) {
            bool dark = t->background.r + t->background.g + t->background.b < 384;
            cui_color highlight = o.custom_highlight ? o.highlight : dark ? t->text : t->background;
            cui_status_shimmer(ui, cui_status_child_id(child, 50), state->text_bounds[i], c->text,
                               font, highlight, p.phase, o.shimmer_spread, o.absolute_spread, dark);
        }
    }
    if (o.variant == CUI_MARKER_SEPARATOR && p.alignment != CUI_MARKER_ALIGN_END) {
        cui_begin_box(ui, cui_status_child_id(id, 102),
                      (cui_box){.width = fmaxf(0, line - 8),
                                .height = 1,
                                .fixed_width = true,
                                .background = t->border});
        cui_end_box(ui);
    }
    if (o.variant != CUI_MARKER_SEPARATOR && p.alignment == CUI_MARKER_ALIGN_CENTER)
        cui_status_spacer(ui, cui_status_child_id(id, 102), fmaxf(0, remaining / 2 - 8));
    cui_end_box(ui);
    if (o.variant == CUI_MARKER_BORDER) {
        cui_begin_box(ui, cui_status_child_id(id, 104), (cui_box){.height = 8, .width = 1});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return true;
}
