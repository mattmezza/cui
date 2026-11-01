/* SPDX-License-Identifier: MIT */
/* Prepared source checkpoint: not executed until an explicit compiler lease. */
#include "cui/cui_components.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static unsigned calls;
static void content(cui_context *ui, void *user) {
    assert(user == &calls);
    ++calls;
    cui_begin_box(ui, 101, (cui_box){.width = 80, .height = 40});
    cui_end_box(ui);
}
bool frozen_cui_badge_ex(cui_context *, cui_id, const cui_theme *, cui_badge_options,
                         cui_button_content_fn, void *);
void frozen_cui_badge(cui_context *, cui_id, const cui_theme *, cui_str);
_Static_assert(offsetof(cui_badge_icon_content, state) == 0, "state first");
_Static_assert(offsetof(cui_badge_icon_content, options) == sizeof(void *), "options offset");
_Static_assert(sizeof(cui_badge_icon_content) >= sizeof(void *) + sizeof(cui_icon_options),
               "descriptor contains full Icon options");
_Static_assert(offsetof(cui_badge_result, icon_error) >= sizeof(bool), "result layout");

static void legacy_control(void) {
    cui_context *a = cui_create((cui_config){.width = 200, .height = 100});
    cui_context *b = cui_create((cui_config){.width = 200, .height = 100});
    assert(a && b);
    cui_theme theme = cui_theme_light();
    for (unsigned mode = 0; mode < 5; ++mode) {
        cui_badge_options options = cui_badge_defaults();
        options.variant = mode == 2 ? CUI_BADGE_DOT : mode == 3 ? CUI_BADGE_ICON : CUI_BADGE_COUNT;
        options.count = mode == 1 ? 0 : 100;
        options.icon = cui_string("check");
        assert(cui_begin_frame(a, 200, 100, .016f));
        if (mode == 4)
            cui_badge(a, 100, &theme, cui_string("Legacy"));
        else
            assert(cui_badge_ex(a, 100, &theme, options, content, &calls));
        cui_draw_list da = cui_end_frame(a);
        assert(cui_begin_frame(b, 200, 100, .016f));
        if (mode == 4)
            frozen_cui_badge(b, 100, &theme, cui_string("Legacy"));
        else
            assert(frozen_cui_badge_ex(b, 100, &theme, options, content, &calls));
        cui_draw_list db = cui_end_frame(b);
        assert(da.count == db.count);
        cui_rect ra = cui_bounds(a, 100), rb = cui_bounds(b, 100);
        assert(ra.x == rb.x && ra.y == rb.y && ra.w == rb.w && ra.h == rb.h);
        size_t na, nb;
        const cui_semantic *sa = cui_semantics(a, &na), *sb = cui_semantics(b, &nb);
        assert(na == nb);
        for (size_t i = 0; i < na; ++i) {
            assert(sa[i].role == sb[i].role && sa[i].name.length == sb[i].name.length);
            assert(!memcmp(sa[i].name.data, sb[i].name.data, sa[i].name.length));
        }
    }
    cui_destroy(a);
    cui_destroy(b);
}

/* Independent shell tree + ordinary Icon must produce identical software RGBA.
 * Different contexts retain their own lists; source replacement happens before
 * rendering to witness old-mask lifetime, not merely cache counters. */
