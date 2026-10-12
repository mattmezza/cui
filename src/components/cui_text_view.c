/* SPDX-License-Identifier: MIT */
#include "cui/cui_text_view.h"
#include <math.h>
#include <string.h>
cui_text_view_style cui_text_view_style_default(void) {
    return (cui_text_view_style){.foreground = {24, 24, 27, 255},
                                 .muted = {113, 113, 122, 255},
                                 .link = {37, 99, 235, 255},
                                 .selection = {59, 130, 246, 80},
                                 .code_background = {244, 244, 245, 255},
                                 .border = {228, 228, 231, 255},
                                 .mark = {255, 220, 0, 100},
                                 .font_size = 14,
                                 .line_height = 20,
                                 .paragraph_gap = 16};
}
cui_text_view_style cui_text_view_style_from_theme(const cui_theme *t) {
    if (!t)
        return cui_text_view_style_default();
    return (cui_text_view_style){.foreground = t->text,
                                 .muted = t->muted,
                                 .link = t->link,
                                 .selection = t->selection,
                                 .code_background = t->accent,
                                 .border = t->border,
                                 .mark = t->warning,
                                 .font_size = t->font_size * .875f,
                                 .line_height = t->font_size * 1.25f,
                                 .paragraph_gap = t->font_size};
}
cui_text_view_options cui_text_view_default(void) {
    return (cui_text_view_options){.width = -1,
                                   .height = -1,
                                   .selectable = true,
                                   .code_actions_height = 32,
                                   .table_actions_height = 32};
}
static cui_text_style cui_text_view_text_style(unsigned flags) {
    return (cui_text_style){.weight = flags & CUI_TEXT_VIEW_BOLD       ? CUI_TEXT_WEIGHT_BOLD
                                      : flags & CUI_TEXT_VIEW_SEMIBOLD ? CUI_TEXT_WEIGHT_SEMIBOLD
                                      : flags & CUI_TEXT_VIEW_MEDIUM   ? CUI_TEXT_WEIGHT_MEDIUM
                                                                       : CUI_TEXT_WEIGHT_NORMAL,
                            .italic = (flags & CUI_TEXT_VIEW_ITALIC) != 0,
                            .monospace = (flags & CUI_TEXT_VIEW_CODE) != 0};
}
static cui_size cui_text_view_core_measure(void *ui, cui_str text, float font, unsigned flags) {
    return cui_measure_text_ex(ui, text, font, cui_text_view_text_style(flags));
}
static cui_str cui_text_view_slice(cui_str s, cui_text_view_range r) {
    return r.begin <= r.end && r.end <= s.length ? (cui_str){s.data + r.begin, r.end - r.begin}
                                                 : (cui_str){0};
}
static bool cui_text_view_overlap(cui_text_view_range a, cui_text_view_range b) {
    return a.begin < b.end && a.end > b.begin;
}
static size_t cui_text_view_previous(cui_str text, size_t at) {
    if (at) {
        --at;
        while (at && ((unsigned char)text.data[at] & 0xc0) == 0x80)
            --at;
    }
    return at;
}
static size_t cui_text_view_next(cui_str text, size_t at) {
    if (at < text.length) {
        ++at;
        while (at < text.length && ((unsigned char)text.data[at] & 0xc0) == 0x80)
            ++at;
    }
    return at;
}
static bool cui_text_view_selection_target(const cui_text_view_state *s, cui_id root,
                                           cui_id target) {
    if (target == root || target == (root ^ UINT64_C(0x891dd9c052ad9f04)))
        return true;
    for (size_t i = 0; i < s->glyph_count; ++i) {
        const cui_text_view_glyph *g = &s->workspace.glyphs[i];
        const cui_text_view_span *span = &s->workspace.spans[g->span];
        if (span->link.begin == span->link.end)
            continue;
        if (i) {
            const cui_text_view_glyph *prev = &s->workspace.glyphs[i - 1];
            const cui_text_view_span *old = &s->workspace.spans[prev->span];
            if (prev->bounds.y == g->bounds.y && old->link.begin == span->link.begin &&
                old->link.end == span->link.end)
                continue;
        }
        cui_id link = root ^ UINT64_C(0x951f5d7100000000) ^ (uint64_t)span->link.begin ^
                      ((uint64_t)g->text.begin << 24);
        if (target == link)
            return true;
    }
    return false;
}
static void cui_text_view_reduce(cui_context *ui, cui_id id, cui_text_view_state *s,
                                 cui_text_view_options o) {
    if (!o.selectable || o.disabled) {
        s->selecting = false;
        return;
    }
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        const cui_event *e = &events[i];
        if (e->type == CUI_EVENT_CANCEL) {
            s->selecting = false;
            s->dragged = false;
            continue;
        }
        if (cui_event_consumed(ui, i)) {
            if (e->type == CUI_EVENT_POINTER_UP && e->pointer_button == CUI_POINTER_PRIMARY)
                s->selecting = false;
            continue;
        }
        bool pointer = e->type == CUI_EVENT_POINTER_DOWN || e->type == CUI_EVENT_POINTER_MOVE ||
                       e->type == CUI_EVENT_POINTER_UP;
        bool owned = pointer && cui_event_within_scope(ui, i, id) &&
                     cui_text_view_selection_target(s, id, cui_event_target(ui, i));
        if (pointer && ((e->type == CUI_EVENT_POINTER_DOWN && owned) ||
                        (s->selecting && cui_pointer_scope_allowed(ui, id)))) {
            size_t at =
                cui_text_view_hit(s, e->x - s->viewport.x, e->y - s->viewport.y + s->scroll.y);
            if (at == SIZE_MAX)
                continue;
            if (e->type == CUI_EVENT_POINTER_DOWN && e->pointer_button == CUI_POINTER_PRIMARY) {
                s->selecting = true;
                s->dragged = false;
                s->pointer_x = s->press_x = e->x;
                s->pointer_y = s->press_y = e->y;
                (void)cui_text_view_select(s, e->modifiers & CUI_MOD_SHIFT ? s->anchor : at, at);
            } else if (s->selecting && (e->type != CUI_EVENT_POINTER_UP ||
                                        e->pointer_button == CUI_POINTER_PRIMARY)) {
                if (fabsf(e->x - s->press_x) > 2 || fabsf(e->y - s->press_y) > 2)
                    s->dragged = true;
                s->pointer_x = e->x;
                s->pointer_y = e->y;
                if (s->dragged) {
                    (void)cui_text_view_select(s, s->anchor, at);
                    cui_focus(ui, id);
                    (void)cui_cancel_event_activation(ui, i);
                    (void)cui_consume_event(ui, i);
                }
                if (e->type == CUI_EVENT_POINTER_UP)
                    s->selecting = false;
            }
            cui_invalidate(ui);
        }
        if (e->type == CUI_EVENT_KEY_DOWN && cui_focused(ui) == id) {
            cui_str text = cui_text_view_rendered_text(s);
            bool handled = false;
            if ((e->modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) && e->key == CUI_KEY_A) {
                (void)cui_text_view_select(s, 0, text.length);
                handled = true;
            } else if ((e->modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) && e->key == CUI_KEY_C &&
                       s->anchor != s->cursor) {
                if (o.clipboard && (o.clipboard->capabilities & CUI_PLATFORM_CLIPBOARD) &&
                    o.clipboard->clipboard_set) {
                    if (o.copy_format == CUI_TEXT_VIEW_COPY_SOURCE && !o.scrollable &&
                        s->format == CUI_TEXT_VIEW_MARKDOWN) {
                        size_t length;
                        if (cui_text_view_copy_selection(s, o.copy_format, o.copy_buffer,
                                                         o.copy_capacity,
                                                         &length) == CUI_TEXT_VIEW_OK)
                            (void)o.clipboard->clipboard_set(o.clipboard->user,
                                                             (cui_str){o.copy_buffer, length});
                    } else
                        (void)o.clipboard->clipboard_set(o.clipboard->user,
                                                         cui_text_view_selected_text(s));
                }
                handled = true;
            } else if (!(e->modifiers & (CUI_MOD_CTRL | CUI_MOD_META | CUI_MOD_ALT)) &&
                       (e->key == CUI_KEY_LEFT || e->key == CUI_KEY_RIGHT ||
                        e->key == CUI_KEY_HOME || e->key == CUI_KEY_END)) {
                size_t at = e->key == CUI_KEY_HOME   ? 0
                            : e->key == CUI_KEY_END  ? text.length
                            : e->key == CUI_KEY_LEFT ? cui_text_view_previous(text, s->cursor)
                                                     : cui_text_view_next(text, s->cursor);
                (void)cui_text_view_select(s, e->modifiers & CUI_MOD_SHIFT ? s->anchor : at, at);
                handled = true;
            } else if (e->key == CUI_KEY_ESCAPE && s->anchor != s->cursor) {
                (void)cui_text_view_select(s, s->cursor, s->cursor);
                handled = true;
            }
            if (handled) {
                (void)cui_consume_event(ui, i);
                cui_invalidate(ui);
            }
        }
    }
}
static float cui_text_view_opacity(const cui_text_view_state *s, size_t at) {
    if (!s->stream_fade || s->reduced_motion || s->workspace.mapping[at].born < 0)
        return 1;
    float age = s->time - s->workspace.mapping[at].born;
    float t = s->fade_duration > 0 ? fminf(fmaxf(age / s->fade_duration, 0), 1) : 1;
    return 1 - (1 - t) * (1 - t) * (1 - t);
}
static cui_color cui_text_view_foreground(const cui_text_view_state *s,
                                          const cui_text_view_glyph *g, cui_text_view_style style,
                                          cui_text_view_options o) {
    cui_text_view_span *span = &s->workspace.spans[g->span];
    cui_text_view_block *block = &s->workspace.blocks[g->block];
    cui_color color = span->link.end > span->link.begin    ? style.link
                      : block->kind == CUI_TEXT_VIEW_QUOTE ? style.muted
                                                           : style.foreground;
    if (o.highlighter && block->kind == CUI_TEXT_VIEW_CODE_BLOCK) {
        cui_str code = cui_text_view_slice(cui_text_view_rendered_text(s), block->text),
                lang = cui_text_view_slice(cui_text_view_source(s), block->resource);
        cui_color resolved;
        if (o.highlighter(code, lang, g->text.begin - block->text.begin, &resolved,
                          o.highlighter_user))
            color = resolved;
    }
    return color;
}
static bool cui_text_view_color_equal(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static bool cui_text_view_visible(const cui_text_view_state *s, cui_rect r, float scroll,
                                  float viewport) {
    if (r.y + r.h <= scroll || r.y >= scroll + viewport)
        return false;
    /* Fit clamping removes a straddling normal line whole. An oversized line keeps its visible
     * part. */
    if (s->clamped && r.y + r.h > s->height && r.h <= s->height)
        return false;
    return true;
}
static void cui_text_view_quad(cui_context *ui, cui_id id, cui_rect r, cui_color color) {
    if (!color.a || r.w <= 0 || r.h <= 0)
        return;
    cui_begin_absolute_box(ui, id, (cui_box){.background = color}, r);
    cui_end_box(ui);
}
static void cui_text_view_link(cui_context *ui, cui_id id, cui_text_view_state *s,
                               cui_text_view_range uri, bool activated, cui_text_view_options o) {
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    cui_str url = cui_text_view_slice(cui_text_view_source(s), uri);
    for (size_t i = 0; i < count; ++i) {
        if (cui_event_consumed(ui, i))
            continue;
        const cui_event *e = &events[i];
        bool click = activated && cui_event_activation(ui, i) == id;
        if (e->type == CUI_EVENT_POINTER_DOWN && e->pointer_button == CUI_POINTER_SECONDARY &&
            cui_event_target(ui, i) == id)
            s->secondary_link = id;
        if (e->type == CUI_EVENT_POINTER_UP && e->pointer_button == CUI_POINTER_SECONDARY &&
            s->secondary_link == id) {
            s->secondary_link = 0;
            click = cui_event_target(ui, i) == id;
        }
        if (!click)
            continue;
        if (o.on_link) {
            o.on_link(url, e, o.link_user);
            (void)cui_consume_event(ui, i);
        } else if (o.open_url && e->pointer_button == CUI_POINTER_PRIMARY) {
            (void)o.open_url(url, o.link_user);
            (void)cui_consume_event(ui, i);
        }
    }
}
static void cui_text_view_paint(cui_context *ui, cui_id id, const cui_theme *theme,
                                cui_text_view_state *s, cui_text_view_style style,
                                cui_text_view_options o, float viewport) {
    float scroll = o.scrollable ? s->scroll.y : 0;
    for (size_t i = 0; i < s->block_count; ++i) {
        cui_text_view_block *b = &s->workspace.blocks[i];
        cui_rect r = b->bounds;
        if (r.y + r.h <= scroll || r.y >= scroll + viewport)
            continue;
        cui_id part = id ^ UINT64_C(0x451ddbcb00000000) ^ (uint64_t)i;
        if (b->kind == CUI_TEXT_VIEW_CODE_BLOCK) {
            cui_text_view_quad(ui, part, r, style.code_background);
        } else if (b->kind == CUI_TEXT_VIEW_TABLE_CELL) {
            if (s->clamped && r.y + 8 + style.line_height > s->height &&
                style.line_height <= s->height)
                continue;
            cui_begin_absolute_box(
                ui, part,
                (cui_box){.background = b->table_header ? style.code_background : (cui_color){0},
                          .border = style.border,
                          .border_width = 1},
                r);
            cui_end_box(ui);
        } else if (b->kind == CUI_TEXT_VIEW_QUOTE)
            cui_text_view_quad(ui, part, (cui_rect){r.x, r.y, 3, r.h}, style.border);
        else if (b->kind == CUI_TEXT_VIEW_RULE)
            cui_text_view_quad(ui, part, r, style.border);
        else if (b->kind == CUI_TEXT_VIEW_LIST_ITEM) {
            cui_begin_absolute_box(
                ui, part, (cui_box){0},
                (cui_rect){r.x, r.y, fmaxf(16, b->text_inset - 8), style.line_height});
            cui_text_ex(ui,
                        b->level ? (cui_str){b->marker_text, strlen(b->marker_text)}
                                 : (cui_str){"\xe2\x80\xa2", 3},
                        style.font_size, style.foreground, style.line_height, (cui_text_style){0});
            cui_end_box(ui);
        } else if (b->kind == CUI_TEXT_VIEW_IMAGE && b->image_ready) {
            cui_begin_absolute_box(ui, part, (cui_box){0}, r);
            (void)cui_paint_bitmap(ui, part ^ 1,
                                   (cui_bitmap_paint){.bitmap = b->image, .opacity = 1},
                                   (cui_box){.width = r.w, .height = r.h});
            cui_end_box(ui);
        }
    }
    /* Link hit regions follow the full laid-out link line, independently of
     * selection/highlight/fade paint fragmentation. */
    for (size_t i = 0; i < s->glyph_count;) {
        cui_text_view_glyph *first = &s->workspace.glyphs[i];
        cui_text_view_span *span = &s->workspace.spans[first->span];
        if (span->link.begin == span->link.end ||
            !cui_text_view_visible(s, first->bounds, scroll, viewport)) {
            ++i;
            continue;
        }
        size_t end = i + 1;
        while (end < s->glyph_count) {
            cui_text_view_glyph *g = &s->workspace.glyphs[end];
            cui_text_view_span *next = &s->workspace.spans[g->span];
            if (g->bounds.y != first->bounds.y || next->link.begin != span->link.begin ||
                next->link.end != span->link.end)
                break;
            ++end;
        }
        cui_text_view_glyph *last = &s->workspace.glyphs[end - 1];
        cui_rect r = first->bounds;
        r.w = last->bounds.x + last->bounds.w - r.x;
        cui_id link_id = id ^ UINT64_C(0x951f5d7100000000) ^ (uint64_t)span->link.begin ^
                         ((uint64_t)first->text.begin << 24);
        cui_interaction interaction = cui_interact(
            ui, link_id, CUI_ROLE_LINK,
            cui_text_view_slice(cui_text_view_rendered_text(s),
                                (cui_text_view_range){first->text.begin, last->text.end}),
            o.disabled, false, 0);
        cui_set_semantic_details(ui, link_id, (cui_semantic_details){.cursor = CUI_CURSOR_POINTER});
        cui_begin_absolute_box(ui, link_id, (cui_box){0}, r);
        cui_end_box(ui);
        cui_text_view_link(ui, link_id, s, span->link, interaction.activated, o);
        i = end;
    }
    cui_text_view_range selection = {s->anchor < s->cursor ? s->anchor : s->cursor,
                                     s->anchor > s->cursor ? s->anchor : s->cursor};
    for (size_t i = 0; i < s->glyph_count;) {
        cui_text_view_glyph *first = &s->workspace.glyphs[i];
        if (!cui_text_view_visible(s, first->bounds, scroll, viewport)) {
            ++i;
            continue;
        }
        cui_color color = cui_text_view_foreground(s, first, style, o);
        bool selected = o.selectable && cui_text_view_overlap(first->text, selection);
        float opacity = cui_text_view_opacity(s, first->text.begin);
        size_t end = i + 1;
        while (end < s->glyph_count) {
            cui_text_view_glyph *g = &s->workspace.glyphs[end],
                                *previous = &s->workspace.glyphs[end - 1];
            if (g->span != first->span || g->bounds.y != first->bounds.y ||
                g->text.begin != previous->text.end ||
                cui_text_view_overlap(g->text, selection) != selected ||
                cui_text_view_opacity(s, g->text.begin) != opacity ||
                !cui_text_view_color_equal(cui_text_view_foreground(s, g, style, o), color))
                break;
            bool same = true;
            for (size_t h = 0; h < s->highlight_count; ++h)
                if (cui_text_view_overlap(g->text, s->workspace.highlights[h].range) !=
                    cui_text_view_overlap(first->text, s->workspace.highlights[h].range)) {
                    same = false;
                    break;
                }
            if (!same)
                break;
            ++end;
        }
        cui_text_view_glyph *last = &s->workspace.glyphs[end - 1];
        cui_rect r = first->bounds;
        r.w = last->bounds.x + last->bounds.w - r.x;
        cui_id leaf = id ^ UINT64_C(0x164b82d900000000) ^ (uint64_t)first->text.begin;
        cui_text_view_span *span = &s->workspace.spans[first->span];
        bool is_link = span->link.end > span->link.begin;
        if (first->flags & CUI_TEXT_VIEW_CODE)
            cui_text_view_quad(ui, leaf ^ UINT64_C(0x719ccbbb), r, style.code_background);
        for (size_t h = 0; h < s->highlight_count; ++h)
            if (cui_text_view_overlap(first->text, s->workspace.highlights[h].range))
                cui_text_view_quad(ui, leaf ^ UINT64_C(0x9000000000000000) ^ (uint64_t)h, r,
                                   s->workspace.highlights[h].background);
        if (first->flags & CUI_TEXT_VIEW_MARK)
            cui_text_view_quad(ui, leaf ^ UINT64_C(0x714c1d09), r, style.mark);
        if (selected)
            cui_text_view_quad(ui, leaf ^ UINT64_C(0x521e8438), r, style.selection);
        cui_begin_absolute_box(ui, leaf, (cui_box){.custom_opacity = true, .opacity = opacity}, r);
        cui_text_ex(ui,
                    cui_text_view_slice(cui_text_view_rendered_text(s),
                                        (cui_text_view_range){first->text.begin, last->text.end}),
                    first->font_size, color, first->line_height,
                    cui_text_view_text_style(first->flags));
        if (is_link)
            cui_text_view_quad(ui, leaf ^ 1, (cui_rect){0, r.h - 2, r.w, 1}, color);
        if (first->flags & CUI_TEXT_VIEW_STRIKE)
            cui_text_view_quad(ui, leaf ^ 2, (cui_rect){0, r.h * .5f, r.w, 1}, color);
        cui_end_box(ui);

        i = end;
    }
    /* Actions paint after document glyphs so the code overlay covers underlying text. */
    for (size_t i = 0; i < s->block_count; ++i) {
        cui_text_view_block *b = &s->workspace.blocks[i];
        cui_id part = id ^ UINT64_C(0x451ddbcb00000000) ^ (uint64_t)i;
        if (b->actions_height > 0 && ((b->kind == CUI_TEXT_VIEW_CODE_BLOCK && o.code_actions) ||
                                      (b->kind == CUI_TEXT_VIEW_TABLE_CELL && o.table_actions))) {
            bool code = b->kind == CUI_TEXT_VIEW_CODE_BLOCK;
            float y = code ? b->bounds.y + 8 : b->bounds.y + b->bounds.h + 4;
            cui_rect action_bounds = {code ? 8 : 0, y, code ? fmaxf(1, s->width - 16) : s->width,
                                      b->actions_height};
            if (action_bounds.y + action_bounds.h <= scroll || action_bounds.y >= scroll + viewport)
                continue;
            cui_id action_id = part ^ UINT64_C(0x513b5e17);
            cui_begin_absolute_box(ui, action_id,
                                   (cui_box){.align_x = code ? CUI_ALIGN_END : CUI_ALIGN_START},
                                   action_bounds);
            if (code)
                cui_begin_box(
                    ui, action_id ^ UINT64_C(0x813b17),
                    (cui_box){.background = style.code_background, .radius = theme->radius});
            if (b->kind == CUI_TEXT_VIEW_CODE_BLOCK)
                o.code_actions(ui, action_id, theme,
                               cui_text_view_slice(cui_text_view_rendered_text(s), b->text),
                               cui_text_view_slice(cui_text_view_source(s), b->resource),
                               o.actions_user);
            else
                o.table_actions(ui, action_id, theme, s, b->table, o.actions_user);
            if (code)
                cui_end_box(ui);
            cui_end_box(ui);
        }
    }
}
bool cui_text_view(cui_context *ui, cui_id id, const cui_theme *theme, cui_text_view_state *s,
                   cui_text_view_options o) {
    if (!ui || !id || !theme || !s || !isfinite(o.width) || !isfinite(o.height) ||
        !isfinite(o.dt) || o.dt < 0 || !isfinite(o.code_actions_height) ||
        o.code_actions_height < 0 || !isfinite(o.table_actions_height) ||
        o.table_actions_height < 0)
        return false;
    cui_text_view_style style = o.custom_style ? o.style : cui_text_view_style_from_theme(theme);
    float width = o.width > 0 ? o.width : s->viewport.w > 0 ? s->viewport.w : 320;
    float viewport = o.height > 0 ? o.height : s->viewport.h > 0 ? s->viewport.h : 240;
    float content_width = o.scrollable ? fmaxf(1, width - 10) : width;
    s->stream_fade = o.stream_fade;
    s->reduced_motion = o.reduced_motion;
    for (size_t i = 0; i < s->block_count; ++i)
        if (s->workspace.blocks[i].kind == CUI_TEXT_VIEW_IMAGE) {
            cui_text_view_block *b = &s->workspace.blocks[i];
            cui_bitmap bitmap = {0};
            b->image_ready =
                o.image && o.image(cui_text_view_slice(cui_text_view_source(s), b->resource),
                                   &bitmap, o.image_user);
            b->image = bitmap;
        }
    if (cui_text_view_layout(
            s,
            (cui_text_view_layout_options){
                .width = content_width,
                .font_size = style.font_size,
                .line_height = style.line_height,
                .paragraph_gap = style.paragraph_gap,
                .max_lines = o.max_lines,
                .scrollable = o.scrollable,
                .code_actions_height = o.code_actions ? o.code_actions_height : 0,
                .table_actions_height = o.table_actions ? o.table_actions_height : 0},
            cui_text_view_core_measure, ui) != CUI_TEXT_VIEW_OK)
        return false;
    if (!cui_begin_focus_scope(ui, id, false))
        return false;
    (void)cui_interact(ui, id, CUI_ROLE_GROUP, cui_text_view_rendered_text(s), o.disabled, false,
                       0);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.focusable = o.selectable,
                               .pointer_activatable = o.selectable,
                               .readonly = true,
                               .cursor = o.selectable ? CUI_CURSOR_TEXT : CUI_CURSOR_DEFAULT});
    cui_set_tab_stop(ui, id, false);
    cui_text_view_reduce(ui, id, s, o);
    if (!o.scrollable) {
        viewport = s->height;
        s->scroll.y = 0;
    }
    if (s->reveal_pending) {
        cui_rect r = {0};
        bool found = false;
        for (size_t i = 0; i < s->glyph_count; ++i) {
            cui_text_view_glyph *g = &s->workspace.glyphs[i];
            if ((s->reveal.begin >= g->text.begin && s->reveal.begin < g->text.end) ||
                (s->reveal.begin == s->text_length && i + 1 == s->glyph_count)) {
                r = g->bounds;
                found = true;
                break;
            }
        }
        if (!found)
            for (size_t i = 0; i < s->block_count; ++i) {
                cui_text_view_block *b = &s->workspace.blocks[i];
                if (s->reveal.begin >= b->text.begin && s->reveal.begin <= b->text.end) {
                    r = b->bounds;
                    found = true;
                    break;
                }
            }
        if (found && o.scrollable) {
            if (r.y < s->scroll.y)
                s->scroll.y = r.y;
            else if (r.y + r.h > s->scroll.y + viewport)
                s->scroll.y = r.y + r.h - viewport;
            s->reveal_pending = false;
        } else if (found) {
            if (o.on_reveal)
                o.on_reveal((cui_rect){s->viewport.x + r.x, s->viewport.y + r.y, r.w, r.h},
                            o.reveal_user);
            s->reveal_pending = false;
        }
    }
    if (s->selecting && o.scrollable && s->viewport.h > 0) {
        float speed = s->pointer_y < s->viewport.y + 12                   ? -120
                      : s->pointer_y > s->viewport.y + s->viewport.h - 12 ? 120
                                                                          : 0;
        float before = s->scroll.y;
        s->scroll.y = fminf(fmaxf(0, s->scroll.y + speed * o.dt), fmaxf(0, s->height - viewport));
        if (before != s->scroll.y) {
            size_t at = cui_text_view_hit(s, s->pointer_x - s->viewport.x,
                                          s->pointer_y - s->viewport.y + s->scroll.y);
            if (at != SIZE_MAX)
                (void)cui_text_view_select(s, s->anchor, at);
            cui_invalidate(ui);
        }
    }
    cui_begin_box(ui, id,
                  (cui_box){.width = o.width,
                            .height = o.scrollable ? o.height : s->height,
                            .fixed_height = !o.scrollable,
                            .clip = true,
                            .inert = o.disabled});
    (void)cui_track_bounds(ui, id, &s->viewport);
    if (o.scrollable) {
        cui_theme scroll_theme = *theme;
        scroll_theme.surface = (cui_color){0};
        cui_id scroll_id = id ^ UINT64_C(0x891dd9c052ad9f04);
        cui_scroll_options scroll = {.width = width,
                                     .height = viewport,
                                     .content_height = s->height,
                                     .vertical = true,
                                     .disabled = o.disabled};
        if (cui_scrollable_begin(ui, scroll_id, &scroll_theme, &s->scroll, scroll)) {
            cui_text_view_paint(ui, id, theme, s, style, o, viewport);
            cui_scrollable_end_ex(ui, scroll_id, &scroll_theme, &s->scroll, scroll);
        }
    } else
        cui_text_view_paint(ui, id, theme, s, style, o, viewport);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    if (cui_text_view_advance(s, o.dt))
        cui_invalidate(ui);
    return true;
}
