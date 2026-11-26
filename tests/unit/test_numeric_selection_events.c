/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_numeric_selection_events.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    size_t value, calls;
    cui_pagination_change_event page_event;
    cui_stepper_change_event step_event;
    cui_rating_change_event rating_event;
    float rating;
    cui_draw_list draw;
    bool teardown_requested;
} fixture;
static fixture make(void) {
    fixture f = {0};
    f.ui = cui_create((cui_config){.width = 1400, .height = 800});
    assert(f.ui);
    return f;
}
static void metadata(fixture *f, const cui_component_event_metadata *e) {
    assert(e->kind == CUI_COMPONENT_EVENT_CHANGE && e->target == 2);
    assert(e->source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(e->flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(e->frame_sequence == cui_frame_sequence(f->ui));
}
static void committed_semantics(fixture *f, float value) {
    size_t count;
    const cui_semantic *items = cui_semantics(f->ui, &count);
    bool found = false;
    for (size_t i = 0; i < count; ++i)
        if (items[i].id == 2) {
            assert(items[i].value == value);
            found = true;
        }
    assert(found); /* Observation follows the real reducer's final semantics. */
}
static void page_changed(void *user, const cui_pagination_change_event *e) {
    fixture *f = user;
    metadata(f, &e->event);
    assert(e->after == f->value);
    committed_semantics(f, (float)e->after);
    f->page_event = *e;
    f->calls++;
    f->teardown_requested = true;
}
static void key(fixture *f, cui_id id, cui_key code) {
    cui_focus(f->ui, id);
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code}));
}
static void activate(fixture *f, unsigned part) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                              .accessibility_target = cui_numeric_part_id(2, part),
                                              .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
}
static void click(fixture *f, unsigned part, float fraction) {
    cui_rect b = cui_bounds(f->ui, cui_numeric_part_id(2, part));
    assert(b.w > 0 && b.h > 0);
    cui_event e = {.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w * fraction, .y = b.y + b.h * .5f};
    assert(cui_queue_event(f->ui, e));
    e.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f->ui, e));
}
static cui_numeric_selection_event_result page_frame(fixture *f, bool styled, bool legacy,
                                                     size_t total, bool compact, bool disabled,
                                                     unsigned passive, bool handler) {
    assert(cui_begin_frame(f->ui, 1400, 800, .016f));
    cui_begin_box(
        f->ui, 900,
        (cui_box){
            .width = 1300, .height = 700, .inert = passive == 1, .layout_only = passive == 2});
    cui_theme t = cui_theme_light();
    cui_numeric_selection_event_result r = {0};
    if (legacy)
        r.control = styled ? cui_pagination(f->ui, 2, &t, &f->value, total, compact, disabled)
                           : cui_base_pagination(f->ui, 2, &f->value, total, compact, disabled);
    else {
        cui_pagination_event_handler h = {handler ? page_changed : NULL, f};
        r = styled ? cui_pagination_events(f->ui, 2, &t, &f->value, total, compact, disabled, h)
                   : cui_base_pagination_events(f->ui, 2, &f->value, total, compact, disabled, h);
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static cui_numeric_selection_event_result sized_page_frame(fixture *f,
                                                           cui_pagination_options options) {
    assert(cui_begin_frame(f->ui, 220, 800, .016f));
    cui_theme theme = cui_theme_light();
    cui_numeric_selection_event_result r = cui_pagination_events_ex(
        f->ui, 2, &theme, &f->value, 20, options, (cui_pagination_event_handler){page_changed, f});
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void sized_pagination(void) {
    fixture f = make();
    f.value = 3;
    cui_pagination_options options = {
        .compact = true, .design_size = CUI_DESIGN_LG, .available_width = 180};
    assert(!sized_page_frame(&f, options).control);
    click(&f, 3, .5f);
    cui_numeric_selection_event_result r = sized_page_frame(&f, options);
    assert(r.control && r.change_count == 1 && f.calls == 1 && f.value == 4);
    activate(&f, 4);
    activate(&f, 2);
    r = sized_page_frame(&f, options);
    assert(r.control && r.change_count == 1 && f.calls == 2 && f.value == 19);
    options.disabled = true;
    activate(&f, 4);
    r = sized_page_frame(&f, options);
    assert(!r.control && !r.change_count && f.calls == 2 && f.value == 19);
    options.disabled = false;
    options.available_width = NAN;
    f.value = 22;
    r = sized_page_frame(&f, options);
    assert(!r.control && !r.change_count && f.calls == 2 && f.value == 22);
    assert(cui_bounds(f.ui, cui_numeric_part_id(2, 4)).h == 0);
    cui_destroy(f.ui);
}
static void pagination_modes(void) {
    fixture f = make();
    f.value = 3;
    cui_pagination_options options = {
        .design_size = CUI_DESIGN_BASE, .available_width = 180, .visible_pages = 10};
    assert(!sized_page_frame(&f, options).control);
    size_t count;
    const cui_semantic *nodes = cui_semantics(f.ui, &count);
    assert(count == 15); /* Navigation group, four text buttons, ten numbered buttons. */
    cui_rect eighth = {0};
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].name.length == 1 && nodes[i].name.data[0] == '8')
            eighth = cui_bounds(f.ui, nodes[i].id);
    assert(eighth.w > 0 && eighth.h > 0);
    cui_event input = {.type = CUI_EVENT_POINTER_DOWN,
                       .x = eighth.x + .5f * eighth.w,
                       .y = eighth.y + .5f * eighth.h};
    assert(cui_queue_event(f.ui, input));
    input.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f.ui, input));
    assert(sized_page_frame(&f, options).change_count == 1 && f.value == 8 && f.calls == 1);

    activate(&f, 4);
    options.icon_only = true;
    options.compact = true; /* Explicit icon mode takes priority. */
    assert(!sized_page_frame(&f, options).control && f.value == 8 && f.calls == 1);
    nodes = cui_semantics(f.ui, &count);
    assert(count == 3 && nodes[1].id == cui_numeric_part_id(2, 2) &&
           nodes[2].id == cui_numeric_part_id(2, 3));
    assert(nodes[1].name.length == 8 && !memcmp(nodes[1].name.data, "Previous", 8));
    assert(nodes[2].name.length == 4 && !memcmp(nodes[2].name.data, "Next", 4));
    size_t icons = 0;
    for (size_t i = 0; i < f.draw.count; ++i) {
        const cui_draw_command *c = &f.draw.commands[i];
        assert(c->type != CUI_DRAW_TEXT);
        if (c->type == CUI_DRAW_IMAGE) {
            assert(c->bounds.w == 16 && c->bounds.h == 16 && c->bitmap.bitmap.pixels);
            ++icons;
        }
    }
    assert(icons == 2);
    key(&f, nodes[2].id, CUI_KEY_ENTER);
    assert(sized_page_frame(&f, options).change_count == 1 && f.value == 9 && f.calls == 2);
    activate(&f, 2);
    assert(sized_page_frame(&f, options).change_count == 1 && f.value == 8 && f.calls == 3);
    options.disabled = true;
    click(&f, 3, .5f);
    assert(!sized_page_frame(&f, options).control && f.value == 8 && f.calls == 3);
    cui_destroy(f.ui);
}
static const cui_semantic *pagination_node(const fixture *f, cui_role role, const char *name) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(f->ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].role == role && nodes[i].name.length == strlen(name) &&
            !memcmp(nodes[i].name.data, name, nodes[i].name.length))
            return &nodes[i];
    return NULL;
}
static void pagination_key(fixture *f, cui_key code) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code}));
}
static void pagination_action(fixture *f, cui_id target) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                              .accessibility_target = target,
                                              .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
}
static cui_numeric_selection_event_result
ellipsis_frame(fixture *f, size_t total, cui_pagination_options options, bool passive, bool omit) {
    assert(cui_begin_frame(f->ui, 400, 420, .016f));
    cui_theme theme = cui_theme_light();
    cui_numeric_selection_event_result result = {0};
    if (!omit) {
        cui_begin_box(f->ui, 911, (cui_box){.inert = passive});
        result = cui_pagination_events_ex(f->ui, 2, &theme, &f->value, total, options,
                                          (cui_pagination_event_handler){page_changed, f});
        cui_end_box(f->ui);
    }
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return result;
}
static void pagination_localized_navigation(void) {
    fixture f = make();
    f.value = 3;
    char next[] = {'S', 'u', 'i', 'v', 'a', 'n', 't', 'e'};
    const cui_pagination_labels labels = {
        .group = CUI_STR("Pages de résultats"),
        .first = CUI_STR("Première"),
        .previous = CUI_STR("Précédente"),
        .next = {next, sizeof(next)},
        .last = CUI_STR("Dernière"),
        .later_pages = CUI_STR("Pages suivantes"),
    };
    cui_pagination_options options = {.design_size = CUI_DESIGN_BASE,
                                      .available_width = 352,
                                      .visible_pages = 5,
                                      .ellipsis_viewport = {0, 0, 400, 420},
                                      .labels = &labels};
    assert(!ellipsis_frame(&f, 20, options, false, false).control);
    assert(pagination_node(&f, CUI_ROLE_GROUP, "Pages de résultats"));
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Première"));
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Dernière"));
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Suivante"));
    next[0] = 'X';
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Suivante"));
    bool copied_label = false;
    for (size_t i = 0; i < f.draw.count; ++i) {
        const cui_draw_command *c = &f.draw.commands[i];
        copied_label |= c->type == CUI_DRAW_TEXT && c->text.length == sizeof(next) &&
                        !memcmp(c->text.data, "Suivante", sizeof(next));
    }
    assert(copied_label);
    next[0] = 'S';

    activate(&f, 4); /* Queued while the old layout still offers Last. */
    options.navigation = CUI_PAGINATION_NAV_CHEVRONS;
    options.available_width = 195;
    assert(!ellipsis_frame(&f, 20, options, false, false).control && f.value == 3 && !f.calls);
    assert(!pagination_node(&f, CUI_ROLE_BUTTON, "Première"));
    assert(!pagination_node(&f, CUI_ROLE_BUTTON, "Dernière"));
    for (unsigned part = 2; part <= 3; ++part) {
        const char *label = part == 2 ? "Précédente" : "Suivante";
        const cui_semantic *button = pagination_node(&f, CUI_ROLE_BUTTON, label);
        assert(button && button->id == cui_numeric_part_id(2, part));
        cui_rect bounds = cui_bounds(f.ui, button->id);
        cui_rect icon = {0}, title = {0};
        for (size_t i = 0; i < f.draw.count; ++i) {
            const cui_draw_command *c = &f.draw.commands[i];
            cui_rect box = c->bounds;
            if (box.x < bounds.x || box.x + box.w > bounds.x + bounds.w || box.y < bounds.y ||
                box.y + box.h > bounds.y + bounds.h)
                continue;
            if (c->type == CUI_DRAW_IMAGE)
                icon = box;
            if (c->type == CUI_DRAW_TEXT && c->text.length == strlen(label) &&
                !memcmp(c->text.data, label, c->text.length))
                title = box;
        }
        assert(icon.w > 0 && title.w > 0);
        assert(part == 2 ? icon.x + icon.w < title.x : title.x + title.w < icon.x);
        assert(fabsf(icon.y + icon.h / 2 - (bounds.y + bounds.h / 2)) <= 1);
        assert(fabsf(title.y + title.h / 2 - (bounds.y + bounds.h / 2)) <= 1);
    }
    click(&f, 3, .85f); /* Icon and label have one full-width hit target. */
    assert(ellipsis_frame(&f, 20, options, false, false).change_count == 1 && f.value == 4);
    const cui_semantic *gap = pagination_node(&f, CUI_ROLE_BUTTON, "Pages suivantes");
    assert(gap);
    pagination_action(&f, gap->id);
    assert(!ellipsis_frame(&f, 20, options, false, false).control);
    assert(pagination_node(&f, CUI_ROLE_MENU, "Pages suivantes"));
    pagination_key(&f, CUI_KEY_ESCAPE);
    ellipsis_frame(&f, 20, options, false, false);
    assert(!pagination_node(&f, CUI_ROLE_MENU, "Pages suivantes"));

    options.icon_only = true;
    assert(!ellipsis_frame(&f, 20, options, false, false).control);
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Précédente"));
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Suivante"));
    for (size_t i = 0; i < f.draw.count; ++i)
        assert(f.draw.commands[i].type != CUI_DRAW_TEXT);
    cui_destroy(f.ui);
}
static void pagination_ellipsis_menu(void) {
    fixture f = make();
    f.value = 205;
    cui_pagination_options options = {.design_size = CUI_DESIGN_BASE,
                                      .available_width = 352,
                                      .visible_pages = 5,
                                      .ellipsis_viewport = {0, 0, 400, 420}};
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    const cui_semantic *gap = pagination_node(&f, CUI_ROLE_BUTTON, "Later pages");
    assert(gap && !gap->details.expanded && gap->details.controls);
    cui_id later = gap->id;
    key(&f, later, CUI_KEY_DOWN);
    assert(!ellipsis_frame(&f, 400, options, false, false).control && !f.calls);
    const cui_semantic *panel = pagination_node(&f, CUI_ROLE_MENU, "Later pages");
    assert(panel && panel->details.scrollable && panel->details.labelled_by == later);
    cui_rect panel_bounds = cui_bounds(f.ui, panel->id);
    assert(panel_bounds.w > 0 && panel_bounds.h > 0 && panel_bounds.h <= 240);
    const cui_semantic *row = pagination_node(&f, CUI_ROLE_MENUITEM, "207");
    assert(row && cui_focused(f.ui) == row->id);
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Later pages")->details.expanded);

    cui_id popup_id = panel->id;
    cui_rect initial_trigger = cui_bounds(f.ui, later);
    options.available_width = 164;
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    cui_rect moved_trigger = cui_bounds(f.ui, later);
    assert(initial_trigger.x != moved_trigger.x || initial_trigger.y != moved_trigger.y);
    assert(cui_needs_redraw(f.ui));
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    cui_rect moved_popup = cui_bounds(f.ui, popup_id);
    assert(panel_bounds.x != moved_popup.x || panel_bounds.y != moved_popup.y);

    pagination_key(&f, CUI_KEY_END);
    assert(!ellipsis_frame(&f, 400, options, false, false).control && !f.calls);
    row = pagination_node(&f, CUI_ROLE_MENUITEM, "306");
    assert(row && cui_focused(f.ui) == row->id);
    assert(!pagination_node(&f, CUI_ROLE_MENUITEM, "307"));
    pagination_key(&f, CUI_KEY_ENTER);
    cui_numeric_selection_event_result result = ellipsis_frame(&f, 400, options, false, false);
    assert(result.control && result.change_count == 1 && f.value == 306 && f.calls == 1);
    assert(f.page_event.before == 205 && f.page_event.after == 306);
    assert(!pagination_node(&f, CUI_ROLE_MENU, "Later pages") && cui_focused(f.ui) == later);

    cui_id earlier = pagination_node(&f, CUI_ROLE_BUTTON, "Earlier pages")->id;
    pagination_action(&f, earlier);
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    row = pagination_node(&f, CUI_ROLE_MENUITEM, "205");
    assert(row); /* The earlier gap exposes the 100 pages nearest the window. */
    pagination_action(&f, row->id);
    f.value = 305; /* External owner replacement retires previously offered rows. */
    assert(!ellipsis_frame(&f, 400, options, false, false).control && f.value == 305 &&
           f.calls == 1 && !pagination_node(&f, CUI_ROLE_MENU, "Earlier pages"));

    pagination_action(&f, earlier);
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    assert(pagination_node(&f, CUI_ROLE_MENU, "Earlier pages"));
    pagination_key(&f, CUI_KEY_ESCAPE);
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    assert(!pagination_node(&f, CUI_ROLE_MENU, "Earlier pages") && cui_focused(f.ui) == earlier);

    pagination_action(&f, earlier);
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    row = pagination_node(&f, CUI_ROLE_MENUITEM, "204");
    assert(row);
    pagination_action(&f, row->id);
    options.disabled = true;
    assert(!ellipsis_frame(&f, 400, options, false, false).control);
    assert(f.value == 305 && f.calls == 1 && !pagination_node(&f, CUI_ROLE_MENU, "Earlier pages"));
    assert(pagination_node(&f, CUI_ROLE_BUTTON, "Earlier pages")->disabled);
    options.disabled = false;
    ellipsis_frame(&f, 400, options, false, false);
    pagination_action(&f, earlier);
    ellipsis_frame(&f, 400, options, false, false);
    assert(pagination_node(&f, CUI_ROLE_MENU, "Earlier pages"));
    ellipsis_frame(&f, 400, options, false, true);
    ellipsis_frame(&f, 400, options, false, false);
    assert(!pagination_node(&f, CUI_ROLE_MENU, "Earlier pages"));
    pagination_action(&f, earlier);
    ellipsis_frame(&f, 400, options, false, false);
    row = pagination_node(&f, CUI_ROLE_MENUITEM, "204");
    assert(row);
    pagination_action(&f, row->id);
    assert(!ellipsis_frame(&f, 400, options, true, false).control && f.value == 305 &&
           f.calls == 1);
    ellipsis_frame(&f, 400, options, false, false);
    assert(!pagination_node(&f, CUI_ROLE_MENU, "Earlier pages"));

    f.value = 6;
    ellipsis_frame(&f, 20, options, false, false);
    pagination_action(&f, earlier);
    ellipsis_frame(&f, 20, options, false, false);
    row = pagination_node(&f, CUI_ROLE_MENUITEM, "2");
    assert(row);
    pagination_action(&f, row->id);
    result = ellipsis_frame(&f, 20, options, false, false);
    assert(result.control && result.change_count == 1 && f.value == 2 && f.calls == 2);
    assert(!pagination_node(&f, CUI_ROLE_BUTTON, "Earlier pages"));
    assert(cui_focused(f.ui) == cui_numeric_part_id(2, 2));
    cui_destroy(f.ui);
}
static void compare(fixture *a, fixture *b) {
    assert(a->value == b->value && cui_focused(a->ui) == cui_focused(b->ui));
    cui_rect x = cui_bounds(a->ui, 2), y = cui_bounds(b->ui, 2);
    assert(x.x == y.x && x.y == y.y && x.w == y.w && x.h == y.h);
    size_t an, bn;
    const cui_semantic *as = cui_semantics(a->ui, &an), *bs = cui_semantics(b->ui, &bn);
    assert(an == bn && a->draw.count == b->draw.count);
    for (size_t i = 0; i < an; ++i) {
        assert(as[i].id == bs[i].id && as[i].role == bs[i].role && as[i].value == bs[i].value &&
               as[i].disabled == bs[i].disabled &&
               as[i].details.selected == bs[i].details.selected &&
               as[i].details.readonly == bs[i].details.readonly);
        cui_rect ab = cui_bounds(a->ui, as[i].id), bb = cui_bounds(b->ui, bs[i].id);
        assert(ab.x == bb.x && ab.y == bb.y && ab.w == bb.w && ab.h == bb.h);
    }
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *ac = &a->draw.commands[i], *bc = &b->draw.commands[i];
        assert(ac->type == bc->type && ac->bounds.x == bc->bounds.x &&
               ac->bounds.y == bc->bounds.y && ac->bounds.w == bc->bounds.w &&
               ac->bounds.h == bc->bounds.h && ac->color.r == bc->color.r &&
               ac->color.g == bc->color.g && ac->color.b == bc->color.b &&
               ac->color.a == bc->color.a && ac->radius == bc->radius && ac->stroke == bc->stroke &&
               ac->font_size == bc->font_size && ac->text.length == bc->text.length);
        if (ac->text.length)
            assert(!memcmp(ac->text.data, bc->text.data, ac->text.length));
        assert(ac->bitmap.bitmap.width == bc->bitmap.bitmap.width &&
               ac->bitmap.bitmap.height == bc->bitmap.bitmap.height &&
               ac->bitmap.source.x == bc->bitmap.source.x &&
               ac->bitmap.source.y == bc->bitmap.source.y &&
               ac->bitmap.source.w == bc->bitmap.source.w &&
               ac->bitmap.source.h == bc->bitmap.source.h &&
               ac->bitmap.opacity == bc->bitmap.opacity &&
               ac->bitmap.custom_tint == bc->bitmap.custom_tint);
        if (ac->bitmap.bitmap.pixels && bc->bitmap.bitmap.pixels)
            for (unsigned row = 0; row < ac->bitmap.bitmap.height; ++row)
                assert(!memcmp(ac->bitmap.bitmap.pixels + row * ac->bitmap.bitmap.stride,
                               bc->bitmap.bitmap.pixels + row * bc->bitmap.bitmap.stride,
                               ac->bitmap.bitmap.width * 4u));
    }
}
static void pagination(bool styled) {
    fixture f = make();
    f.value = 50;
    assert(!page_frame(&f, styled, false, 100, false, false, 0, true).control);
    key(&f, cui_numeric_part_id(2, 3), CUI_KEY_ENTER);
    assert(page_frame(&f, styled, false, 100, false, false, 0, true).change_count == 1 &&
           f.value == 51);
    assert(f.page_event.before == 50 && f.page_event.after == 51 && f.page_event.total == 100);
    activate(&f, 4);
    activate(&f, 2);
    assert(page_frame(&f, styled, false, 100, false, false, 0, true).change_count == 1 &&
           f.value == 99);
    activate(&f, 2);
    activate(&f, 3);
    assert(!page_frame(&f, styled, false, 100, false, false, 0, true).control && f.value == 99);
    assert(f.calls == 2);
    activate(&f, 4);
    assert(!page_frame(&f, styled, false, 100, false, false, 0, false).change_count &&
           f.value == 100 && f.calls == 2);
    f.value = SIZE_MAX;
    assert(page_frame(&f, styled, false, SIZE_MAX, true, false, 0, true).change_count == 0);
    activate(&f, 2);
    assert(page_frame(&f, styled, false, SIZE_MAX, true, false, 0, true).change_count == 1);
    assert(f.page_event.before == SIZE_MAX && f.page_event.after == SIZE_MAX - 1 &&
           f.page_event.total == SIZE_MAX);
    assert(page_frame(&f, styled, false, 0, true, false, 0, true).change_count == 1 &&
           f.value == 0);
    assert(f.teardown_requested); /* Actual destruction deferred until outer calls completed. */
    cui_destroy(f.ui);
    if (styled) {
        f = make();
        f.value = 50;
        page_frame(&f, true, false, 100, false, false, 0, true);
        click(&f, 4, .5f);
        click(&f, 2, .5f);
        assert(page_frame(&f, true, false, 100, false, false, 0, true).change_count == 1 &&
               f.value == 99);
        cui_destroy(f.ui);
    }
    for (unsigned passive = 0; passive < 3; ++passive)
        for (unsigned disabled = 0; disabled < 2; ++disabled) {
            fixture a = make(), b = make();
            a.value = b.value = SIZE_MAX;
            cui_numeric_selection_event_result ar =
                page_frame(&a, styled, true, 100, false, disabled != 0, passive, false);
            cui_numeric_selection_event_result br =
                page_frame(&b, styled, false, 100, false, disabled != 0, passive, true);
            compare(&a, &b);
            assert(a.value == 100 && ar.control == br.control);
            assert(br.change_count == ((!passive && !disabled) ? 1u : 0u));
            cui_destroy(a.ui);
            cui_destroy(b.ui);
        }
}
static const cui_step steps[] = {
    {.title = {"Start", 5}, .status = CUI_STEP_ACTIVE},
    {.title = {"Payment", 7}, .description = {"Details", 7}, .status = CUI_STEP_ERROR},
    {.title = {"Finish", 6}, .status = CUI_STEP_COMPLETE, .disabled = true}};
