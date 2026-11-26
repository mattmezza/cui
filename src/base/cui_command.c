/* SPDX-License-Identifier: MIT */
#include "cui_command_events_private.h"
#include "cui_command_unicode.inc"
#include "cui_utf8_internal.h"
#include <ctype.h>
#include <math.h>
#include <string.h>

enum { CUI_COMMAND_CASED = 1, CUI_COMMAND_CASE_IGNORABLE = 2, CUI_COMMAND_SPACE = 4 };

static bool cui_command_str_valid(cui_str s) {
    return !s.length || s.data;
}
static cui_str cui_command_trim_custom(cui_str q) {
    while (q.length && isspace((unsigned char)*q.data)) {
        ++q.data;
        --q.length;
    }
    while (q.length && isspace((unsigned char)q.data[q.length - 1]))
        --q.length;
    return q;
}
static unsigned cui_command_scalar_properties(uint32_t scalar) {
    if (scalar < 128)
        return cui_command_ascii_properties[scalar];
    size_t lo = 0, hi = sizeof cui_command_properties / sizeof cui_command_properties[0];
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (scalar < cui_command_properties[mid].first)
            hi = mid;
        else if (scalar > cui_command_properties[mid].last)
            lo = mid + 1;
        else
            return cui_command_properties[mid].flags;
    }
    return 0;
}
static cui_str cui_command_trim(cui_str q) {
    size_t width;
    uint32_t scalar;
    while (cui_utf8_decode((const unsigned char *)q.data, q.length, &width, &scalar) &&
           (cui_command_scalar_properties(scalar) & CUI_COMMAND_SPACE)) {
        q.data += width;
        q.length -= width;
    }
    while (q.length) {
        size_t at = q.length - 1;
        while (at && q.length - at < 4 && ((unsigned char)q.data[at] & 0xc0u) == 0x80u)
            --at;
        if (!cui_utf8_decode((const unsigned char *)q.data + at, q.length - at, &width, &scalar) ||
            width != q.length - at || !(cui_command_scalar_properties(scalar) & CUI_COMMAND_SPACE))
            break;
        q.length = at;
    }
    return q;
}
static uint32_t cui_command_ascii_lower(uint32_t scalar) {
    return scalar >= 'A' && scalar <= 'Z' ? scalar + ('a' - 'A') : scalar;
}
static uint32_t cui_command_simple_lower(uint32_t scalar) {
    if (scalar < 128)
        return cui_command_ascii_lower(scalar);
    size_t lo = 0, hi = sizeof cui_command_lowercase / sizeof cui_command_lowercase[0];
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (scalar < cui_command_lowercase[mid].first)
            hi = mid;
        else if (scalar > cui_command_lowercase[mid].last)
            lo = mid + 1;
        else if ((scalar - cui_command_lowercase[mid].first) % cui_command_lowercase[mid].step == 0)
            return (uint32_t)((int32_t)scalar + cui_command_lowercase[mid].delta);
        else
            break;
    }
    return scalar;
}
static uint32_t cui_command_take_scalar(cui_str *s) {
    const unsigned char *data = (const unsigned char *)s->data;
    size_t width;
    uint32_t scalar;
    if (!cui_utf8_decode(data, s->length, &width, &scalar)) {
        /* Keep malformed bytes distinct from valid scalars and from each other. */
        scalar = 0x110000u + data[0];
        width = 1;
    }
    s->data += width;
    s->length -= width;
    return scalar;
}

typedef struct {
    cui_str remaining;
    uint32_t pending;
    bool preceded_by_cased;
} cui_command_lowercase_cursor;