static void pixel_control(void) {
    cui_context *a = cui_create((cui_config){.width = 96, .height = 96});
    cui_context *b = cui_create((cui_config){.width = 96, .height = 96});
    cui_software *renderer = cui_software_create(96, 96, NULL);
    assert(a && b && renderer);
    cui_theme theme = cui_theme_light();
    cui_icon_state state = {0};
    assert(cui_icon_set_data(&state, cui_string("<svg width=\"16\" height=\"16\"><rect x=\"1\" "
                                                "y=\"2\" width=\"5\" height=\"9\"/></svg>")));
    for (unsigned turn = 0; turn < 4; ++turn) {
        cui_icon_constraints limits = {.max_width = 12, .custom_max_width = true};
        cui_box style = {.opacity = .75f};
        cui_badge_icon_content icon = {
            .state = &state,
            .options = {.size = CUI_ICON_CUSTOM,
                        .custom_size = 14,
                        .custom_color = true,
                        .color = {180, 30, 90, 128},
                        .custom_transform = true,
                        .transform = cui_icon_rotation(turn * 1.5707963f),
                        .constraints = &limits,
                        .style = &style}};
        cui_badge_options options = cui_badge_defaults();
        options.variant = CUI_BADGE_ICON;
        assert(cui_begin_frame(a, 96, 96, .016f));
        assert(cui_badge_with_icon(a, 100, &theme, options, &icon, content, &calls).icon_error ==
               CUI_ICON_OK);
        cui_draw_list da = cui_end_frame(a);
        assert(cui_begin_frame(b, 96, 96, .016f));
        cui_begin_box(b, 100, (cui_box){0});
        content(b, &calls);
        cui_begin_absolute_box(b, 200,
                               (cui_box){.align_x = CUI_ALIGN_END, .align_y = CUI_ALIGN_END},
                               (cui_rect){0, 0, -1, -1});
        cui_begin_box(b, 300,
                      (cui_box){.width = 16,
                                .height = 16,
                                .background = theme.danger,
                                .radius = 1024,
                                .border = theme.background,
                                .border_width = 1,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_icon_options artwork = icon.options;
        artwork.decorative = true;
        artwork.inherited_text_size = 10;
        artwork.custom_inherited_color = true;
        artwork.inherited_color = (cui_color){255, 255, 255, 255};
        assert(cui_icon(b, 400, &theme, &state, artwork) == CUI_ICON_OK);
        cui_end_box(b);
        cui_end_box(b);
        cui_end_box(b);
        cui_draw_list db = cui_end_frame(b);
        assert(cui_icon_set_empty(&state));
        assert(cui_software_render(renderer, &da, (cui_color){0, 0, 0, 0}));
        unsigned w, h;
        const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
        size_t bytes = (size_t)w * h * 4;
        unsigned char *saved = malloc(bytes);
        assert(saved);
        memcpy(saved, pixels, bytes);
        assert(cui_software_render(renderer, &db, (cui_color){0, 0, 0, 0}));
        pixels = cui_software_pixels(renderer, &w, &h);
        assert(!memcmp(saved, pixels, bytes));
        bool covered = false;
        for (size_t i = 3; i < bytes; i += 4)
            covered |= pixels[i] != 0;
        assert(covered);
        free(saved);
        assert(cui_icon_set_data(&state, cui_string("<svg width=\"16\" height=\"16\"><rect x=\"1\" "
                                                    "y=\"2\" width=\"5\" height=\"9\"/></svg>")));
    }
    cui_destroy(a);
    cui_destroy(b);
    cui_software_destroy(renderer);
    cui_icon_state_destroy(&state);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_icon_state state;
    cui_icon_state_init(&state);
    assert(cui_icon_set_data(&state,
                             cui_string("<svg width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
                                        "<path d=\"M1 1H14V5H5V14H1Z\"/></svg>")));
    cui_badge_options badge = cui_badge_defaults();
    badge.variant = CUI_BADGE_ICON;
    cui_icon_constraints constraints = {.max_width = 18, .custom_max_width = true};
    cui_box style = {.background = {17, 23, 31, 255}, .radius = 2};
    cui_badge_icon_content icon = {.state = &state,
                                   .options = {.size = CUI_ICON_CUSTOM,
                                               .custom_size = 23,
                                               .custom_color = true,
                                               .color = {211, 19, 73, 128},
                                               .custom_transform = true,
                                               .transform = {0, 1, -1, 0, 0, 0},
                                               .style = &style,
                                               .constraints = &constraints,
                                               .accessible_name = {"Must remain decorative", 22}}};
    const cui_id marker = cui_badge_indicator_id(100);
    const cui_id artwork = marker ^ UINT64_C(0x2c38f42991a75e63);
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_badge_result result = cui_badge_with_icon(ui, 100, &theme, badge, &icon, content, &calls);
    assert(result.submitted && result.icon_error == CUI_ICON_OK && calls == 1);
    cui_end_frame(ui);
    assert(cui_bounds(ui, 100).w == 80 && cui_bounds(ui, 100).h == 40);
    assert(cui_bounds(ui, marker).w == 16 && cui_bounds(ui, marker).h == 16);
    assert(cui_bounds(ui, artwork).w == 18 && cui_bounds(ui, artwork).h == 23);
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].role == CUI_ROLE_STATUS);
    assert(!icon.options.decorative && icon.options.inherited_text_size == 0);
    assert(icon.options.color.r == 211 && style.radius == 2 && constraints.max_width == 18);
    /* Structural rejection is atomic; artwork validation is explicitly later. */
    assert(cui_begin_frame(ui, 400, 200, .016f));
    badge.variant = CUI_BADGE_DOT;
    result = cui_badge_with_icon(ui, 100, &theme, badge, &icon, content, &calls);
    assert(!result.submitted && result.icon_error == CUI_ICON_INVALID && calls == 1);
    badge.variant = CUI_BADGE_ICON;
    assert(!cui_badge_with_icon(ui, 100, &theme, badge, NULL, content, &calls).submitted);
    icon.options.custom_size = NAN;
    result = cui_badge_with_icon(ui, 100, &theme, badge, &icon, content, &calls);
    assert(result.submitted && result.icon_error == CUI_ICON_INVALID && calls == 2);
    cui_end_frame(ui);
    assert(cui_bounds(ui, marker).w == 16);
    cui_destroy(ui);
    cui_icon_state_destroy(&state);
    legacy_control();
    pixel_control();
    return 0;
}
