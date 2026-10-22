/* Frozen pre-change ef086a41 paint oracle; independent of new resolver. */
static cui_box legacy_box(cui_context *ui, cui_id id, const cui_theme *theme,
                          cui_dropdown_button_options options, bool trigger, bool disabled,
                          bool pair_surfaced, cui_color *foreground,
                          const cui_dropdown_button_metrics *metrics) {
    bool no_padding =
        options.variant == CUI_DROPDOWN_BUTTON_LINK || options.variant == CUI_DROPDOWN_BUTTON_TEXT;
    bool loading = !trigger && options.action_loading && !options.action_disabled &&
                   !options.disabled && !options.menu.popup.disabled;
    bool inactive = disabled && !loading;
    bool hovered = !disabled && cui_hovered(ui, id);
    bool pressed = !disabled && cui_pressed(ui, id);
    bool active = options.selected || pressed;
    bool outlined = options.outline || options.variant == CUI_DROPDOWN_BUTTON_OUTLINE;
    cui_theme_button_kind kind = cui_dropdown_kind(options.variant);
    cui_theme_button_colors colors = theme->buttons[kind];
    cui_box box = {.height = isfinite(options.height) && options.height > 0 ? options.height
                             : no_padding                                   ? 0
                                                                            : metrics->height,
                   .width = trigger ? (no_padding ? metrics->caret_size : metrics->height) : 0,
                   .padding = 0,
                   .row = true,
                   .align_x = CUI_ALIGN_CENTER,
                   .align_y = CUI_ALIGN_CENTER,
                   .radius = theme->radius,
                   .background = outlined ? (active    ? colors.outline_active
                                             : hovered ? colors.outline_hover
                                                       : colors.outline_normal)
                                          : (active    ? colors.active
                                             : hovered ? colors.hover
                                                       : colors.normal),
                   .border = outlined ? colors.outline_border : colors.border,
                   .border_width = outlined || kind == CUI_THEME_BUTTON_DEFAULT ? 1 : 0};
    *foreground = outlined ? colors.outline_foreground : colors.foreground;
    if (!trigger)
        box.width = options.width;
    if (options.variant == CUI_DROPDOWN_BUTTON_GHOST) {
        box.border_width = 0;
        box.background = pressed            ? theme->buttons[CUI_THEME_BUTTON_DEFAULT].active
                         : options.selected ? theme->buttons[CUI_THEME_BUTTON_SECONDARY].active
                         : hovered          ? theme->accent
                         : pair_surfaced && !disabled ? cui_dropdown_opacity(theme->accent, .5f)
                                                      : (cui_color){0};
        *foreground = hovered ? theme->accent_foreground : theme->secondary_foreground;
    } else if (options.variant == CUI_DROPDOWN_BUTTON_LINK) {
        box.background = (cui_color){0};
        box.border_width = 0;
        *foreground = active ? theme->link_active : hovered ? theme->link_hover : theme->link;
    } else if (options.variant == CUI_DROPDOWN_BUTTON_TEXT) {
        box.background = (cui_color){0};
        box.border_width = 0;
        *foreground = cui_dropdown_opacity(theme->text, active ? .7f : hovered ? 1 : .9f);
    } else if (options.variant == CUI_DROPDOWN_BUTTON_CUSTOM) {
        box.background = active    ? options.custom.active
                         : hovered ? options.custom.hover
                                   : cui_dropdown_opacity(options.custom.color, .2f);
        box.border =
            outlined ? cui_dropdown_opacity(options.custom.color, .4f) : options.custom.color;
        box.border_width = outlined ? 1 : 0;
        *foreground = options.custom.foreground;
    }
    if (inactive) {
        *foreground = cui_dropdown_opacity(theme->muted, .5f);
        if (options.variant == CUI_DROPDOWN_BUTTON_GHOST ||
            options.variant == CUI_DROPDOWN_BUTTON_LINK ||
            options.variant == CUI_DROPDOWN_BUTTON_TEXT)
            box.background = box.border = (cui_color){0};
        else if (options.variant == CUI_DROPDOWN_BUTTON_CUSTOM) {
            box.background = cui_dropdown_opacity(options.custom.color, outlined ? .1f : .15f);
            box.border = cui_dropdown_opacity(box.border, outlined ? .5f : .15f);
        } else if (outlined) {
            box.background = cui_dropdown_opacity(colors.outline_normal, .5f);
            box.border = cui_dropdown_opacity(colors.outline_border, .5f);
        } else if (kind == CUI_THEME_BUTTON_DEFAULT) {
            box.background = cui_dropdown_opacity(theme->input_background, .5f);
            box.border = cui_dropdown_opacity(theme->input, .5f);
        } else {
            float opacity = kind == CUI_THEME_BUTTON_SECONDARY ? 1.5f : .15f;
            box.background = cui_dropdown_opacity(colors.normal, opacity);
            box.border = cui_dropdown_opacity(colors.border, opacity);
        }
    }
    if (loading) {
        box.custom_opacity = true;
        box.opacity = .8f;
    }
    if (cui_focused(ui) == id) {
        box.border = theme->focus;
        box.border_width = 2;
    }
    box.custom_corners = true;
    box.corner_radii =
        trigger ? (cui_corner_radii){.top_right = theme->radius, .bottom_right = theme->radius}
                : (cui_corner_radii){.top_left = theme->radius, .bottom_left = theme->radius};
    box.custom_border_edges = true;
    box.border_edges = (cui_border_edges){.left = trigger ? 0 : box.border_width,
                                          .right = box.border_width,
                                          .top = box.border_width,
                                          .bottom = box.border_width};
    return box;
}

