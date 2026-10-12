/* SPDX-License-Identifier: MIT */
#include "cui/cui_status.h"
#include <math.h>
cui_tag_options cui_tag_options_default(void) {
    return (cui_tag_options){.variant = CUI_TAG_SECONDARY, .size = CUI_STATUS_MEDIUM};
}
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