static bool cui_command_cased_after(cui_str remaining) {
    while (remaining.length) {
        unsigned flags = cui_command_scalar_properties(cui_command_take_scalar(&remaining));
        if (!(flags & CUI_COMMAND_CASE_IGNORABLE))
            return (flags & CUI_COMMAND_CASED) != 0;
    }
    return false;
}
static uint32_t cui_command_lower_next(cui_command_lowercase_cursor *cursor) {
    if (cursor->pending) {
        uint32_t scalar = cursor->pending;
        cursor->pending = 0;
        return scalar;
    }
    uint32_t scalar = cui_command_take_scalar(&cursor->remaining);
    unsigned props = cui_command_scalar_properties(scalar);
    bool final_sigma =
        scalar == 0x3a3 && cursor->preceded_by_cased && !cui_command_cased_after(cursor->remaining);
    if (!(props & CUI_COMMAND_CASE_IGNORABLE))
        cursor->preceded_by_cased = (props & CUI_COMMAND_CASED) != 0;
    if (final_sigma)
        return 0x3c2;
    if (scalar == 0x130) {
        cursor->pending = 0x307;
        return 'i';
    }
    return cui_command_simple_lower(scalar);
}
static bool cui_command_lower_has_next(cui_command_lowercase_cursor cursor) {
    return cursor.remaining.length || cursor.pending;
}
static bool cui_command_is_ascii(cui_str text) {
    uint64_t block;
    while (text.length >= sizeof block) {
        memcpy(&block, text.data, sizeof block);
        if (block & UINT64_C(0x8080808080808080))
            return false;
        text.data += sizeof block;
        text.length -= sizeof block;
    }
    for (size_t at = 0; at < text.length; ++at)
        if ((unsigned char)text.data[at] >= 128)
            return false;
    return true;
}
static bool cui_command_contains_ascii(cui_str s, cui_str q) {
    if (s.length < q.length)
        return false;
    uint32_t first = cui_command_ascii_lower((unsigned char)q.data[0]);
    for (size_t at = 0; at <= s.length - q.length; ++at) {
        if (cui_command_ascii_lower((unsigned char)s.data[at]) != first)
            continue;
        size_t j = 1;
        while (j < q.length && cui_command_ascii_lower((unsigned char)s.data[at + j]) ==
                                   cui_command_ascii_lower((unsigned char)q.data[j]))
            ++j;
        if (j == q.length)
            return true;
    }
    return false;
}
static bool cui_command_contains(cui_str s, cui_str q, bool ascii_query) {
    if (!q.length)
        return true;
    if (!cui_command_str_valid(s))
        return false;
    if (ascii_query && cui_command_is_ascii(s))
        return cui_command_contains_ascii(s, q);
    /* Copy cursors, so each candidate keeps its full-string casing context and may start
       within a lowercase expansion. No proportional storage or NUL terminators are required. */
    cui_command_lowercase_cursor needle = {.remaining = q};
    uint32_t first = cui_command_lower_next(&needle);
    for (cui_command_lowercase_cursor text = {.remaining = s}; cui_command_lower_has_next(text);) {
        if (cui_command_lower_next(&text) != first)
            continue;
        cui_command_lowercase_cursor rest = text, query = needle;
        for (;;) {
            if (!cui_command_lower_has_next(query))
                return true;
            if (!cui_command_lower_has_next(rest) ||
                cui_command_lower_next(&query) != cui_command_lower_next(&rest))
                break;
        }
    }
    return false;
}
static bool cui_command_matches_trimmed(const cui_command_item *i, cui_str q) {
    bool ascii = cui_command_is_ascii(q);
    if (cui_command_contains(i->label, q, ascii))
        return true;
    if (i->keyword_count && !i->keywords)
        return false;
    for (size_t k = 0; k < i->keyword_count; ++k)
        if (cui_command_contains(i->keywords[k], q, ascii))
            return true;
    return false;
}
bool cui_command_matches(const cui_command_item *i, cui_str q) {
    return i && cui_command_str_valid(q) && cui_command_matches_trimmed(i, cui_command_trim(q));
}
cui_command_options cui_command_default(void) {
    return (cui_command_options){
        .searchable = true, .filterable = true, .bordered = true, .width = 380, .max_height = 300};
}
bool cui_command_init(cui_command_state *s, cui_command_row *rows, size_t n, cui_edit *q) {
    if (!s || !rows || !n || n > CUI_COMMAND_LIMIT)
        return false;
    *s = (cui_command_state){.rows = rows, .capacity = n, .query = q, .selected = SIZE_MAX};
    return true;
}
bool cui_command_index_equal(cui_command_index a, cui_command_index b) {
    return a.section == b.section && a.row == b.row;
}
bool cui_command_selected(const cui_command_state *s, cui_command_index *out) {
    if (!s || s->selected >= s->row_count || s->rows[s->selected].kind != CUI_COMMAND_ROW_ITEM)
        return false;
    if (out)
        *out = s->rows[s->selected].index;
    return true;
}
static bool cui_command_item_valid(const cui_command_item *i) {
    if (!cui_command_str_valid(i->label) || !cui_command_str_valid(i->icon) ||
        i->keyword_count > CUI_COMMAND_LIMIT || (i->keyword_count && !i->keywords))
        return false;
    for (size_t n = 0; n < i->keyword_count; ++n)
        if (!cui_command_str_valid(i->keywords[n]))
            return false;
    return true;
}
static bool cui_command_match(const cui_command_item *i, cui_str q, cui_command_options o) {
    return !o.searchable || !o.filterable ||
           (o.matches ? o.matches(i, q, o.match_user) : cui_command_matches_trimmed(i, q));
}
static cui_component_event_metadata
cui_command_events_private_metadata(const cui_command_events_private_observer *observer,
                                    cui_component_event_kind kind) {
    cui_component_event_metadata event = {0};
    if (observer->has_cause) {
        event = observer->cause;
        event.kind = kind;
    } else
        (void)cui_component_event_init(&event, kind, observer->target,
                                       CUI_COMPONENT_EVENT_SOURCE_PROGRAMMATIC);
    return event;
}
static void cui_command_events_private_count(size_t *count, bool *overflow) {
    if (*count == SIZE_MAX)
        *overflow = true;
    else
        ++*count;
}
static void
cui_command_events_private_notify_selection(cui_command_events_private_observer *observer,
                                            const cui_command_selection_change *change) {
    if (!observer || observer->suppress || !observer->handler.on_selection)
        return;
    cui_command_events_private_count(&observer->result.selection_count,
                                     &observer->result.count_overflow);
    observer->handler.on_selection(observer->handler.user, change);
}
bool cui_command_events_private_rebuild(cui_command_state *s, const cui_command_entry *entries,
                                        size_t n, cui_str q, bool changed, cui_command_options o,
                                        cui_command_events_private_observer *observer) {
    if (!s || !s->rows || !s->capacity || s->capacity > CUI_COMMAND_LIMIT ||
        n > CUI_COMMAND_LIMIT || (n && !entries) || !cui_command_str_valid(q))
        return false;
    size_t required = 0, total = 0;
    bool ungrouped = false;
    /* Reserve worst-case rows before writing, including filtered entries: strict bounded contract.
     */
    for (size_t e = 0; e < n; ++e) {
        const cui_command_entry *v = &entries[e];
        if (v->kind == CUI_COMMAND_ITEM) {
            if (!cui_command_item_valid(&v->item))
                return false;
            ++required;
            ++total;
            ungrouped = true;
        } else if (v->kind == CUI_COMMAND_GROUP) {
            if (v->item_count > CUI_COMMAND_LIMIT || (v->item_count && !v->items) ||
                !cui_command_str_valid(v->heading))
                return false;
            if (required > CUI_COMMAND_LIMIT - v->item_count)
                return false;
            required += v->item_count + (v->heading.length ? 1 : 0);
            total += v->item_count;
            for (size_t i = 0; i < v->item_count; ++i)
                if (!cui_command_item_valid(&v->items[i]))
                    return false;
        } else if (v->kind == CUI_COMMAND_SEPARATOR)
            ++required;
        else
            return false;
        if (required > s->capacity || total > CUI_COMMAND_LIMIT)
            return false;
    }
    cui_command_index old = {0};
    bool had = cui_command_selected(s, &old);
    size_t count = 0, matched = 0, group = ungrouped ? 1 : 0, item = 0;
    bool separator = false;
    cui_str raw_query = q;
    q = o.matches ? cui_command_trim_custom(q) : cui_command_trim(q);
    for (size_t e = 0; e < n; ++e) {
        const cui_command_entry *v = &entries[e];
        if (v->kind == CUI_COMMAND_SEPARATOR) {
            separator = count > 0;
            continue;
        }
        size_t section = v->kind == CUI_COMMAND_ITEM ? 0 : group++;
        size_t length = v->kind == CUI_COMMAND_ITEM ? 1 : v->item_count;
        bool heading = false;
        for (size_t i = 0; i < length; ++i) {
            const cui_command_item *it = v->kind == CUI_COMMAND_ITEM ? &v->item : &v->items[i];
            size_t row = v->kind == CUI_COMMAND_ITEM ? item++ : i;
            if (!cui_command_match(it, q, o))
                continue;
            if (separator) {
                s->rows[count++] = (cui_command_row){.kind = CUI_COMMAND_ROW_SEPARATOR};
                separator = false;
            }
            if (v->kind == CUI_COMMAND_GROUP && !heading) {
                heading = true;
                if (v->heading.length)
                    s->rows[count++] =
                        (cui_command_row){.kind = CUI_COMMAND_ROW_HEADING, .heading = v->heading};
            }
            s->rows[count++] = (cui_command_row){
                .kind = CUI_COMMAND_ROW_ITEM, .index = {section, row}, .item = it};
            ++matched;
        }
    }
    s->row_count = count;
    s->matched_count = matched;
    s->selected = SIZE_MAX;
    if (!changed && had)
        for (size_t i = 0; i < count; ++i)
            if (s->rows[i].item && !s->rows[i].item->disabled &&
                cui_command_index_equal(old, s->rows[i].index)) {
                s->selected = i;
                break;
            }
    if (s->selected == SIZE_MAX && (changed || !s->preserve_no_selection)) {
        for (size_t i = 0; i < count; ++i)
            if (s->rows[i].item && !s->rows[i].item->disabled) {
                s->selected = i;
                break;
            }
        s->pending_scroll = true;
    }
    if (changed)
        s->preserve_no_selection = false;
    s->initialized = true;
    cui_command_index now = {0};
    bool selection_changed =
        changed && cui_command_selected(s, &now) && (!had || !cui_command_index_equal(old, now));
    cui_command_selection_change selection = {0};
    if (observer) {
        selection.event = cui_command_events_private_metadata(observer, CUI_COMPONENT_EVENT_CHANGE);
        selection.has_previous = had;
        selection.previous = old;
        selection.value = now;
        /* Borrowed query must be observed before either legacy hook can replace it. */
        if (changed && o.searchable && !observer->suppress && observer->handler.on_query) {
            cui_command_query_change query = {
                cui_command_events_private_metadata(observer, CUI_COMPONENT_EVENT_CHANGE),
                raw_query};
            cui_command_events_private_count(&observer->result.query_count,
                                             &observer->result.count_overflow);
            observer->handler.on_query(observer->handler.user, &query);
        }
    }
    if (selection_changed) {
        if (o.on_select)
            o.on_select(now, o.user);
        cui_command_events_private_notify_selection(observer, &selection);
    }
    if (changed && o.searchable && o.on_query)
        o.on_query(raw_query, o.user);
    return true;
}
bool cui_command_events_private_select(cui_command_state *s, cui_command_index index, bool scroll,
                                       cui_command_options o,
                                       cui_command_events_private_observer *observer) {
    if (!s)
        return false;
    for (size_t i = 0; i < s->row_count; ++i)
        if (s->rows[i].item && !s->rows[i].item->disabled &&
            cui_command_index_equal(index, s->rows[i].index)) {
            if (s->selected == i)
                return false;
            cui_command_selection_change change = {0};
            if (observer) {
                change.event =
                    cui_command_events_private_metadata(observer, CUI_COMPONENT_EVENT_CHANGE);
                change.has_previous = cui_command_selected(s, &change.previous);
                change.value = index;
            }
            s->selected = i;
            s->preserve_no_selection = false;
            s->pending_scroll = scroll;
            if (o.on_select)
                o.on_select(index, o.user);
            cui_command_events_private_notify_selection(observer, &change);
            return true;
        }
    return false;
}
void cui_command_clear_selection(cui_command_state *s) {
    if (s) {
        s->selected = SIZE_MAX;
        s->preserve_no_selection = true;
        s->pending_scroll = false;
    }
}
bool cui_command_events_private_move(cui_command_state *s, bool forward, cui_command_options o,
                                     cui_command_events_private_observer *observer) {
    if (!s || !s->row_count)
        return false;
    size_t i = s->selected < s->row_count ? s->selected : forward ? s->row_count - 1 : 0;
    for (size_t k = 0; k < s->row_count; ++k) {
        i = forward ? (i + 1) % s->row_count : (i + s->row_count - 1) % s->row_count;
        if (s->rows[i].item && !s->rows[i].item->disabled)
            return cui_command_events_private_select(s, s->rows[i].index, true, o, observer);
    }
    return false;
}
bool cui_command_events_private_confirm(cui_command_state *s, cui_command_options o,
                                        cui_command_events_private_observer *observer) {
    if (!s || o.disabled || s->selected >= s->row_count)
        return false;
    cui_command_row row = s->rows[s->selected];
    if (!row.item || row.item->disabled)
        return false;
    void (*confirm)(cui_command_index, void *) = o.on_confirm;
    void *user = o.user;
    cui_command_activation activation = {0};
    if (observer) {
        activation.event =
            cui_command_events_private_metadata(observer, CUI_COMPONENT_EVENT_ACTIVATE);
        activation.value = row.index;
    }
    if (row.item->action)
        row.item->action(row.item->action_user);
    if (confirm)
        confirm(row.index, user);
    if (observer && !observer->suppress && observer->handler.on_activate) {
        cui_command_events_private_count(&observer->result.activation_count,
                                         &observer->result.count_overflow);
        observer->handler.on_activate(observer->handler.user, &activation);
    }
    return true;
}