/* Frozen submission/paint tree, including Base delegation and semantic role. */
static void legacy_label(cui_context *ui, cui_id id, cui_str text, float size, cui_color color,
                         bool underline) {
    cui_begin_box(ui, id ^ UINT64_C(0xc8134e39119b750d),
                  (cui_box){.height = size * 1.25f, .align_y = CUI_ALIGN_CENTER});
    cui_text(ui, text, size, color);
    if (underline) {
        cui_size measured = cui_measure_text(ui, text, size);
        cui_begin_box(ui, id ^ UINT64_C(0xc8134e39119b751d),
                      (cui_box){.width = measured.width, .height = 1, .background = color});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
static cui_dropdown_button_result
legacy_render(cui_context *ui, cui_id container, cui_id action_id, cui_id trigger_id,
              const cui_theme *theme, cui_str action_name, cui_str menu_name,
              const cui_menu_entry *items, size_t count, cui_id *menu_action,
              cui_menu_tree_state *state, cui_dropdown_button_options options,
              cui_dropdown_event_handlers handlers, cui_dropdown_event_result *observed) {
    cui_dropdown_button_result result = {0};
    if (!ui || !theme || !state || !cui_design_valid(options.design_size) || !state->overlay.id ||
        !menu_action || (!items && count) || !container ||
        (options.hide_action && options.hide_menu) || (!options.hide_action && !action_id) ||
        (!options.hide_menu && !trigger_id) || action_id == trigger_id || container == action_id ||
        container == trigger_id || !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 || !isfinite(options.width))
        return result;
    if (options.inherit_action_variant)
        options.variant = options.action_variant;
    if (options.inherit_action_size)
        options.size = options.action_size;
    options.selected = options.selected || (!options.hide_action && options.action_selected);
    float density =
        isfinite(options.density) && options.density > 0 ? options.density : theme->font_size / 16;
    cui_dropdown_button_metrics metrics = cui_dropdown_button_size_metrics(options.size, density);
    if (!cui_design_valid(options.design_size))
        return (cui_dropdown_button_result){0};
    if (options.design_size) {
        cui_design_metrics d;
        if (!cui_design_resolve(options.design_size, density, &d))
            return (cui_dropdown_button_result){0};
        metrics = (cui_dropdown_button_metrics){d.control_height, d.padding_x, d.font_size, d.gap,
                                                d.indicator_size};
    }
    state->drag_scroll = options.menu.drag_scroll;
    if (observed) {
        *observed = cui_base_dropdown_button_events(
            ui, options.hide_action ? 0 : action_id, options.hide_menu ? 0 : trigger_id,
            action_name, menu_name, items, count, menu_action, state,
            options.disabled || options.menu.popup.disabled, options.action_disabled,
            options.action_loading, options.menu.popup.dt, options.menu.keep_open_checks, handlers);
        result = observed->legacy;
    } else
        result = cui_base_dropdown_button(
            ui, options.hide_action ? 0 : action_id, options.hide_menu ? 0 : trigger_id,
            action_name, menu_name, items, count, menu_action, state,
            options.disabled || options.menu.popup.disabled, options.action_disabled,
            options.action_loading, options.menu.popup.dt, options.menu.keep_open_checks);
    if (options.variant == CUI_DROPDOWN_BUTTON_LINK) {
        if (!options.hide_action)
            cui_set_semantic_role(ui, action_id, CUI_ROLE_LINK);
        if (!options.hide_menu)
            cui_set_semantic_role(ui, trigger_id, CUI_ROLE_LINK);
    }
    bool all_disabled = options.disabled || options.menu.popup.disabled;
    bool action_disabled = all_disabled || options.action_disabled || options.action_loading;
    bool pair_surfaced = state->overlay.open ||
                         (!options.hide_action && cui_hovered(ui, action_id)) ||
                         (!options.hide_menu && cui_hovered(ui, trigger_id));
    cui_color trigger_foreground, action_foreground;
    bool no_padding =
        options.variant == CUI_DROPDOWN_BUTTON_LINK || options.variant == CUI_DROPDOWN_BUTTON_TEXT;
    float padding = no_padding ? 0 : metrics.padding_x;
    cui_begin_box(ui, container, (cui_box){.row = true, .gap = 0, .align_y = CUI_ALIGN_CENTER});
    if (!options.hide_action) {
        cui_dropdown_button_options action_options = options;
        action_options.outline = options.outline || options.action_outline;
        cui_box action_box =
            legacy_box(ui, action_id, theme, action_options, false, action_disabled, pair_surfaced,
                       &action_foreground, &metrics);
        cui_begin_box(ui, action_id, action_box);
        if (padding > 0) {
            cui_begin_box(ui, 0, (cui_box){.width = padding});
            cui_end_box(ui);
        }
        cui_begin_box(
            ui, action_id ^ UINT64_C(0x4aef702b79ab3160),
            (cui_box){.row = true,
                      .gap = metrics.gap,
                      .max_width = options.width > 0 ? fmaxf(0, options.width - 2 * padding) : 0,
                      .clip = true,
                      .align_y = CUI_ALIGN_CENTER});
        {
            if (options.action_loading && options.leading.length)
                (void)cui_icon_spinner(
                    ui, action_id ^ UINT64_C(0x4aef702b79ab3181), theme,
                    (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                       .custom_size =
                                           !options.design_size &&
                                                   options.size == CUI_DROPDOWN_BUTTON_LARGE
                                               ? 24 * theme->font_size / 16
                                               : metrics.caret_size,
                                       .custom_color = true,
                                       .color = action_foreground});
            else if (options.leading.length)
                legacy_label(ui, action_id ^ UINT64_C(0x4aef702b79ab3180), options.leading,
                             metrics.font_size, action_foreground, false);
            if (options.content)
                options.content(ui, options.content_user);
            else
                legacy_label(ui, action_id, action_name, metrics.font_size, action_foreground,
                             options.variant == CUI_DROPDOWN_BUTTON_LINK);
            if (options.trailing.length)
                legacy_label(ui, action_id ^ UINT64_C(0x4aef702b79ab3190), options.trailing,
                             metrics.font_size, action_foreground, false);
        }
        cui_end_box(ui);
        if (padding > 0) {
            cui_begin_box(ui, 0, (cui_box){.width = padding});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    if (!options.hide_menu) {
        cui_box trigger_box = legacy_box(ui, trigger_id, theme, options, true, all_disabled,
                                         pair_surfaced, &trigger_foreground, &metrics);
        cui_begin_box(ui, trigger_id, trigger_box);
        cui_begin_box(ui, trigger_id ^ UINT64_C(0x4aef702b79ab3170),
                      (cui_box){.width = metrics.caret_size,
                                .height = metrics.caret_size,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_color caret = trigger_foreground;
        caret.a = (uint8_t)lroundf(caret.a * .75f);
        (void)cui_icon_named(ui, trigger_id ^ UINT64_C(0x4aef702b79ab3171), theme,
                             cui_string("chevron-down"),
                             (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                                .custom_size = metrics.caret_size,
                                                .custom_color = true,
                                                .color = caret});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_choice_menu_retain_metrics(state, cui_choice_menu_metrics(theme, options.menu));
    if (options.tooltip_id && options.tooltip_state) {
        cui_hover_options tooltip = options.tooltip_options;
        tooltip.trigger = action_id;
        tooltip.disabled =
            tooltip.disabled || action_disabled || options.hide_action || state->overlay.open;
        cui_tooltip(ui, options.tooltip_id, theme, options.tooltip_state, options.tooltip, tooltip);
    }
    options.menu.popup.side = options.anchor == CUI_DROPDOWN_ANCHOR_BOTTOM_RIGHT ||
                                      options.anchor == CUI_DROPDOWN_ANCHOR_BOTTOM_LEFT
                                  ? CUI_POPUP_ABOVE
                                  : CUI_POPUP_BELOW;
    options.menu.popup.align_end = options.anchor == CUI_DROPDOWN_ANCHOR_TOP_RIGHT ||
                                   options.anchor == CUI_DROPDOWN_ANCHOR_BOTTOM_RIGHT;
    if (state->overlay.open)
        cui_choice_menu_level(ui, theme, items, count, state, 0, cui_bounds(ui, trigger_id),
                              options.menu, menu_name);
    return result;
}