static void step_changed(void *user, const cui_stepper_change_event *e) {
    fixture *f = user;
    metadata(f, &e->event);
    assert(e->after == f->value);
    committed_semantics(f, (float)e->after);
    f->step_event = *e;
    f->calls++;
    f->teardown_requested = true;
}
static cui_numeric_selection_event_result step_frame(fixture *f, bool styled, bool legacy,
                                                     const cui_step *items, size_t count,
                                                     bool disabled, unsigned passive, bool handler,
                                                     bool vertical) {
    assert(cui_begin_frame(f->ui, 1400, 800, .016f));
    cui_begin_box(
        f->ui, 900,
        (cui_box){
            .width = 1300, .height = 700, .inert = passive == 1, .layout_only = passive == 2});
    cui_theme t = cui_theme_light();
    cui_numeric_selection_event_result r = {0};
    if (legacy)
        r.control = styled ? cui_stepper(f->ui, 2, &t, items, count, &f->value, vertical, disabled)
                           : cui_base_stepper(f->ui, 2, items, count, &f->value, disabled);
    else {
        cui_stepper_event_handler h = {handler ? step_changed : NULL, f};
        r = styled
                ? cui_stepper_events(f->ui, 2, &t, items, count, &f->value, vertical, disabled, h)
                : cui_base_stepper_events(f->ui, 2, items, count, &f->value, disabled, h);
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void stepper(bool styled) {
    fixture f = make();
    step_frame(&f, styled, false, steps, 3, false, 0, true, false);
    key(&f, cui_numeric_part_id(2, 2), CUI_KEY_ENTER);
    assert(step_frame(&f, styled, false, steps, 3, false, 0, true, false).change_count == 1 &&
           f.value == 1);
    assert(f.step_event.before == 0 && f.step_event.after == 1 && f.step_event.count == 3);
    activate(&f, 1);
    activate(&f, 2);
    assert(!step_frame(&f, styled, false, steps, 3, false, 0, true, false).control &&
           f.value == 1 && f.calls == 1);
    assert(!cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                              .accessibility_target = cui_numeric_part_id(2, 3),
                                              .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    assert(cui_last_error(f.ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(f.ui);
    assert(!step_frame(&f, styled, false, steps, 3, false, 0, true, false).change_count &&
           f.value == 1);
    activate(&f, 1);
    assert(!step_frame(&f, styled, false, steps, 3, false, 0, false, false).change_count &&
           f.value == 0 && f.calls == 1);
    f.value = SIZE_MAX;
    assert(step_frame(&f, styled, false, steps, 3, false, 0, true, false).change_count == 1 &&
           f.value == 2);
    assert(f.step_event.before == SIZE_MAX && f.step_event.after == 2);
    assert(step_frame(&f, styled, false, NULL, 0, false, 0, true, true).change_count == 1 &&
           f.value == 0);
    f.value = 9;
    assert(!step_frame(&f, styled, false, NULL, 3, false, 0, true, false).control && f.value == 9);
    assert(!step_frame(&f, styled, false, steps, 101, false, 0, true, false).control &&
           f.value == 9);
    assert(steps[0].status == CUI_STEP_ACTIVE && steps[1].status == CUI_STEP_ERROR &&
           steps[2].status == CUI_STEP_COMPLETE);
    assert(f.teardown_requested);
    cui_destroy(f.ui);
    if (styled) {
        f = make();
        step_frame(&f, true, false, steps, 3, false, 0, true, false);
        click(&f, 2, .5f);
        assert(step_frame(&f, true, false, steps, 3, false, 0, true, false).change_count == 1 &&
               f.value == 1);
        cui_destroy(f.ui);
    }
    for (unsigned passive = 0; passive < 3; ++passive)
        for (unsigned disabled = 0; disabled < 2; ++disabled)
            for (unsigned vertical = 0; vertical < 2; ++vertical) {
                fixture a = make(), b = make();
                a.value = b.value = SIZE_MAX;
                cui_numeric_selection_event_result ar = step_frame(
                    &a, styled, true, steps, 3, disabled != 0, passive, false, vertical != 0);
                cui_numeric_selection_event_result br = step_frame(
                    &b, styled, false, steps, 3, disabled != 0, passive, true, vertical != 0);
                compare(&a, &b);
                assert(a.value == 2 && ar.control == br.control);
                assert(br.change_count == ((!passive && !disabled) ? 1u : 0u));
                cui_destroy(a.ui);
                cui_destroy(b.ui);
            }
}
static void rating_changed(void *user, const cui_rating_change_event *e) {
    fixture *f = user;
    metadata(f, &e->event);
    assert(e->after == f->rating && isfinite(e->after));
    committed_semantics(f, e->after);
    assert(e->before_finite == (isfinite(e->before) != 0));
    f->rating_event = *e;
    f->calls++;
    f->teardown_requested = true;
}
static cui_numeric_selection_event_result rating_frame(fixture *f, bool styled, bool legacy,
                                                       cui_rating_options o, unsigned passive,
                                                       bool handler) {
    assert(cui_begin_frame(f->ui, 1400, 800, .016f));
    cui_begin_box(
        f->ui, 900,
        (cui_box){
            .width = 1300, .height = 700, .inert = passive == 1, .layout_only = passive == 2});
    cui_theme t = cui_theme_light();
    cui_numeric_selection_event_result r = {0};
    if (legacy)
        r.control = styled ? cui_rating(f->ui, 2, &t, cui_string("Stars"), &f->rating, o)
                           : cui_base_rating(f->ui, 2, cui_string("Stars"), &f->rating, o);
    else {
        cui_rating_event_handler h = {handler ? rating_changed : NULL, f};
        r = styled ? cui_rating_events(f->ui, 2, &t, cui_string("Stars"), &f->rating, o, h)
                   : cui_base_rating_events(f->ui, 2, cui_string("Stars"), &f->rating, o, h);
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void rating(bool styled) {
    fixture f = make();
    f.rating = 2;
    cui_rating_options o = {.half_steps = true, .clearable = true};
    rating_frame(&f, styled, false, o, 0, true);
    key(&f, 2, CUI_KEY_RIGHT);
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 && f.rating == 2.5f);
    assert(f.rating_event.before == 2 && f.rating_event.after == 2.5f &&
           f.rating_event.maximum == 5);
    key(&f, 2, CUI_KEY_LEFT);
    key(&f, 2, CUI_KEY_RIGHT);
    assert(!rating_frame(&f, styled, false, o, 0, true).control && f.calls == 1);
    activate(&f, 13);
    activate(&f, 11);
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 && f.rating == 2);
    activate(&f, 11);
    assert(!rating_frame(&f, styled, false, o, 0, true).change_count);
    key(&f, 2, CUI_KEY_HOME);
    o.clearable = false;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 && f.rating == 0);
    key(&f, 2, CUI_KEY_END);
    assert(!rating_frame(&f, styled, false, o, 0, false).change_count && f.rating == 5);
    f.rating = NAN;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 && f.rating == 0);
    assert(isnan(f.rating_event.before) && !f.rating_event.before_finite);
    f.rating = INFINITY;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1);
    assert(isinf(f.rating_event.before) && !f.rating_event.before_finite);
    f.rating = -INFINITY;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 &&
           signbit(f.rating_event.before));
    f.rating = -0.0f;
    assert(!rating_frame(&f, styled, false, o, 0, true).change_count);
    f.rating = FLT_TRUE_MIN;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 &&
           f.rating_event.before == FLT_TRUE_MIN);
    f.rating = 2.26f;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 && f.rating == 2.5f);
    o.half_steps = false;
    f.rating = 2.26f;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 && f.rating == 2);
    o.maximum = 101;
    f.rating = FLT_MAX;
    assert(rating_frame(&f, styled, false, o, 0, true).change_count == 1 && f.rating == 100 &&
           f.rating_event.maximum == 100);
    assert(f.teardown_requested);
    cui_destroy(f.ui);
    if (styled) {
        f = make();
        o = (cui_rating_options){.half_steps = true, .clearable = true};
        rating_frame(&f, true, false, o, 0, true);
        click(&f, 13, .25f);
        assert(rating_frame(&f, true, false, o, 0, true).change_count == 1 && f.rating == 3.5f);
        click(&f, 1, .5f);
        assert(rating_frame(&f, true, false, o, 0, true).change_count == 1 && f.rating == 0);
        cui_destroy(f.ui);
    }
    for (unsigned passive = 0; passive < 3; ++passive)
        for (unsigned restricted = 0; restricted < 3; ++restricted)
            for (unsigned nonfinite = 0; nonfinite < 2; ++nonfinite) {
                fixture a = make(), b = make();
                a.rating = b.rating = nonfinite ? NAN : 2.26f;
                o = (cui_rating_options){.half_steps = true,
                                         .clearable = true,
                                         .disabled = restricted == 1,
                                         .readonly = restricted == 2};
                cui_numeric_selection_event_result ar =
                    rating_frame(&a, styled, true, o, passive, false);
                cui_numeric_selection_event_result br =
                    rating_frame(&b, styled, false, o, passive, true);
                compare(&a, &b);
                assert(a.rating == b.rating && ar.control == br.control);
                assert(br.change_count == ((!passive && !restricted) ? 1u : 0u));
                cui_destroy(a.ui);
                cui_destroy(b.ui);
            }
}
static void paired_input(bool styled) {
    fixture a = make(), b = make();
    a.value = b.value = 50;
    page_frame(&a, styled, true, 100, false, false, 0, false);
    page_frame(&b, styled, false, 100, false, false, 0, true);
    activate(&a, 4);
    activate(&a, 2);
    activate(&b, 4);
    activate(&b, 2);
    cui_numeric_selection_event_result ar =
        page_frame(&a, styled, true, 100, false, false, 0, false);
    cui_numeric_selection_event_result br =
        page_frame(&b, styled, false, 100, false, false, 0, true);
    compare(&a, &b);
    assert(ar.control == br.control && br.change_count == 1 && b.value == 99);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
    a = make();
    b = make();
    step_frame(&a, styled, true, steps, 3, false, 0, false, false);
    step_frame(&b, styled, false, steps, 3, false, 0, true, false);
    activate(&a, 2);
    activate(&b, 2);
    ar = step_frame(&a, styled, true, steps, 3, false, 0, false, false);
    br = step_frame(&b, styled, false, steps, 3, false, 0, true, false);
    compare(&a, &b);
    assert(ar.control == br.control && br.change_count == 1 && b.value == 1);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
    a = make();
    b = make();
    a.rating = b.rating = 2;
    cui_rating_options o = {.half_steps = true};
    rating_frame(&a, styled, true, o, 0, false);
    rating_frame(&b, styled, false, o, 0, true);
    key(&a, 2, CUI_KEY_RIGHT);
    key(&b, 2, CUI_KEY_RIGHT);
    ar = rating_frame(&a, styled, true, o, 0, false);
    br = rating_frame(&b, styled, false, o, 0, true);
    compare(&a, &b);
    assert(ar.control == br.control && br.change_count == 1 && a.rating == b.rating &&
           b.rating == 2.5f);
    if (styled) {
        /* A pointer queued against editable semantics is still gated by the next call's readonly
         * option. */
        click(&a, 13, .25f);
        click(&b, 13, .25f);
        o.readonly = true;
        ar = rating_frame(&a, true, true, o, 0, false);
        br = rating_frame(&b, true, false, o, 0, true);
        compare(&a, &b);
        assert(!ar.control && !br.control && !br.change_count && a.rating == 2.5f &&
               b.rating == 2.5f);
    }
    cui_destroy(a.ui);
    cui_destroy(b.ui);
}
int main(void) {
    sized_pagination();
    pagination_modes();
    pagination_localized_navigation();
    pagination_ellipsis_menu();
    for (unsigned styled = 0; styled < 2; ++styled) {
        pagination(styled != 0);
        stepper(styled != 0);
        rating(styled != 0);
        paired_input(styled != 0);
    }
    return 0;
}
