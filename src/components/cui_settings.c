/* SPDX-License-Identifier: MIT */
#include "cui/cui_settings.h"
#include "cui/cui_icon.h"
#include <math.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_id id;
    const cui_theme *theme;
    const cui_setting_page *pages;
    size_t count;
    cui_settings_state *state;
    cui_settings_options options;
    cui_settings_result *result;
    cui_str query;
    float content_width, height;
} settings_frame;
typedef struct {
    settings_frame *frame;
    size_t page, group;
} settings_navigation;
static cui_id part(settings_frame *f, size_t p, size_t g, size_t i, unsigned n) {
    return cui_settings_part_id(f->id, p, g, i, n);
}
static float density(settings_frame *f) {
    return f->theme->font_size / 16;
}
static cui_control_size control_size(cui_settings_size size) {
    switch (size) {
    case CUI_SETTINGS_XSMALL:
        return CUI_CONTROL_XSMALL;
    case CUI_SETTINGS_SMALL:
        return CUI_CONTROL_SMALL;
    case CUI_SETTINGS_LARGE:
        return CUI_CONTROL_LARGE;
    default:
        return CUI_CONTROL_MEDIUM;
    }
}
static void label(settings_frame *f, cui_str text, float font, cui_color color) {
    if (text.length)
        cui_text_with_line_height(f->ui, text, font, color, 20 * density(f));
}
static void navigate(void *user) {
    settings_navigation *nav = user;
    cui_settings_state *s = nav->frame->state;
    if (s->selected_page != nav->page || s->selected_group != nav->group)
        nav->frame->result->selection_changed = true;
    s->selected_page = nav->page;
    s->selected_group = nav->group;
    s->pending_group_scroll = nav->group != CUI_SETTINGS_NO_GROUP;
    cui_invalidate(nav->frame->ui);
}
static void sidebar(cui_context *ui, void *user) {
    settings_frame *f = user;
    cui_begin_box(ui, part(f, 0, 0, 0, 1),
                  (cui_box){.width = -1,
                            .height = f->height,
                            .padding = 8 * density(f),
                            .gap = 8 * density(f),
                            .clip = true});
    if (f->state->search) {
        cui_begin_box(
            ui, part(f, 0, 0, 0, 2),
            (cui_box){
                .width = -1, .row = true, .gap = 4 * density(f), .align_y = CUI_ALIGN_CENTER});
        cui_icon_named(ui, part(f, 0, 0, 0, 3), f->theme, cui_string("search"),
                       (cui_icon_options){.size = CUI_ICON_SMALL});
        cui_input_ex(
            ui, part(f, 0, 0, 0, 4), f->theme, f->options.search_placeholder, f->state->search,
            &f->state->search_input,
            (cui_input_options){.width = -1, .placeholder = f->options.search_placeholder});
        cui_end_box(ui);
        /* Search is reduced before navigation/content, so same-frame text filters both. */
        f->query = cui_edit_get_text(f->state->search);
        cui_settings_result filter = cui_settings_filter(f->pages, f->count, f->state, f->options);
        f->result->selection_changed |= filter.selection_changed;
        f->result->visible_pages = filter.visible_pages;
        f->result->visible_groups = filter.visible_groups;
        f->result->visible_items = filter.visible_items;
    }
    for (size_t p = 0; p < f->count; p++) {
        const cui_setting_page *page = &f->pages[p];
        size_t visible = 0;
        for (size_t g = 0; g < page->count; g++)
            visible += cui_setting_group_matches(&page->groups[g], f->query);
        if (!visible)
            continue;
        cui_sidebar_menu_item children[256];
        settings_navigation nav[256];
        size_t child_count = 0;
        if (visible > 1)
            for (size_t g = 0; g < page->count; g++) {
                if (!page->groups[g].title.length ||
                    !cui_setting_group_matches(&page->groups[g], f->query))
                    continue;
                nav[child_count] = (settings_navigation){f, p, g};
                children[child_count] = (cui_sidebar_menu_item){
                    .id = part(f, p, g, 0, 6),
                    .label = page->groups[g].title,
                    .active = f->state->selected_page == p && f->state->selected_group == g,
                    .on_activate = navigate,
                    .user = &nav[child_count]};
                child_count++;
            }
        settings_navigation page_nav = {f, p, CUI_SETTINGS_NO_GROUP};
        cui_sidebar_menu_item item = {
            .id = part(f, p, 0, 0, 5),
            .label = page->title,
            .icon = page->icon,
            .active = f->state->selected_page == p &&
                      (f->state->selected_group == CUI_SETTINGS_NO_GROUP || visible == 1),
            .default_open = page->default_open,
            .click_to_open = true,
            .state = &f->state->pages[p].menu,
            .children = children,
            .child_count = child_count,
            .on_activate = navigate,
            .user = &page_nav};
        cui_sidebar_menu(ui, part(f, p, 0, 0, 7), f->theme, &item, 1, false);
    }
    cui_end_box(ui);
}
static bool field(settings_frame *f, const cui_setting_item *item, cui_id id,
                  cui_settings_render_options render) {
    if (item->render) {
        item->render(f->ui, id, f->theme, render, item->user);
        return false;
    }
    bool changed = false;
    cui_edit_result error = CUI_EDIT_OK;
    cui_control_size size = control_size(render.size);
    switch (item->kind) {
    case CUI_SETTING_SWITCH:
        if (item->boolean)
            changed =
                cui_switch_ex(f->ui, id, f->theme, (cui_str){0}, item->boolean, item->switch_state,
                              (cui_switch_options){.size = size,
                                                   .disabled = render.disabled,
                                                   .accessibility_label = item->title,
                                                   .now_seconds = cui_frame_time(f->ui),
                                                   .reduced_motion = cui_reduced_motion(f->ui)});
        break;
    case CUI_SETTING_CHECKBOX:
        if (item->boolean) {
            cui_check_state value = *item->boolean ? CUI_CHECK_CHECKED : CUI_CHECK_UNCHECKED;
            changed = cui_checkbox_ex(f->ui, id, f->theme, (cui_str){0}, &value,
                                      (cui_checkbox_options){.size = size,
                                                             .disabled = render.disabled,
                                                             .accessibility_label = item->title});
            if (changed)
                *item->boolean = value == CUI_CHECK_CHECKED;
        }
        break;
    case CUI_SETTING_INPUT:
        if (item->edit && item->input_state) {
            cui_input_result result =
                cui_input_ex(f->ui, id, f->theme, item->title, item->edit, item->input_state,
                             (cui_input_options){.width = render.vertical ? -1 : 256 * density(f),
                                                 .disabled = render.disabled});
            changed = result.changed;
            error = result.error;
        } else
            f->result->invalid = true;
        break;
    case CUI_SETTING_NUMBER:
        if (item->number && item->number_state) {
            cui_number_options options = item->number_options;
            options.disabled = render.disabled;
            if (options.step == 0)
                options.step = 1;
            options.width = render.vertical ? -1 : 256 * density(f);
            double before = *item->number;
            cui_number_result result = cui_number_input(f->ui, id, f->theme, item->title,
                                                        item->number_state, item->number, options);
            error = result.error;
            if (options.has_minimum && *item->number < options.minimum)
                *item->number = options.minimum;
            if (options.has_maximum && *item->number > options.maximum)
                *item->number = options.maximum;
            item->number_state->last_value = *item->number;
            changed = before != *item->number;
        }
        break;
    case CUI_SETTING_DROPDOWN:
        if (item->choice && item->popup) {
            cui_list_size list_size = render.size == CUI_SETTINGS_XSMALL  ? CUI_LIST_XSMALL
                                      : render.size == CUI_SETTINGS_SMALL ? CUI_LIST_SMALL
                                      : render.size == CUI_SETTINGS_LARGE ? CUI_LIST_LARGE
                                                                          : CUI_LIST_MEDIUM;
            changed = cui_select(
                f->ui, id, f->theme, item->title, (cui_str){0}, item->choices, item->choice_count,
                item->choice, item->popup,
                (cui_choice_popup_options){.disabled = render.disabled,
                                           .width = render.vertical ? -1 : 256 * density(f),
                                           .size = list_size,
                                           .align_end = true,
                                           .viewport = {0, 0, 100000, 100000}});
        }
        break;
    default:
        break;
    }
    if (f->result->edit_error == CUI_EDIT_OK && error != CUI_EDIT_OK)
        f->result->edit_error = error;
    if (changed && item->on_change)
        item->on_change(item->user);
    return changed;
}
static void render_group(settings_frame *f, size_t p, size_t g, bool stacked) {
    const cui_setting_group *group = &f->pages[p].groups[g];
    cui_settings_variant variant = group->custom_variant ? group->variant : f->options.variant;
    float d = density(f), font = 14 * d;
    cui_settings_render_options render = {
        .page = p, .group = g, .size = f->options.size, .variant = variant, .vertical = stacked};
    cui_id id = part(f, p, g, 0, 20);
    cui_begin_box(f->ui, id, (cui_box){.width = -1});
    cui_begin_box(f->ui, part(f, p, g, 0, 44), (cui_box){.height = 16 * d});
    cui_end_box(f->ui);
    cui_begin_box(f->ui, part(f, p, g, 0, 43),
                  (cui_box){.width = -1, .gap = (variant == CUI_SETTINGS_NORMAL ? 16 : 12) * d});
    if (group->title.length) {
        cui_begin_box(f->ui, part(f, p, g, 0, 21), (cui_box){.width = -1, .gap = 4 * d});
        label(f, group->title, f->theme->font_size, f->theme->muted);
        label(f, group->description, font, f->theme->muted);
        cui_end_box(f->ui);
    }
    cui_begin_box(f->ui, part(f, p, g, 0, 29), (cui_box){.width = -1, .gap = 8 * d});
    cui_begin_box(
        f->ui, part(f, p, g, 0, 22),
        (cui_box){.width = -1,
                  .gap = 16 * d,
                  .padding = variant == CUI_SETTINGS_NORMAL ? 0 : 16 * d,
                  .radius = f->theme->radius,
                  .border = variant == CUI_SETTINGS_OUTLINE ? f->theme->border : (cui_color){0},
                  .border_width = variant == CUI_SETTINGS_OUTLINE ? 1 : 0,
                  .background =
                      variant == CUI_SETTINGS_FILL ? f->theme->group_box : (cui_color){0}});
    for (size_t i = 0; i < group->count; i++) {
        const cui_setting_item *item = &group->items[i];
        if (!cui_setting_item_matches(item, f->query))
            continue;
        render.item = i;
        render.disabled = item->disabled;
        render.vertical = stacked || item->vertical;
        cui_begin_box(f->ui, part(f, p, g, i, 23),
                      (cui_box){.width = -1,
                                .row = !render.vertical && !item->custom_item,
                                .gap = 12 * d,
                                .align_y = CUI_ALIGN_CENTER,
                                .custom_opacity = item->disabled,
                                .opacity = .5f});
        if (item->custom_item) {
            if (item->render)
                item->render(f->ui, part(f, p, g, i, 24), f->theme, render, item->user);
        } else {
            cui_begin_box(
                f->ui, part(f, p, g, i, 25),
                (cui_box){
                    .width = -1,
                    .max_width =
                        render.vertical
                            ? 0
                            : fmaxf(0, fmaxf(0, f->content_width - 32 * d -
                                                    (variant == CUI_SETTINGS_NORMAL ? 0 : 32 * d)) *
                                           .6f),
                    .gap = 0});
            label(f, item->title, font, f->theme->group_box_foreground);
            if (item->description.length)
                cui_text_with_line_height(f->ui, item->description, font, f->theme->muted,
                                          font * 1.618033988749895f);
            cui_end_box(f->ui);
            cui_begin_box(f->ui, part(f, p, g, i, 26),
                          (cui_box){.width = render.vertical ? -1 : 0});
            field(f, item, part(f, p, g, i, 24), render);
            cui_end_box(f->ui);
        }
        cui_end_box(f->ui);
    }
    cui_end_box(f->ui);
    if (group->footer) {
        cui_begin_box(f->ui, part(f, p, g, 0, 27), (cui_box){.width = -1, .padding = 0});
        cui_theme footer_theme = *f->theme;
        footer_theme.text = f->theme->muted;
        footer_theme.font_size = font;
        group->footer(f->ui, part(f, p, g, 0, 28), &footer_theme, render, group->footer_user);
        cui_end_box(f->ui);
    }
    cui_end_box(f->ui);
    cui_end_box(f->ui);
    cui_begin_box(f->ui, part(f, p, g, 0, 45), (cui_box){.height = 16 * d});
    cui_end_box(f->ui);
    cui_end_box(f->ui);
}
static void content(cui_context *ui, void *user) {
    settings_frame *f = user;
    f->content_width = fmaxf(1, f->state->resize.slots[1].size);
    size_t p = f->state->selected_page;
    if (p >= f->count)
        return;
    const cui_setting_page *page = &f->pages[p];
    bool visible = false, dirty = false;
    for (size_t g = 0; g < page->count; g++) {
        visible |= cui_setting_group_matches(&page->groups[g], f->query);
        for (size_t i = 0; i < page->groups[g].count; i++) {
            const cui_setting_item *item = &page->groups[g].items[i];
            dirty |= cui_setting_item_matches(item, f->query) && cui_setting_item_dirty(item);
        }
    }
    if (!visible)
        return;
    float d = density(f);
    cui_begin_box(ui, part(f, p, 0, 0, 30),
                  (cui_box){.width = -1, .height = f->height, .clip = true});
    cui_begin_box(ui, part(f, p, 0, 0, 31),
                  (cui_box){.width = -1,
                            .padding = 16 * d,
                            .gap = 12 * d,
                            .border = f->theme->border,
                            .border_width = 1,
                            .custom_border_edges = true,
                            .border_edges = {.bottom = 1}});
    cui_begin_box(ui, part(f, p, 0, 0, 32),
                  (cui_box){.width = -1, .row = true, .gap = 4 * d, .align_y = CUI_ALIGN_CENTER});
    label(f, page->title, f->theme->font_size, f->theme->text);
    cui_settings_render_options render = {
        .page = p, .size = f->options.size, .variant = f->options.variant};
    if (page->title_suffix)
        page->title_suffix(ui, part(f, p, 0, 0, 33), f->theme, render, page->suffix_user);
    if (!page->no_reset && dirty) {
        if (cui_button_ex(ui, part(f, p, 0, 0, 34), f->theme, f->options.reset_label,
                          (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                               .size = CUI_CONTROL_SMALL,
                                               .icon = {"undo-2", 6},
                                               .icon_only = true})) {
            f->result->reset = cui_settings_reset_page(page, f->query);
        }
    }
    cui_end_box(ui);
    label(f, page->description, 14 * d, f->theme->muted);
    cui_end_box(ui);
    cui_rect header = cui_bounds(ui, part(f, p, 0, 0, 31));
    float available = fmaxf(1, f->height - (header.h > 0 ? header.h : 52 * d));
    cui_settings_page_state *state = &f->state->pages[p];
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < f->query.length; i++)
        hash = (hash ^ (unsigned char)f->query.data[i]) * UINT64_C(1099511628211);
    if (!state->query_initialized || state->query_hash != hash) {
        state->scroll = 0;
        state->query_initialized = true;
        state->query_hash = hash;
        if (f->state->selected_group != CUI_SETTINGS_NO_GROUP)
            f->state->pending_group_scroll = true;
    }
    cui_scroll_state scroll = state->scroll_state;
    scroll.y = state->scroll;
    if (f->state->pending_group_scroll) {
        cui_rect group = cui_bounds(ui, part(f, p, f->state->selected_group, 0, 20));
        if (group.h > 0 && state->content.h > 0) {
            scroll.y = fmaxf(0, group.y - state->content.y);
            f->state->pending_group_scroll = false;
        } else
            cui_invalidate(ui);
    }
    cui_scroll_options options = {.width = f->content_width,
                                  .height = available,
                                  .content_width = f->content_width,
                                  .content_height = fmaxf(available, state->content.h),
                                  .vertical = true,
                                  .wheel_step = 20 * d,
                                  .bar_thickness = 8 * d};
    float previous = state->scroll;
    if (cui_scrollable_begin(ui, part(f, p, 0, 0, 35), f->theme, &scroll, options)) {
        cui_track_bounds(ui, part(f, p, 0, 0, 35), &state->viewport);
        cui_begin_box(ui, part(f, p, 0, 0, 36), (cui_box){.width = -1, .row = true});
        cui_track_bounds(ui, part(f, p, 0, 0, 36), &state->content);
        cui_begin_box(ui, part(f, p, 0, 0, 46), (cui_box){.width = 16 * d});
        cui_end_box(ui);
        cui_begin_box(ui, part(f, p, 0, 0, 47), (cui_box){.width = -1});
        bool stacked = f->content_width <= 480;
        for (size_t g = 0; g < page->count; g++)
            if (cui_setting_group_matches(&page->groups[g], f->query))
                render_group(f, p, g, stacked);
        cui_end_box(ui);
        cui_begin_box(ui, part(f, p, 0, 0, 48), (cui_box){.width = 16 * d});
        cui_end_box(ui);
        cui_end_box(ui);
        cui_scrollable_end(ui, part(f, p, 0, 0, 35), f->theme, &scroll, options);
    }
    state->scroll_state = scroll;
    state->scroll = scroll.y;
    state->maximum = fmaxf(0, options.content_height - available);
    f->result->scrolled |= previous != state->scroll;
    cui_end_box(ui);
}
cui_settings_result cui_settings(cui_context *ui, cui_id id, const cui_theme *theme,
                                 const cui_setting_page *pages, size_t count,
                                 cui_settings_state *state, cui_settings_options options) {
    if (!ui || !id || !theme || !state || !isfinite(theme->font_size) || theme->font_size <= 0 ||
        !isfinite(options.width) || !isfinite(options.height) || options.height < 0 ||
        options.height > 1000000 || options.width > 1000000 || !isfinite(options.sidebar_width) ||
        !isfinite(options.sidebar_minimum) || !isfinite(options.sidebar_maximum) ||
        options.size < CUI_SETTINGS_MEDIUM || options.size > CUI_SETTINGS_LARGE ||
        options.variant < CUI_SETTINGS_NORMAL || options.variant > CUI_SETTINGS_OUTLINE)
        return (cui_settings_result){.invalid = true};
    cui_settings_result result = cui_settings_filter(pages, count, state, options);
    if (result.invalid)
        return result;
    for (size_t p = 0; p < count; p++)
        if (pages[p].count > 256)
            return (cui_settings_result){.invalid = true};
    float width = options.width > 0 ? options.width : state->bounds.w > 0 ? state->bounds.w : 800;
    float height = options.height > 0 ? options.height : 500;
    if (!state->resize.slots)
        cui_resizable_state_init(&state->resize, state->resize_slots, 2);
    float minimum = options.sidebar_minimum > 0 ? options.sidebar_minimum : 160;
    float maximum = options.sidebar_maximum > 0 ? options.sidebar_maximum : 360;
    if (maximum < minimum)
        return (cui_settings_result){.invalid = true};
    settings_frame frame = {ui,
                            id,
                            theme,
                            pages,
                            count,
                            state,
                            options,
                            &result,
                            state->search ? cui_edit_get_text(state->search) : options.query,
                            fmaxf(1, width - state->sidebar_width),
                            height};
    if (!frame.options.search_placeholder.length)
        frame.options.search_placeholder = cui_string("Search settings");
    if (!frame.options.reset_label.length)
        frame.options.reset_label = cui_string("Reset All");
    cui_resizable_panel panels[2] = {
        {.id = part(&frame, 0, 0, 0, 40),
         .initial_size = state->sidebar_width,
         .has_initial_size = true,
         .minimum = minimum,
         .maximum = maximum,
         .custom_range = true,
         .content = sidebar,
         .content_user = &frame},
        {.id = part(&frame, 0, 0, 0, 41), .content = content, .content_user = &frame}};
    float old_width = state->sidebar_width;
    cui_begin_box(ui, id, (cui_box){.width = options.width < 0 ? -1 : width, .height = height});
    cui_track_bounds(ui, id, &state->bounds);
    cui_resizable_error error =
        cui_resizable(ui, part(&frame, 0, 0, 0, 42), theme, &state->resize, panels, 2,
                      (cui_resizable_options){.extent = width,
                                              .cross_size = height,
                                              .now_seconds = cui_frame_time(ui),
                                              .key_step = 10});
    state->sidebar_width = state->resize.slots[0].size;
    result.resized = old_width != state->sidebar_width;
    result.invalid |= error == CUI_RESIZABLE_INVALID || error == CUI_RESIZABLE_CAPACITY;
    cui_end_box(ui);
    return result;
}
