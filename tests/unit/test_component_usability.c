/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_edit.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void mail_row(cui_context *ui, float width, cui_str label) {
    assert(cui_begin_frame(ui, width, 120, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = width, .fixed_width = true, .row = true});
    assert(cui_text_ellipsis(
        ui, 2, label,
        (cui_text_ellipsis_options){.width = -1, .font_size = 16, .color = {0, 0, 0, 255}}));
    cui_begin_box(ui, 3, (cui_box){.width = 24, .height = 24, .fixed_width = true});
    cui_text(ui, cui_string("*"), 16, (cui_color){0, 0, 0, 255});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(fabsf(cui_bounds(ui, 3).w - 24) < .01f);
    assert(cui_bounds(ui, 3).x + 24 <= width + .01f);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].name.length == label.length);
    assert(!memcmp(sem[0].name.data, label.data, label.length));
    bool ellipsis = false;
    for (size_t i = 0; i < draw.count; ++i) {
        const cui_draw_command *d = &draw.commands[i];
        if (d->type != CUI_DRAW_TEXT || (d->text.length == 1 && d->text.data[0] == '*'))
            continue;
        assert(d->bounds.x + d->bounds.w <= cui_bounds(ui, 2).w + .01f);
        if (d->text.length == 3 && !memcmp(d->text.data, "…", 3))
            ellipsis = true;
        else {
            assert(d->text.length <= label.length);
            assert(cui_grapheme_boundary(label, d->text.length));
        }
    }
    assert(ellipsis == (cui_measure_text(ui, label, 16).width > width - 24 &&
                        cui_measure_text(ui, cui_string("…"), 16).width <= width - 24));
}
static void zero_ids(cui_context *ui) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 100, 100, .016f));
    for (size_t i = 0; i < 2; ++i) {
        cui_avatar(ui, 0, &theme, cui_string("Anonymous"), 24);
        cui_diagnostic diagnostic;
        assert(cui_latest_diagnostic(ui, &diagnostic));
        assert(diagnostic.detail == CUI_DIAGNOSTIC_ZERO_ID);
        assert(strstr(diagnostic.message, "nonzero ID"));
        assert(!strcmp(diagnostic.operation, "cui_avatar"));
        assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
        cui_clear_error(ui);
        assert(!cui_avatar_ex(ui, 0, &theme, cui_string("Anonymous"), (cui_avatar_options){0}));
        assert(cui_latest_diagnostic(ui, &diagnostic));
        assert(diagnostic.detail == CUI_DIAGNOSTIC_ZERO_ID);
        assert(!strcmp(diagnostic.operation, "cui_avatar_ex"));
        cui_clear_error(ui);
        assert(!cui_tag(ui, 0, &theme, cui_string("Label"), (cui_tag_options){0}));
        assert(cui_latest_diagnostic(ui, &diagnostic));
        assert(diagnostic.detail == CUI_DIAGNOSTIC_ZERO_ID);
        assert(!strcmp(diagnostic.operation, "cui_begin_tag"));
        cui_clear_error(ui);
    }
    assert(!cui_text_ellipsis(ui, 0, cui_string("Label"),
                              (cui_text_ellipsis_options){.width = 50, .font_size = 16}));
    cui_diagnostic diagnostic;
    assert(cui_latest_diagnostic(ui, &diagnostic));
    assert(diagnostic.detail == CUI_DIAGNOSTIC_ZERO_ID);
    assert(!strcmp(diagnostic.operation, "cui_text_ellipsis"));
    cui_clear_error(ui);
    /* Anonymous layout nodes remain legal. */
    cui_begin_box(ui, 0, (cui_box){0});
    cui_begin_box(ui, 0, (cui_box){0});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && draw.count == 0);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0);
}
static void missing_icons(cui_context *ui) {
    cui_theme theme = cui_theme_light();
    cui_str names[2] = {cui_string("unknown-debug-icon"), {0}};
    for (size_t i = 0; i < cui_icon_catalog_count(); ++i)
        if (cui_icon_query(cui_icon_catalog_name(i)) == CUI_ICON_CATALOG_NOT_EMBEDDED) {
            names[1] = cui_icon_catalog_name(i);
            break;
        }
    for (size_t n = 0; n < 2; ++n) {
        if (!names[n].length)
            continue; /* All-assets build has no nonembedded catalog entry. */
        for (unsigned debug = 0; debug < 2; ++debug) {
            assert(cui_begin_frame(ui, 100, 100, .016f));
            assert(cui_icon_named(ui, 40, &theme, names[n],
                                  (cui_icon_options){.debug_placeholder = debug != 0,
                                                     .custom_opacity = true,
                                                     .opacity = .5f,
                                                     .custom_color = true,
                                                     .color = {8, 16, 32, 200},
                                                     .accessible_name = cui_string("Missing")}) ==
                   CUI_ICON_NOT_FOUND);
            cui_draw_list draw = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            bool border = false;
            for (size_t i = 0; i < draw.count; ++i)
                if (draw.commands[i].type == CUI_DRAW_BORDER) {
                    border = true;
                    assert(draw.commands[i].color.a == 100);
                    assert(draw.commands[i].bounds.w == cui_bounds(ui, 40).w);
                }
            assert(border == (debug != 0));
            if (debug) {
                size_t count;
                const cui_semantic *sem = cui_semantics(ui, &count);
                assert(count == 1 && sem[0].role == CUI_ROLE_IMAGE && sem[0].name.length == 7);
            }
        }
    }
}
static void three_lines(cui_context *ui, void *user) {
    (void)user;
    cui_begin_box(ui, 20, (cui_box){.width = -1});
    cui_text(ui, cui_string("Sender"), 16, (cui_color){11, 22, 33, 255});
    cui_text(ui, cui_string("Subject"), 16, (cui_color){11, 22, 33, 255});
    cui_text(ui, cui_string("Preview"), 16, (cui_color){11, 22, 33, 255});
    cui_end_box(ui);
}
static void button_three_lines(cui_context *ui) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 220, 120, .016f));
    cui_button_ex(ui, 10, &theme, cui_string("Mail"),
                  (cui_button_options){.width = 200, .height = 96, .content = three_lines});
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    size_t lines = 0;
    float last_y = -1;
    cui_rect button = cui_bounds(ui, 10);
    for (size_t i = 0; i < draw.count; ++i) {
        const cui_draw_command *d = &draw.commands[i];
        if (d->type != CUI_DRAW_TEXT || d->color.r != 11)
            continue;
        assert(d->bounds.y > last_y);
        assert(d->bounds.y >= button.y && d->bounds.y + d->bounds.h <= button.y + button.h);
        last_y = d->bounds.y;
        ++lines;
    }
    assert(lines == 3);
}
int main(void) {
    assert(cui_icon_query(cui_string("does-not-exist")) == CUI_ICON_UNKNOWN);
    assert(cui_icon_query(cui_string("filter")) == cui_icon_query(cui_string("funnel")));
    for (size_t i = 0; i < cui_icon_catalog_count(); ++i) {
        cui_str svg;
        bool embedded = cui_icon_asset(cui_icon_catalog_name(i), &svg);
        assert(cui_icon_query(cui_icon_catalog_name(i)) ==
               (embedded ? CUI_ICON_EMBEDDED : CUI_ICON_CATALOG_NOT_EMBEDDED));
        assert(cui_icon_query(cui_icon_catalog_path(i)) ==
               cui_icon_query(cui_icon_catalog_name(i)));
    }
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    missing_icons(ui);
    zero_ids(ui);
    button_three_lines(ui);
    cui_str label =
        cui_string("e\xcc\x81👩‍👩‍👧‍👧 mail subject with a very long title");
    mail_row(ui, 170, label);
    mail_row(ui, 95, label); /* Resizing must apply immediately, without previous bounds. */
    mail_row(ui, 900, label);
    mail_row(ui, 30, label);
    mail_row(ui, 24, label);
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(!cui_text_ellipsis(ui, 2, cui_string("two\nlines"),
                              (cui_text_ellipsis_options){.width = 50, .font_size = 16}));
    cui_end_frame(ui);
    cui_destroy(ui);
    return 0;
}