bool cui_command_rebuild(cui_command_state *s, const cui_command_entry *entries, size_t n,
                         cui_str query, bool changed, cui_command_options options) {
    return cui_command_events_private_rebuild(s, entries, n, query, changed, options, NULL);
}
bool cui_command_select(cui_command_state *s, cui_command_index index, bool scroll,
                        cui_command_options options) {
    return cui_command_events_private_select(s, index, scroll, options, NULL);
}
bool cui_command_move(cui_command_state *s, bool forward, cui_command_options options) {
    return cui_command_events_private_move(s, forward, options, NULL);
}
bool cui_command_confirm(cui_command_state *s, cui_command_options options) {
    return cui_command_events_private_confirm(s, options, NULL);
}
cui_command_event_result cui_command_rebuild_events(cui_command_state *s, cui_id target,
                                                    const cui_command_entry *entries, size_t n,
                                                    cui_str query, bool changed,
                                                    cui_command_options options,
                                                    cui_command_event_handler handler) {
    cui_command_events_private_observer observer = {.target = target, .handler = handler};
    if (target)
        observer.result.control =
            cui_command_events_private_rebuild(s, entries, n, query, changed, options, &observer);
    return observer.result;
}
cui_command_event_result cui_command_select_events(cui_command_state *s, cui_id target,
                                                   cui_command_index index, bool scroll,
                                                   cui_command_options options,
                                                   cui_command_event_handler handler) {
    cui_command_events_private_observer observer = {.target = target, .handler = handler};
    if (target)
        observer.result.control =
            cui_command_events_private_select(s, index, scroll, options, &observer);
    return observer.result;
}
cui_command_event_result cui_command_move_events(cui_command_state *s, cui_id target, bool forward,
                                                 cui_command_options options,
                                                 cui_command_event_handler handler) {
    cui_command_events_private_observer observer = {.target = target, .handler = handler};
    if (target)
        observer.result.control = cui_command_events_private_move(s, forward, options, &observer);
    return observer.result;
}
cui_command_event_result cui_command_confirm_events(cui_command_state *s, cui_id target,
                                                    cui_command_options options,
                                                    cui_command_event_handler handler) {
    cui_command_events_private_observer observer = {.target = target, .handler = handler};
    if (target)
        observer.result.control = cui_command_events_private_confirm(s, options, &observer);
    return observer.result;
}
