/* SPDX-License-Identifier: MIT */
#include "cui/cui_text_view.h"
#include <math.h>
#include <string.h>
static bool cui_text_view_boundary(cui_str t, size_t p) {
    return p <= t.length && (p == t.length || !((unsigned char)t.data[p] >= 0x80 &&
                                                (unsigned char)t.data[p] < 0xc0));
}
static size_t cui_text_view_scalar(cui_str s, size_t i) {
    if (i >= s.length)
        return 0;
    unsigned c = (unsigned char)s.data[i];
    size_t n = c < 0x80                 ? 1
               : c >= 0xc2 && c <= 0xdf ? 2
               : c >= 0xe0 && c <= 0xef ? 3
               : c >= 0xf0 && c <= 0xf4 ? 4
                                        : 0;
    if (!n || n > s.length - i)
        return 0;
    for (size_t k = 1; k < n; ++k)
        if (((unsigned char)s.data[i + k] & 0xc0) != 0x80)
            return 0;
    if (n >= 3 && ((c == 0xe0 && (unsigned char)s.data[i + 1] < 0xa0) ||
                   (c == 0xed && (unsigned char)s.data[i + 1] >= 0xa0)))
        return 0;
    if (n == 4 && ((c == 0xf0 && (unsigned char)s.data[i + 1] < 0x90) ||
                   (c == 0xf4 && (unsigned char)s.data[i + 1] >= 0x90)))
        return 0;
    return n;
}
static bool cui_text_view_valid(cui_str s) {
    if (s.length && !s.data)
        return false;
    for (size_t i = 0; i < s.length;) {
        size_t n = cui_text_view_scalar(s, i);
        if (!n || s.data[i] == 0)
            return false;
        i += n;
    }
    return true;
}
static bool cui_text_view_range_valid(cui_str s, cui_text_view_range r) {
    return r.begin <= r.end && cui_text_view_boundary(s, r.begin) &&
           cui_text_view_boundary(s, r.end);
}
bool cui_text_view_init(cui_text_view_state *s, cui_text_view_workspace w, cui_text_view_format f) {
    if (!s || f < CUI_TEXT_VIEW_MARKDOWN || f > CUI_TEXT_VIEW_PLAIN || !w.source || !w.rendered ||
        !w.source_capacity || !w.rendered_capacity ||
        w.source_capacity > CUI_TEXT_VIEW_BYTE_LIMIT ||
        w.rendered_capacity > CUI_TEXT_VIEW_BYTE_LIMIT || !w.mapping ||
        w.mapping_capacity < w.rendered_capacity || !w.spans || !w.span_capacity ||
        w.span_capacity > CUI_TEXT_VIEW_NODE_LIMIT || !w.blocks || !w.block_capacity ||
        w.block_capacity > CUI_TEXT_VIEW_NODE_LIMIT || !w.glyphs || !w.glyph_capacity ||
        w.glyph_capacity > CUI_TEXT_VIEW_BYTE_LIMIT || (w.highlight_capacity && !w.highlights))
        return false;
    *s = (cui_text_view_state){.workspace = w, .format = f, .fade_duration = .35f};
    w.source[0] = w.rendered[0] = 0;
    return true;
}
cui_str cui_text_view_source(const cui_text_view_state *s) {
    return s ? (cui_str){s->workspace.source, s->source_length} : (cui_str){0};
}
cui_str cui_text_view_rendered_text(const cui_text_view_state *s) {
    return s ? (cui_str){s->workspace.rendered, s->text_length} : (cui_str){0};
}
typedef struct {
    cui_text_view_state *state;
    cui_str input;
    bool write, fade;
    size_t text, spans, blocks, block;
    cui_text_view_span last;
    cui_text_view_error error;
} cui_text_view_parser;
static void cui_text_view_emit(cui_text_view_parser *p, cui_str text, cui_text_view_range source,
                               unsigned flags, cui_text_view_range link) {
    cui_text_view_workspace *w = &p->state->workspace;
    if (p->error)
        return;
    if (text.length > w->rendered_capacity - 1 - p->text) {
        p->error = CUI_TEXT_VIEW_CAPACITY;
        return;
    }
    bool mapped = source.begin != SIZE_MAX;
    if (mapped && text.length) {
        bool same = p->spans && p->last.block == p->block && p->last.flags == flags &&
                    p->last.link.begin == link.begin && p->last.link.end == link.end &&
                    p->last.text.end == p->text;
        if (!same) {
            if (p->spans >= w->span_capacity) {
                p->error = CUI_TEXT_VIEW_CAPACITY;
                return;
            }
            p->last = (cui_text_view_span){.text = {p->text, p->text + text.length},
                                           .source = source,
                                           .link = link,
                                           .flags = flags,
                                           .block = p->block};
            ++p->spans;
        } else {
            p->last.text.end = p->text + text.length;
            p->last.source.end = source.end;
        }
        if (p->write)
            w->spans[p->spans - 1] = p->last;
    }
    for (size_t i = 0; i < text.length; ++i) {
        if (p->write) {
            float born = -1;
            if (p->text < p->state->text_length && w->rendered[p->text] == text.data[i] &&
                w->mapping[p->text].flags == flags)
                born = w->mapping[p->text].born;
            else if (p->fade)
                born = p->state->time;
            w->rendered[p->text] = text.data[i];
            w->mapping[p->text] =
                (cui_text_view_mapping){.source = source, .flags = flags, .born = born};
        }
        ++p->text;
    }
}
static void cui_text_view_gap(cui_text_view_parser *p, char c) {
    cui_text_view_emit(p, (cui_str){&c, 1}, (cui_text_view_range){SIZE_MAX, SIZE_MAX}, 0,
                       (cui_text_view_range){0});
}
static bool cui_text_view_block_begin(cui_text_view_parser *p, cui_text_view_block_kind kind,
                                      size_t start, unsigned level) {
    if (p->blocks >= p->state->workspace.block_capacity) {
        p->error = CUI_TEXT_VIEW_CAPACITY;
        return false;
    }
    p->block = p->blocks++;
    if (p->write)
        p->state->workspace.blocks[p->block] = (cui_text_view_block){
            .kind = kind, .text = {p->text, p->text}, .source = {start, start}, .level = level};
    p->last.text.end = SIZE_MAX;
    return true;
}
static void cui_text_view_block_end(cui_text_view_parser *p, size_t end, bool newline) {
    if (p->write && p->block < p->blocks) {
        p->state->workspace.blocks[p->block].text.end = p->text;
        p->state->workspace.blocks[p->block].source.end = end;
    }
    if (newline)
        cui_text_view_gap(p, '\n');
}
static size_t cui_text_view_find(cui_str s, size_t begin, size_t end, const char *needle,
                                 size_t n) {
    if (n > end - begin)
        return SIZE_MAX;
    for (size_t i = begin; i <= end - n; ++i)
        if (!memcmp(s.data + i, needle, n))
            return i;
    return SIZE_MAX;
}
static unsigned cui_text_view_encode(unsigned c, char out[4]) {
    if (c < 0x80) {
        out[0] = (char)c;
        return 1;
    }
    if (c < 0x800) {
        out[0] = (char)(0xc0 | (c >> 6));
        out[1] = (char)(0x80 | (c & 63));
        return 2;
    }
    if (c < 0x10000) {
        out[0] = (char)(0xe0 | (c >> 12));
        out[1] = (char)(0x80 | ((c >> 6) & 63));
        out[2] = (char)(0x80 | (c & 63));
        return 3;
    }
    out[0] = (char)(0xf0 | (c >> 18));
    out[1] = (char)(0x80 | ((c >> 12) & 63));
    out[2] = (char)(0x80 | ((c >> 6) & 63));
    out[3] = (char)(0x80 | (c & 63));
    return 4;
}
static bool cui_text_view_entity(cui_text_view_parser *p, size_t *at, size_t end, unsigned flags,
                                 cui_text_view_range link) {
    size_t a = *at, b = cui_text_view_find(p->input, a, end, ";", 1);
    if (b == SIZE_MAX || b - a > 16)
        return false;
    cui_str name = {p->input.data + a + 1, b - a - 1};
    unsigned c = 0;
    if (name.length == 3 && !memcmp(name.data, "amp", 3))
        c = '&';
    else if (name.length == 2 && !memcmp(name.data, "lt", 2))
        c = '<';
    else if (name.length == 2 && !memcmp(name.data, "gt", 2))
        c = '>';
    else if (name.length == 4 && !memcmp(name.data, "quot", 4))
        c = '"';
    else if (name.length == 4 && !memcmp(name.data, "apos", 4))
        c = '\'';
    else if (name.length == 4 && !memcmp(name.data, "nbsp", 4))
        c = 0xa0;
    else if (name.length > 1 && name.data[0] == '#') {
        size_t i = 1;
        unsigned base = 10;
        if (i < name.length && (name.data[i] == 'x' || name.data[i] == 'X')) {
            base = 16;
            ++i;
        }
        if (i == name.length)
            return false;
        for (; i < name.length; ++i) {
            unsigned char ch = (unsigned char)name.data[i];
            unsigned d = ch >= '0' && ch <= '9'   ? ch - '0'
                         : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
                         : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10
                                                  : 99;
            if (d >= base || c > (0x10ffff - d) / base)
                return false;
            c = c * base + d;
        }
    }
    if (!c || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
        return false;
    char encoded[4];
    unsigned n = cui_text_view_encode(c, encoded);
    cui_text_view_emit(p, (cui_str){encoded, n}, (cui_text_view_range){a, b + 1}, flags, link);
    *at = b + 1;
    return true;
}
static void cui_text_view_inline(cui_text_view_parser *p, size_t a, size_t b, unsigned flags,
                                 cui_text_view_range link, unsigned depth) {
    if (depth > 16) {
        p->error = CUI_TEXT_VIEW_INVALID;
        return;
    }
    while (a < b && !p->error) {
        char c = p->input.data[a];
        if (c == '\\' && a + 1 < b) {
            size_t n = cui_text_view_scalar(p->input, a + 1);
            cui_text_view_emit(p, (cui_str){p->input.data + a + 1, n},
                               (cui_text_view_range){a, a + 1 + n}, flags, link);
            a += 1 + n;
            continue;
        }
        if (c == '&' && cui_text_view_entity(p, &a, b, flags, link))
            continue;
        if (c == '`') {
            size_t close = cui_text_view_find(p->input, a + 1, b, "`", 1);
            if (close != SIZE_MAX) {
                for (size_t i = a + 1; i < close;) {
                    size_t n = cui_text_view_scalar(p->input, i);
                    cui_text_view_emit(p, (cui_str){p->input.data + i, n},
                                       (cui_text_view_range){i, i + n}, flags | CUI_TEXT_VIEW_CODE,
                                       link);
                    i += n;
                }
                a = close + 1;
                continue;
            }
        }
        if (c == '[') {
            size_t close = cui_text_view_find(p->input, a + 1, b, "](", 2);
            if (close != SIZE_MAX) {
                size_t dest = close + 2, last = cui_text_view_find(p->input, dest, b, ")", 1);
                if (last != SIZE_MAX) {
                    size_t uri = dest;
                    while (uri < last && p->input.data[uri] != ' ' && p->input.data[uri] != '\t')
                        ++uri;
                    cui_text_view_inline(p, a + 1, close, flags, (cui_text_view_range){dest, uri},
                                         depth + 1);
                    a = last + 1;
                    continue;
                }
            }
        }
        if (c == '*' || c == '_' || c == '~') {
            size_t n = a + 1 < b && p->input.data[a + 1] == c ? 2 : 1;
            unsigned mark = n == 2 ? (c == '~' ? CUI_TEXT_VIEW_STRIKE : CUI_TEXT_VIEW_BOLD)
                                   : CUI_TEXT_VIEW_ITALIC;
            if (c != '~' || n == 2) {
                size_t close = cui_text_view_find(p->input, a + n, b, p->input.data + a, n);
                if (close != SIZE_MAX && close > a + n) {
                    cui_text_view_inline(p, a + n, close, flags | mark, link, depth + 1);
                    a = close + n;
                    continue;
                }
            }
        }
        size_t n = cui_text_view_scalar(p->input, a);
        cui_text_view_emit(p, (cui_str){p->input.data + a, n}, (cui_text_view_range){a, a + n},
                           flags, link);
        a += n;
    }
}
static size_t cui_text_view_line_end(cui_str s, size_t a) {
    while (a < s.length && s.data[a] != '\n' && s.data[a] != '\r')
        ++a;
    return a;
}
static size_t cui_text_view_next_line(cui_str s, size_t a) {
    if (a < s.length && s.data[a] == '\r')
        ++a;
    if (a < s.length && s.data[a] == '\n')
        ++a;
    return a;
}
static bool cui_text_view_table_separator(cui_str s, size_t a, size_t b) {
    bool dash = false;
    for (size_t i = a; i < b; ++i) {
        char c = s.data[i];
        if (c == '-')
            dash = true;
        else if (c != '|' && c != ':' && c != ' ' && c != '\t')
            return false;
    }
    return dash;
}
static cui_text_view_alignment cui_text_view_table_alignment(cui_str s, size_t a, size_t b,
                                                             unsigned col) {
    while (a < b && s.data[a] == ' ')
        ++a;
    if (a < b && s.data[a] == '|')
        ++a;
    for (unsigned c = 0; a < b; ++c) {
        size_t end = cui_text_view_find(s, a, b, "|", 1);
        if (end == SIZE_MAX)
            end = b;
        if (c == col) {
            while (a < end && s.data[a] == ' ')
                ++a;
            while (end > a && s.data[end - 1] == ' ')
                --end;
            bool first = a < end && s.data[a] == ':', last = end > a && s.data[end - 1] == ':';
            return last ? first ? CUI_TEXT_VIEW_ALIGN_CENTER : CUI_TEXT_VIEW_ALIGN_RIGHT
                        : CUI_TEXT_VIEW_ALIGN_LEFT;
        }
        a = end < b ? end + 1 : b;
    }
    return CUI_TEXT_VIEW_ALIGN_LEFT;
}
static bool cui_text_view_fence_close(cui_str input, size_t a, size_t b, char marker,
                                      size_t length) {
    unsigned indent = 0;
    while (a < b && input.data[a] == ' ' && indent < 3) {
        ++a;
        ++indent;
    }
    size_t start = a;
    while (a < b && input.data[a] == marker)
        ++a;
    if (a - start < length)
        return false;
    while (a < b && (input.data[a] == ' ' || input.data[a] == '\t'))
        ++a;
    return a == b;
}
static void cui_text_view_markdown(cui_text_view_parser *p) {
    size_t a = 0;
    unsigned table = 0, list_number = 0;
    bool prior_ordered = false;
    while (a < p->input.length && !p->error) {
        size_t b = cui_text_view_line_end(p->input, a), next = cui_text_view_next_line(p->input, b),
               start = a;
        while (a < b && (p->input.data[a] == ' ' || p->input.data[a] == '\t'))
            ++a;
        if (a == b) {
            a = next;
            continue;
        }
        if (b - a >= 3 &&
            (!memcmp(p->input.data + a, "```", 3) || !memcmp(p->input.data + a, "~~~", 3))) {
            prior_ordered = false;
            char fence = p->input.data[a];
            size_t fence_length = 3;
            while (a + fence_length < b && p->input.data[a + fence_length] == fence)
                ++fence_length;
            size_t lang = a + fence_length, lang_end = b;
            while (lang < lang_end && p->input.data[lang] == ' ')
                ++lang;
            lang_end = lang;
            while (lang_end < b && p->input.data[lang_end] != ' ' &&
                   p->input.data[lang_end] != '\t')
                ++lang_end;
            if (!cui_text_view_block_begin(p, CUI_TEXT_VIEW_CODE_BLOCK, start, 0))
                return;
            if (p->write)
                p->state->workspace.blocks[p->block].resource =
                    (cui_text_view_range){lang, lang_end};
            size_t i = next, end = i;
            while (i < p->input.length) {
                size_t e = cui_text_view_line_end(p->input, i);
                if (cui_text_view_fence_close(p->input, i, e, fence, fence_length)) {
                    end = cui_text_view_next_line(p->input, e);
                    break;
                }
                for (size_t j = i; j < e;) {
                    size_t n = cui_text_view_scalar(p->input, j);
                    cui_text_view_emit(p, (cui_str){p->input.data + j, n},
                                       (cui_text_view_range){j, j + n}, CUI_TEXT_VIEW_CODE,
                                       (cui_text_view_range){0});
                    j += n;
                }
                size_t following = cui_text_view_next_line(p->input, e);
                if (following < p->input.length &&
                    !cui_text_view_fence_close(p->input, following,
                                               cui_text_view_line_end(p->input, following), fence,
                                               fence_length))
                    cui_text_view_emit(
                        p, (cui_str){"\n", 1},
                        (cui_text_view_range){e, cui_text_view_next_line(p->input, e)},
                        CUI_TEXT_VIEW_CODE, (cui_text_view_range){0});
                i = cui_text_view_next_line(p->input, e);
                end = i;
            }
            cui_text_view_block_end(p, end, true);
            a = end;
            continue;
        }
        size_t next_end = cui_text_view_line_end(p->input, next);
        if (cui_text_view_find(p->input, a, b, "|", 1) != SIZE_MAX && next < p->input.length &&
            cui_text_view_table_separator(p->input, next, next_end)) {
            prior_ordered = false;
            ++table;
            unsigned row = 0;
            size_t line = start, line_end = b;
            do {
                size_t cell = line;
                while (cell < line_end && p->input.data[cell] == ' ')
                    ++cell;
                if (cell < line_end && p->input.data[cell] == '|')
                    ++cell;
                unsigned col = 0;
                while (cell < line_end) {
                    size_t end = cui_text_view_find(p->input, cell, line_end, "|", 1);
                    if (end == SIZE_MAX)
                        end = line_end;
                    size_t l = cell, r = end;
                    while (l < r && p->input.data[l] == ' ')
                        ++l;
                    while (r > l && p->input.data[r - 1] == ' ')
                        --r;
                    if (!cui_text_view_block_begin(p, CUI_TEXT_VIEW_TABLE_CELL, cell, 0))
                        return;
                    if (p->write) {
                        cui_text_view_block *block = &p->state->workspace.blocks[p->block];
                        block->table = table;
                        block->row = row;
                        block->column = col;
                        block->table_header = row == 0;
                        block->alignment =
                            cui_text_view_table_alignment(p->input, next, next_end, col);
                    }
                    cui_text_view_inline(p, l, r, 0, (cui_text_view_range){0}, 0);
                    cui_text_view_block_end(p, end, false);
                    ++col;
                    if (end < line_end && end + 1 < line_end)
                        cui_text_view_gap(p, ' ');
                    cell = end < line_end ? end + 1 : line_end;
                }
                cui_text_view_gap(p, '\n');
                ++row;
                line = cui_text_view_next_line(p->input, line_end);
                if (row == 1)
                    line = cui_text_view_next_line(p->input, next_end);
                line_end = cui_text_view_line_end(p->input, line);
            } while (line < p->input.length &&
                     cui_text_view_find(p->input, line, line_end, "|", 1) != SIZE_MAX);
            a = line;
            continue;
        }
        cui_text_view_range marker = {0};
        unsigned level = 0;
        while (a + level < b && p->input.data[a + level] == '#' && level < 6)
            ++level;
        cui_text_view_block_kind kind = CUI_TEXT_VIEW_PARAGRAPH;
        if (level && a + level < b && p->input.data[a + level] == ' ') {
            kind = CUI_TEXT_VIEW_HEADING;
            a += level + 1;
        } else {
            level = 0;
            if (p->input.data[a] == '>') {
                kind = CUI_TEXT_VIEW_QUOTE;
                ++a;
                if (a < b && p->input.data[a] == ' ')
                    ++a;
            } else if (b - a >= 2 &&
                       (p->input.data[a] == '-' || p->input.data[a] == '+' ||
                        p->input.data[a] == '*') &&
                       p->input.data[a + 1] == ' ') {
                kind = CUI_TEXT_VIEW_LIST_ITEM;
                level = 0;
                a += 2;
            } else {
                size_t digit = a;
                while (digit < b && p->input.data[digit] >= '0' && p->input.data[digit] <= '9')
                    ++digit;
                if (digit > a && digit - a <= 9 && digit + 1 < b && p->input.data[digit] == '.' &&
                    p->input.data[digit + 1] == ' ') {
                    kind = CUI_TEXT_VIEW_LIST_ITEM;
                    level = 1;
                    marker = (cui_text_view_range){a, digit + 1};
                    a = digit + 2;
                }
            }
        }
        if (b - a >= 3 &&
            ((p->input.data[a] == '-' && cui_text_view_table_separator(p->input, a, b)) ||
             !memcmp(p->input.data + a, "***", 3)))
            kind = CUI_TEXT_VIEW_RULE;
        if (b - a >= 4 && p->input.data[a] == '!' && p->input.data[a + 1] == '[') {
            size_t close = cui_text_view_find(p->input, a + 2, b, "](", 2),
                   last = close == SIZE_MAX ? SIZE_MAX
                                            : cui_text_view_find(p->input, close + 2, b, ")", 1);
            if (last != SIZE_MAX) {
                prior_ordered = false;
                if (!cui_text_view_block_begin(p, CUI_TEXT_VIEW_IMAGE, start, 0))
                    return;
                if (p->write)
                    p->state->workspace.blocks[p->block].resource =
                        (cui_text_view_range){close + 2, last};
                cui_text_view_inline(p, a + 2, close, 0, (cui_text_view_range){0}, 0);
                cui_text_view_block_end(p, next, true);
                a = next;
                continue;
            }
        }
        if (!cui_text_view_block_begin(p, kind, start, level))
            return;
        if (kind == CUI_TEXT_VIEW_LIST_ITEM && level) {
            if (!prior_ordered) {
                list_number = 0;
                for (size_t i = marker.begin; i + 1 < marker.end; ++i)
                    list_number = list_number * 10 + (unsigned)(p->input.data[i] - '0');
            } else
                ++list_number;
            prior_ordered = true;
        } else
            prior_ordered = false;
        if (p->write) {
            cui_text_view_block *block = &p->state->workspace.blocks[p->block];
            block->list_marker = marker;
            block->list_number = list_number;
            if (kind == CUI_TEXT_VIEW_LIST_ITEM && level) {
                char digits[10];
                unsigned number = list_number, count = 0;
                do {
                    digits[count++] = (char)('0' + number % 10);
                    number /= 10;
                } while (number);
                for (unsigned i = 0; i < count; ++i)
                    block->marker_text[i] = digits[count - i - 1];
                block->marker_text[count] = '.';
                block->marker_text[count + 1] = 0;
            }
        }
        if (kind != CUI_TEXT_VIEW_RULE) {
            size_t inline_end = b;
            if (kind == CUI_TEXT_VIEW_PARAGRAPH && b >= a + 2 && p->input.data[b - 1] == ' ' &&
                p->input.data[b - 2] == ' ')
                inline_end -= 2;
            cui_text_view_inline(p, a, inline_end, 0, (cui_text_view_range){0}, 0);
        }
        if (kind == CUI_TEXT_VIEW_PARAGRAPH) {
            while (next < p->input.length) {
                size_t end = cui_text_view_line_end(p->input, next), first = next;
                while (first < end && (p->input.data[first] == ' ' || p->input.data[first] == '\t'))
                    ++first;
                if (first == end)
                    break;
                char lead = p->input.data[first];
                size_t digits = first;
                while (digits < end && p->input.data[digits] >= '0' && p->input.data[digits] <= '9')
                    ++digits;
                bool list = ((lead == '-' || lead == '+' || lead == '*') && first + 1 < end &&
                             p->input.data[first + 1] == ' ') ||
                            (digits > first && digits + 1 < end && p->input.data[digits] == '.' &&
                             p->input.data[digits + 1] == ' ');
                if (lead == '#' || lead == '>' || list ||
                    (end >= first + 3 && (!memcmp(p->input.data + first, "```", 3) ||
                                          !memcmp(p->input.data + first, "~~~", 3) ||
                                          !memcmp(p->input.data + first, "***", 3))) ||
                    (end >= first + 2 && !memcmp(p->input.data + first, "![", 2)))
                    break;
                size_t after = cui_text_view_next_line(p->input, end);
                if (cui_text_view_find(p->input, first, end, "|", 1) != SIZE_MAX &&
                    after < p->input.length &&
                    cui_text_view_table_separator(p->input, after,
                                                  cui_text_view_line_end(p->input, after)))
                    break;
                bool hard = b >= 2 && p->input.data[b - 1] == ' ' && p->input.data[b - 2] == ' ';
                cui_text_view_emit(p, (cui_str){hard ? "\n" : " ", 1},
                                   (cui_text_view_range){b, next}, 0, (cui_text_view_range){0});
                size_t inline_end = end;
                if (end >= first + 2 && p->input.data[end - 1] == ' ' &&
                    p->input.data[end - 2] == ' ')
                    inline_end -= 2;
                cui_text_view_inline(p, first, inline_end, 0, (cui_text_view_range){0}, 0);
                b = end;
                next = after;
            }
        }
        cui_text_view_block_end(p, next, true);
        a = next;
    }
}
static bool cui_text_view_name(cui_str s, const char *name) {
    size_t n = strlen(name);
    if (s.length != n)
        return false;
    for (size_t i = 0; i < n; ++i) {
        char c = s.data[i];
        if (c >= 'A' && c <= 'Z')
            c = (char)(c + 32);
        if (c != name[i])
            return false;
    }
    return true;
}
static cui_text_view_range cui_text_view_attribute(cui_str input, size_t a, size_t b,
                                                   const char *name) {
    while (a < b) {
        while (a < b && (input.data[a] == ' ' || input.data[a] == '\t' || input.data[a] == '/'))
            ++a;
        size_t start = a;
        while (a < b && input.data[a] != '=' && input.data[a] != ' ' && input.data[a] != '\t')
            ++a;
        size_t end = a;
        while (a < b && (input.data[a] == ' ' || input.data[a] == '\t'))
            ++a;
        if (a == b || input.data[a] != '=') {
            if (a == start)
                ++a;
            continue;
        }
        ++a;
        while (a < b && input.data[a] == ' ')
            ++a;
        char quote = a < b && (input.data[a] == '\'' || input.data[a] == '"') ? input.data[a++] : 0;
        size_t value = a;
        while (a < b &&
               (quote ? input.data[a] != quote : input.data[a] != ' ' && input.data[a] != '\t'))
            ++a;
        if (cui_text_view_name((cui_str){input.data + start, end - start}, name))
            return (cui_text_view_range){value, a};
        if (quote && a < b)
            ++a;
    }
    return (cui_text_view_range){0};
}
static void cui_text_view_html_style(unsigned *flags, unsigned *depth, unsigned flag,
                                     bool closing) {
    if (closing) {
        if (*depth)
            --*depth;
    } else
        ++*depth;
    if (*depth)
        *flags |= flag;
    else
        *flags &= ~flag;
}
static size_t cui_text_view_html_skip_raw(cui_str input, size_t start, cui_str tag) {
    for (size_t i = start; i + 2 < input.length; ++i) {
        if (input.data[i] != '<' || input.data[i + 1] != '/')
            continue;
        size_t a = i + 2, end = a;
        while (end < input.length && input.data[end] != '>' && input.data[end] != ' ' &&
               input.data[end] != '\t' && input.data[end] != '\n' && input.data[end] != '\r')
            ++end;
        if (end - a != tag.length)
            continue;
        bool equal = true;
        for (size_t j = 0; j < tag.length; ++j) {
            char x = input.data[a + j], y = tag.data[j];
            if (x >= 'A' && x <= 'Z')
                x = (char)(x + 32);
            if (y >= 'A' && y <= 'Z')
                y = (char)(y + 32);
            if (x != y) {
                equal = false;
                break;
            }
        }
        if (equal) {
            size_t close = cui_text_view_find(input, end, input.length, ">", 1);
            return close == SIZE_MAX ? input.length : close + 1;
        }
    }
    return input.length;
}
static void cui_text_view_html(cui_text_view_parser *p) {
    size_t a = 0;
    unsigned flags = 0, table = 0, row = 0, col = 0;
    unsigned bold_depth = 0, italic_depth = 0, code_depth = 0, strike_depth = 0, mark_depth = 0;
    cui_text_view_range link = {0};
    bool active = false, pre = false;
    while (a < p->input.length && !p->error) {
        if (p->input.data[a] == '<') {
            size_t end = cui_text_view_find(p->input, a + 1, p->input.length, ">", 1);
            if (end != SIZE_MAX) {
                size_t begin = a + 1;
                bool closing = begin < end && p->input.data[begin] == '/';
                if (closing)
                    ++begin;
                size_t name_end = begin;
                while (name_end < end && p->input.data[name_end] != ' ' &&
                       p->input.data[name_end] != '/' && p->input.data[name_end] != '\t')
                    ++name_end;
                cui_str name = {p->input.data + begin, name_end - begin};
                if (!closing &&
                    (cui_text_view_name(name, "script") || cui_text_view_name(name, "style"))) {
                    a = cui_text_view_html_skip_raw(p->input, end + 1, name);
                    continue;
                }
                if (cui_text_view_name(name, "strong") || cui_text_view_name(name, "b"))
                    cui_text_view_html_style(&flags, &bold_depth, CUI_TEXT_VIEW_BOLD, closing);
                else if (cui_text_view_name(name, "em") || cui_text_view_name(name, "i"))
                    cui_text_view_html_style(&flags, &italic_depth, CUI_TEXT_VIEW_ITALIC, closing);
                else if (cui_text_view_name(name, "code"))
                    cui_text_view_html_style(&flags, &code_depth, CUI_TEXT_VIEW_CODE, closing);
                else if (cui_text_view_name(name, "s") || cui_text_view_name(name, "del"))
                    cui_text_view_html_style(&flags, &strike_depth, CUI_TEXT_VIEW_STRIKE, closing);
                else if (cui_text_view_name(name, "mark"))
                    cui_text_view_html_style(&flags, &mark_depth, CUI_TEXT_VIEW_MARK, closing);
                else if (cui_text_view_name(name, "a"))
                    link = closing ? (cui_text_view_range){0}
                                   : cui_text_view_attribute(p->input, name_end, end, "href");
                else if (cui_text_view_name(name, "br")) {
                    if (active)
                        cui_text_view_emit(p, (cui_str){"\n", 1}, (cui_text_view_range){a, end + 1},
                                           flags, link);
                } else if (cui_text_view_name(name, "table")) {
                    if (!closing) {
                        ++table;
                        row = 0;
                        col = 0;
                    }
                } else if (cui_text_view_name(name, "tr")) {
                    if (closing) {
                        cui_text_view_gap(p, '\n');
                        ++row;
                    }
                    col = 0;
                } else {
                    cui_text_view_block_kind kind = CUI_TEXT_VIEW_PARAGRAPH;
                    unsigned level = 0;
                    bool block = false;
                    if (name.length == 2 && (name.data[0] == 'h' || name.data[0] == 'H') &&
                        name.data[1] >= '1' && name.data[1] <= '6') {
                        kind = CUI_TEXT_VIEW_HEADING;
                        level = (unsigned)(name.data[1] - '0');
                        block = true;
                    } else if (cui_text_view_name(name, "p") || cui_text_view_name(name, "div"))
                        block = true;
                    else if (cui_text_view_name(name, "blockquote")) {
                        kind = CUI_TEXT_VIEW_QUOTE;
                        block = true;
                    } else if (cui_text_view_name(name, "li")) {
                        kind = CUI_TEXT_VIEW_LIST_ITEM;
                        block = true;
                    } else if (cui_text_view_name(name, "pre")) {
                        kind = CUI_TEXT_VIEW_CODE_BLOCK;
                        block = true;
                        pre = !closing;
                    } else if (cui_text_view_name(name, "td") || cui_text_view_name(name, "th")) {
                        kind = CUI_TEXT_VIEW_TABLE_CELL;
                        block = true;
                    } else if (cui_text_view_name(name, "hr")) {
                        kind = CUI_TEXT_VIEW_RULE;
                        block = true;
                    } else if (cui_text_view_name(name, "img")) {
                        kind = CUI_TEXT_VIEW_IMAGE;
                        block = true;
                    }
                    if (block) {
                        if (active) {
                            cui_text_view_block_end(p, a, kind != CUI_TEXT_VIEW_TABLE_CELL);
                            active = false;
                        }
                        if (!closing) {
                            if (!cui_text_view_block_begin(p, kind, a, level))
                                return;
                            active = true;
                            if (p->write && kind == CUI_TEXT_VIEW_TABLE_CELL) {
                                cui_text_view_block *v = &p->state->workspace.blocks[p->block];
                                v->table = table;
                                v->row = row;
                                v->column = col;
                                v->table_header = cui_text_view_name(name, "th");
                            }
                            if (kind == CUI_TEXT_VIEW_TABLE_CELL)
                                ++col;
                            if (kind == CUI_TEXT_VIEW_IMAGE) {
                                cui_text_view_range src = cui_text_view_attribute(
                                                        p->input, name_end, end, "src"),
                                                    alt = cui_text_view_attribute(
                                                        p->input, name_end, end, "alt");
                                if (p->write)
                                    p->state->workspace.blocks[p->block].resource = src;
                                for (size_t i = alt.begin; i < alt.end;) {
                                    size_t n = cui_text_view_scalar(p->input, i);
                                    cui_text_view_emit(p, (cui_str){p->input.data + i, n},
                                                       (cui_text_view_range){i, i + n}, 0, link);
                                    i += n;
                                }
                                cui_text_view_block_end(p, end + 1, true);
                                active = false;
                            } else if (kind == CUI_TEXT_VIEW_RULE) {
                                cui_text_view_block_end(p, end + 1, true);
                                active = false;
                            }
                        } else if (kind == CUI_TEXT_VIEW_TABLE_CELL)
                            cui_text_view_gap(p, ' ');
                    }
                }
                a = end + 1;
                continue;
            }
        }
        if (!active) {
            if (!pre && (p->input.data[a] == ' ' || p->input.data[a] == '\n' ||
                         p->input.data[a] == '\r' || p->input.data[a] == '\t')) {
                ++a;
                continue;
            }
            if (!cui_text_view_block_begin(p, CUI_TEXT_VIEW_PARAGRAPH, a, 0))
                return;
            active = true;
        }
        if (p->input.data[a] == '&' && cui_text_view_entity(p, &a, p->input.length, flags, link))
            continue;
        size_t n = cui_text_view_scalar(p->input, a);
        char ch = p->input.data[a];
        if (!pre && (ch == '\n' || ch == '\r' || ch == '\t'))
            cui_text_view_emit(p, (cui_str){" ", 1}, (cui_text_view_range){a, a + n}, flags, link);
        else
            cui_text_view_emit(p, (cui_str){p->input.data + a, n}, (cui_text_view_range){a, a + n},
                               flags, link);
        a += n;
    }
    if (active)
        cui_text_view_block_end(p, p->input.length, true);
}
static void cui_text_view_parse(cui_text_view_parser *p) {
    if (p->state->format == CUI_TEXT_VIEW_MARKDOWN)
        cui_text_view_markdown(p);
    else if (p->state->format == CUI_TEXT_VIEW_HTML)
        cui_text_view_html(p);
    else if (p->input.length) {
        if (cui_text_view_block_begin(p, CUI_TEXT_VIEW_PARAGRAPH, 0, 0)) {
            for (size_t i = 0; i < p->input.length;) {
                size_t n = cui_text_view_scalar(p->input, i);
                cui_text_view_emit(p, (cui_str){p->input.data + i, n},
                                   (cui_text_view_range){i, i + n}, 0, (cui_text_view_range){0});
                i += n;
            }
            cui_text_view_block_end(p, p->input.length, false);
        }
    }
}
static bool cui_text_view_source_of_range(const cui_text_view_state *s, cui_text_view_range range,
                                          cui_text_view_range *out) {
    size_t a = SIZE_MAX, b = 0;
    for (size_t i = range.begin; i < range.end; ++i) {
        cui_text_view_range m = s->workspace.mapping[i].source;
        if (m.begin != SIZE_MAX) {
            if (m.begin < a)
                a = m.begin;
            if (m.end > b)
                b = m.end;
        }
    }
    if (a == SIZE_MAX)
        return false;
    *out = (cui_text_view_range){a, b};
    return true;
}
static bool cui_text_view_remap_source(cui_text_view_range *r, size_t prefix, size_t suffix,
                                       size_t old_length, size_t new_length) {
    if (r->end <= prefix)
        return true;
    if (r->begin >= old_length - suffix) {
        r->begin = new_length - (old_length - r->begin);
        r->end = new_length - (old_length - r->end);
        return true;
    }
    return false;
}
static bool cui_text_view_table_highlight_stable(const cui_text_view_state *s,
                                                 cui_text_view_range range, size_t prefix,
                                                 size_t suffix, bool append) {
    if (append)
        return true;
    for (size_t i = 0; i < s->block_count; ++i) {
        const cui_text_view_block *cell = &s->workspace.blocks[i];
        if (cell->kind != CUI_TEXT_VIEW_TABLE_CELL || cell->text.end <= range.begin ||
            cell->text.begin >= range.end)
            continue;
        size_t first = i, last = i;
        while (first && s->workspace.blocks[first - 1].kind == CUI_TEXT_VIEW_TABLE_CELL &&
               s->workspace.blocks[first - 1].table == cell->table)
            --first;
        while (last + 1 < s->block_count &&
               s->workspace.blocks[last + 1].kind == CUI_TEXT_VIEW_TABLE_CELL &&
               s->workspace.blocks[last + 1].table == cell->table)
            ++last;
        if (s->workspace.blocks[first].source.begin >= s->source_length - suffix)
            continue;
        size_t table_end = cui_text_view_next_line(
            cui_text_view_source(s),
            cui_text_view_line_end(cui_text_view_source(s), s->workspace.blocks[last].source.end));
        if (prefix >= table_end)
            continue;
        size_t row_end = cui_text_view_next_line(
            cui_text_view_source(s),
            cui_text_view_line_end(cui_text_view_source(s), cell->source.end));
        if (row_end > prefix)
            return false;
    }
    return true;
}
cui_text_view_error cui_text_view_set_text(cui_text_view_state *s, cui_str input) {
    if (!s || !s->workspace.source || !cui_text_view_valid(input))
        return CUI_TEXT_VIEW_INVALID;
    if (input.length >= s->workspace.source_capacity)
        return CUI_TEXT_VIEW_CAPACITY;
    cui_text_view_parser dry = {.state = s, .input = input};
    cui_text_view_parse(&dry);
    if (dry.error)
        return dry.error;
    bool append = input.length >= s->source_length &&
                  (!s->source_length || !memcmp(input.data, s->workspace.source, s->source_length));
    if (input.length == s->source_length && append)
        return CUI_TEXT_VIEW_OK;
    size_t prefix = 0, suffix = 0;
    while (prefix < input.length && prefix < s->source_length &&
           input.data[prefix] == s->workspace.source[prefix])
        ++prefix;
    while (prefix && (!cui_text_view_boundary(input, prefix) ||
                      !cui_text_view_boundary(cui_text_view_source(s), prefix)))
        --prefix;
    while (suffix < input.length - prefix && suffix < s->source_length - prefix &&
           input.data[input.length - 1 - suffix] ==
               s->workspace.source[s->source_length - 1 - suffix])
        ++suffix;
    while (suffix && (!cui_text_view_boundary(input, input.length - suffix) ||
                      !cui_text_view_boundary(cui_text_view_source(s), s->source_length - suffix)))
        --suffix;
    size_t kept = 0;
    for (size_t i = 0; i < s->highlight_count; ++i) {
        cui_text_view_highlight h = s->workspace.highlights[i];
        cui_text_view_range source;
        if (!cui_text_view_table_highlight_stable(s, h.range, prefix, suffix, append) ||
            !cui_text_view_source_of_range(s, h.range, &source) ||
            !cui_text_view_remap_source(&source, prefix, suffix, s->source_length, input.length))
            continue;
        h.range = source;
        s->workspace.highlights[kept++] = h;
    }
    bool reveal = s->reveal_pending;
    cui_text_view_range reveal_source;
    if (reveal)
        reveal = cui_text_view_source_of_range(s, s->reveal, &reveal_source) &&
                 cui_text_view_remap_source(&reveal_source, prefix, suffix, s->source_length,
                                            input.length);
    if (input.length)
        memmove(s->workspace.source, input.data, input.length);
    s->workspace.source[input.length] = 0;
    cui_text_view_parser actual = {.state = s,
                                   .input = {s->workspace.source, input.length},
                                   .write = true,
                                   .fade = append && s->stream_fade && !s->reduced_motion};
    cui_text_view_parse(&actual);
    s->source_length = input.length;
    s->text_length = actual.text;
    s->span_count = actual.spans;
    s->block_count = actual.blocks;
    s->glyph_count = 0;
    s->workspace.rendered[s->text_length] = 0;
    s->highlight_count = 0;
    for (size_t i = 0; i < kept; ++i) {
        cui_text_view_highlight h = s->workspace.highlights[i];
        cui_text_view_range mapped;
        if (cui_text_view_range_for_source(s, h.range, &mapped)) {
            h.range = mapped;
            s->workspace.highlights[s->highlight_count++] = h;
        }
    }
    s->reveal_pending = reveal && cui_text_view_range_for_source(s, reveal_source, &s->reveal);
    if (!append) {
        s->anchor = s->cursor = 0;
    } else {
        if (s->anchor > s->text_length)
            s->anchor = s->text_length;
        if (s->cursor > s->text_length)
            s->cursor = s->text_length;
    }
    return actual.error;
}
bool cui_text_view_select(cui_text_view_state *s, size_t a, size_t c) {
    if (!s || !cui_text_view_boundary(cui_text_view_rendered_text(s), a) ||
        !cui_text_view_boundary(cui_text_view_rendered_text(s), c))
        return false;
    bool changed = s->anchor != a || s->cursor != c;
    s->anchor = a;
    s->cursor = c;
    return changed;
}
cui_str cui_text_view_selected_text(const cui_text_view_state *s) {
    if (!s)
        return (cui_str){0};
    size_t a = s->anchor < s->cursor ? s->anchor : s->cursor,
           b = s->anchor > s->cursor ? s->anchor : s->cursor;
    return (cui_str){s->workspace.rendered + a, b - a};
}
bool cui_text_view_selected_source_range(const cui_text_view_state *s, cui_text_view_range *out) {
    if (!s || s->format == CUI_TEXT_VIEW_HTML)
        return false;
    size_t a = s->anchor < s->cursor ? s->anchor : s->cursor,
           b = s->anchor > s->cursor ? s->anchor : s->cursor;
    size_t start = SIZE_MAX, end = 0;
    for (size_t i = a; i < b; ++i) {
        cui_text_view_range m = s->workspace.mapping[i].source;
        if (m.begin != SIZE_MAX) {
            if (m.begin < start)
                start = m.begin;
            if (m.end > end)
                end = m.end;
        }
    }
    if (start == SIZE_MAX)
        return false;
    if (out)
        *out = (cui_text_view_range){start, end};
    return true;
}
bool cui_text_view_range_for_source(const cui_text_view_state *s, cui_text_view_range r,
                                    cui_text_view_range *out) {
    if (!s || s->format == CUI_TEXT_VIEW_HTML || r.begin == r.end ||
        !cui_text_view_range_valid(cui_text_view_source(s), r))
        return false;
    size_t begin = SIZE_MAX, end = 0;
    for (size_t i = 0; i < s->text_length; ++i) {
        cui_text_view_range m = s->workspace.mapping[i].source;
        if (m.begin != SIZE_MAX && m.begin < r.end && m.end > r.begin) {
            if (begin == SIZE_MAX)
                begin = i;
            end = i + 1;
        }
    }
    if (begin == SIZE_MAX)
        return false;
    if (out)
        *out = (cui_text_view_range){begin, end};
    return true;
}
cui_text_view_error cui_text_view_set_highlights(cui_text_view_state *s,
                                                 const cui_text_view_highlight *h, size_t n) {
    if (!s || (n && !h))
        return CUI_TEXT_VIEW_INVALID;
    if (s->format == CUI_TEXT_VIEW_HTML)
        return CUI_TEXT_VIEW_UNSUPPORTED;
    if (n > s->workspace.highlight_capacity)
        return CUI_TEXT_VIEW_CAPACITY;
    for (size_t i = 0; i < n; ++i)
        if (!cui_text_view_range_valid(cui_text_view_rendered_text(s), h[i].range))
            return CUI_TEXT_VIEW_INVALID;
    if (n)
        memmove(s->workspace.highlights, h, n * sizeof *h);
    s->highlight_count = n;
    return CUI_TEXT_VIEW_OK;
}
cui_text_view_error cui_text_view_reveal(cui_text_view_state *s, cui_text_view_range r) {
    if (!s || !cui_text_view_range_valid(cui_text_view_rendered_text(s), r))
        return CUI_TEXT_VIEW_INVALID;
    s->reveal = r;
    s->reveal_pending = true;
    s->reveal_age = 0;
    return CUI_TEXT_VIEW_OK;
}
bool cui_text_view_advance(cui_text_view_state *s, float dt) {
    if (!s || !isfinite(dt) || dt < 0)
        return false;
    if (!isfinite(s->time + dt))
        return false;
    s->time += dt;
    if (s->reveal_pending) {
        s->reveal_age += dt;
        if (s->reveal_age > 1)
            s->reveal_pending = false;
    }
    if (s->reduced_motion || !s->stream_fade)
        return s->reveal_pending;
    for (size_t i = 0; i < s->text_length; ++i)
        if (s->workspace.mapping[i].born >= 0 &&
            s->time - s->workspace.mapping[i].born < s->fade_duration)
            return true;
    return s->reveal_pending;
}
static unsigned cui_text_view_block_flags(const cui_text_view_block *b) {
    return b->kind == CUI_TEXT_VIEW_HEADING      ? (b->level == 1   ? CUI_TEXT_VIEW_BOLD
                                                    : b->level == 6 ? CUI_TEXT_VIEW_MEDIUM
                                                                    : CUI_TEXT_VIEW_SEMIBOLD)
           : b->kind == CUI_TEXT_VIEW_CODE_BLOCK ? CUI_TEXT_VIEW_CODE
                                                 : 0;
}
static bool cui_text_view_layout_block(cui_text_view_state *s, size_t index, float x, float y,
                                       float width, cui_text_view_layout_options o,
                                       cui_text_view_measure_fn measure, void *user) {
    cui_text_view_block *b = &s->workspace.blocks[index];
    float scale = b->kind == CUI_TEXT_VIEW_HEADING ? (b->level == 1   ? 2
                                                      : b->level == 2 ? 1.5f
                                                      : b->level == 3 ? 1.25f
                                                      : b->level == 4 ? 1.125f
                                                                      : 1)
                                                   : 1;
    float font = o.font_size * scale, line = o.line_height * scale,
          padding = b->kind == CUI_TEXT_VIEW_CODE_BLOCK   ? 12
                    : b->kind == CUI_TEXT_VIEW_TABLE_CELL ? 4
                                                          : 0;
    float left = b->kind == CUI_TEXT_VIEW_QUOTE        ? 12
                 : b->kind == CUI_TEXT_VIEW_LIST_ITEM  ? 20
                 : b->kind == CUI_TEXT_VIEW_TABLE_CELL ? 8
                                                       : padding;
    if (b->kind == CUI_TEXT_VIEW_LIST_ITEM && b->level) {
        cui_size marker = measure(user, (cui_str){b->marker_text, strlen(b->marker_text)}, font, 0);
        if (!isfinite(marker.width) || marker.width < 0 || marker.width > 10000000)
            return false;
        left = fmaxf(left, marker.width + 8);
    }
    b->text_inset = left;
    float available = fmaxf(1, width - left - (b->kind == CUI_TEXT_VIEW_TABLE_CELL ? 8 : padding));
    float cursor_x = 0, cursor_y = 0;
    size_t first_glyph = s->glyph_count;
    if (b->kind == CUI_TEXT_VIEW_RULE) {
        b->bounds = (cui_rect){x, y, width, 1};
        return true;
    }
    if (b->kind == CUI_TEXT_VIEW_IMAGE && b->image_ready && b->image.width && b->image.height) {
        float w = fminf(width, (float)b->image.width);
        b->bounds = (cui_rect){x, y, w, w * b->image.height / b->image.width};
        return true;
    }
    for (size_t span = 0; span < s->span_count; ++span) {
        cui_text_view_span *r = &s->workspace.spans[span];
        if (r->block != index)
            continue;
        unsigned flags = r->flags | cui_text_view_block_flags(b);
        float size =
            flags & CUI_TEXT_VIEW_CODE && b->kind != CUI_TEXT_VIEW_CODE_BLOCK ? font * .875f : font;
        size_t segment = r->text.begin;
        float measured = 0;
        for (size_t i = r->text.begin; i < r->text.end;) {
            size_t n = cui_text_view_scalar(cui_text_view_rendered_text(s), i);
            if (!n)
                return false;
            if (s->workspace.rendered[i] == '\n') {
                cursor_x = 0;
                cursor_y += line;
                i += n;
                segment = i;
                measured = 0;
                continue;
            }
            bool word = s->workspace.rendered[i] != ' ' && s->workspace.rendered[i] != '\t' &&
                        (i == r->text.begin || s->workspace.rendered[i - 1] == ' ' ||
                         s->workspace.rendered[i - 1] == '\n');
            if (word && cursor_x > 0) {
                size_t end = i;
                while (end < r->text.end && s->workspace.rendered[end] != ' ' &&
                       s->workspace.rendered[end] != '\t' && s->workspace.rendered[end] != '\n')
                    end += cui_text_view_scalar(cui_text_view_rendered_text(s), end);
                cui_size word_size =
                    measure(user, (cui_str){s->workspace.rendered + i, end - i}, size, flags);
                if (!isfinite(word_size.width) || word_size.width < 0 || word_size.width > 10000000)
                    return false;
                if (word_size.width > available - cursor_x) {
                    cursor_x = 0;
                    cursor_y += line;
                    segment = i;
                    measured = 0;
                }
            }
            cui_size m = measure(user, (cui_str){s->workspace.rendered + segment, i + n - segment},
                                 size, flags);
            if (!isfinite(m.width) || m.width < 0 || m.width > 10000000)
                return false;
            float advance = fmaxf(0, m.width - measured);
            if (cursor_x > 0 && cursor_x + advance > available) {
                cursor_x = 0;
                cursor_y += line;
                segment = i;
                measured = 0;
                m = measure(user, (cui_str){s->workspace.rendered + i, n}, size, flags);
                advance = m.width;
                if (!isfinite(advance) || advance < 0 || advance > 10000000)
                    return false;
            }
            if (s->glyph_count >= s->workspace.glyph_capacity)
                return false;
            s->workspace.glyphs[s->glyph_count++] = (cui_text_view_glyph){
                .text = {i, i + n},
                .span = span,
                .block = index,
                .bounds = {x + left + cursor_x, y + padding + cursor_y, advance, line},
                .font_size = size,
                .line_height = line,
                .flags = flags};
            cursor_x += advance;
            measured = m.width;
            i += n;
        }
    }
    if (b->kind == CUI_TEXT_VIEW_TABLE_CELL && b->alignment != CUI_TEXT_VIEW_ALIGN_LEFT) {
        for (size_t first = first_glyph; first < s->glyph_count;) {
            size_t last = first + 1;
            while (last < s->glyph_count &&
                   s->workspace.glyphs[last].bounds.y == s->workspace.glyphs[first].bounds.y)
                ++last;
            cui_rect tail = s->workspace.glyphs[last - 1].bounds;
            float extra = fmaxf(0, available - (tail.x + tail.w - x - left));
            float shift = b->alignment == CUI_TEXT_VIEW_ALIGN_CENTER ? extra * .5f : extra;
            for (size_t j = first; j < last; ++j)
                s->workspace.glyphs[j].bounds.x += shift;
            first = last;
        }
    }
    b->actions_height = b->kind == CUI_TEXT_VIEW_CODE_BLOCK ? o.code_actions_height : 0;
    b->bounds = (cui_rect){x, y, width, cursor_y + line + padding * 2};
    return true;
}
static void cui_text_view_table_widths(cui_text_view_state *s, size_t first, size_t end,
                                       float width) {
    float weights[CUI_TEXT_VIEW_NODE_LIMIT], widths[CUI_TEXT_VIEW_NODE_LIMIT];
    unsigned columns = 0;
    for (size_t i = first; i < end; ++i)
        if (s->workspace.blocks[i].column + 1 > columns)
            columns = s->workspace.blocks[i].column + 1;
    for (unsigned c = 0; c < columns; ++c) {
        weights[c] = 5;
        widths[c] = 0;
    }
    for (size_t i = first; i < end; ++i) {
        const cui_text_view_block *cell = &s->workspace.blocks[i];
        unsigned count = 0;
        for (size_t j = cell->text.begin; j < cell->text.end && count < 150; ++j)
            if (((unsigned char)s->workspace.rendered[j] & 0xc0) != 0x80)
                ++count;
        if ((float)count > weights[cell->column])
            weights[cell->column] = (float)count;
    }
    float remaining = fmaxf(width, (float)columns * 64), total = 0;
    for (unsigned c = 0; c < columns; ++c)
        total += weights[c];
    bool changed;
    do {
        changed = false;
        for (unsigned c = 0; c < columns; ++c)
            if (widths[c] == 0 && remaining * (weights[c] / total) < 64) {
                widths[c] = 64;
                remaining -= 64;
                total -= weights[c];
                changed = true;
            }
    } while (changed && total > 0);
    float x = 0;
    for (unsigned c = 0; c < columns; ++c) {
        if (widths[c] == 0)
            widths[c] = remaining * (weights[c] / total);
        for (size_t i = first; i < end; ++i)
            if (s->workspace.blocks[i].column == c) {
                s->workspace.blocks[i].bounds.x = x;
                s->workspace.blocks[i].bounds.w = widths[c];
                s->workspace.blocks[i].table_columns = columns;
            }
        x += widths[c];
    }
}
cui_text_view_error cui_text_view_layout(cui_text_view_state *s, cui_text_view_layout_options o,
                                         cui_text_view_measure_fn measure, void *user) {
    if (!s || !measure || !isfinite(o.width) || o.width <= 0 || o.width > 10000000 ||
        !isfinite(o.font_size) || o.font_size <= 0 || o.font_size > 10000000 ||
        !isfinite(o.line_height) || o.line_height <= 0 || o.line_height > 10000000 ||
        !isfinite(o.paragraph_gap) || o.paragraph_gap < 0 || o.paragraph_gap > 10000000 ||
        !isfinite(o.code_actions_height) || o.code_actions_height < 0 ||
        o.code_actions_height > 10000000 || !isfinite(o.table_actions_height) ||
        o.table_actions_height < 0 || o.table_actions_height > 10000000)
        return CUI_TEXT_VIEW_INVALID;
    if (s->text_length > s->workspace.glyph_capacity)
        return CUI_TEXT_VIEW_CAPACITY;
    s->glyph_count = 0;
    float y = 0;
    for (size_t b = 0; b < s->block_count;) {
        cui_text_view_block *block = &s->workspace.blocks[b];
        if (block->kind == CUI_TEXT_VIEW_TABLE_CELL) {
            unsigned table = block->table, row = block->row;
            if (b == 0 || s->workspace.blocks[b - 1].kind != CUI_TEXT_VIEW_TABLE_CELL ||
                s->workspace.blocks[b - 1].table != table) {
                size_t end = b + 1;
                while (end < s->block_count &&
                       s->workspace.blocks[end].kind == CUI_TEXT_VIEW_TABLE_CELL &&
                       s->workspace.blocks[end].table == table)
                    ++end;
                cui_text_view_table_widths(s, b, end, o.width);
            }
            float height = 0;
            size_t first = b;
            while (b < s->block_count && s->workspace.blocks[b].kind == CUI_TEXT_VIEW_TABLE_CELL &&
                   s->workspace.blocks[b].table == table && s->workspace.blocks[b].row == row) {
                float x = s->workspace.blocks[b].bounds.x, w = s->workspace.blocks[b].bounds.w;
                if (!cui_text_view_layout_block(s, b, x, y, w, o, measure, user))
                    return CUI_TEXT_VIEW_INVALID;
                if (s->workspace.blocks[b].bounds.h > height)
                    height = s->workspace.blocks[b].bounds.h;
                ++b;
            }
            for (size_t i = first; i < b; ++i)
                s->workspace.blocks[i].bounds.h = height;
            y += height;
            if (b == s->block_count || s->workspace.blocks[b].kind != CUI_TEXT_VIEW_TABLE_CELL ||
                s->workspace.blocks[b].table != table) {
                s->workspace.blocks[b - 1].actions_height = o.table_actions_height;
                y +=
                    o.paragraph_gap + (o.table_actions_height > 0 ? o.table_actions_height + 4 : 0);
            }
        } else {
            if (!cui_text_view_layout_block(s, b, 0, y, o.width, o, measure, user))
                return CUI_TEXT_VIEW_INVALID;
            y += block->bounds.h;
            if (b + 1 < s->block_count &&
                (block->kind != CUI_TEXT_VIEW_LIST_ITEM ||
                 s->workspace.blocks[b + 1].kind != CUI_TEXT_VIEW_LIST_ITEM))
                y += block->kind == CUI_TEXT_VIEW_HEADING ? o.font_size * (16.f / 14) * .3f
                                                          : o.paragraph_gap;
            ++b;
        }
        if (!isfinite(y) || y > 10000000)
            return CUI_TEXT_VIEW_INVALID;
    }
    s->width = o.width;
    s->height = y;
    s->clamped = false;
    if (o.max_lines && !o.scrollable) {
        float cap = (float)o.max_lines * o.line_height;
        if (y > cap) {
            s->clamped = true;
            s->height = cap;
        }
        s->reveal_pending = false;
    }
    return CUI_TEXT_VIEW_OK;
}
size_t cui_text_view_hit(const cui_text_view_state *s, float x, float y) {
    if (!s || !isfinite(x) || !isfinite(y))
        return SIZE_MAX;
    float nearest = INFINITY;
    size_t offset = 0;
    for (size_t i = 0; i < s->glyph_count; ++i) {
        const cui_text_view_glyph *g = &s->workspace.glyphs[i];
        float dy = y < g->bounds.y                 ? g->bounds.y - y
                   : y > g->bounds.y + g->bounds.h ? y - g->bounds.y - g->bounds.h
                                                   : 0;
        float dx = x < g->bounds.x                 ? g->bounds.x - x
                   : x > g->bounds.x + g->bounds.w ? x - g->bounds.x - g->bounds.w
                                                   : 0;
        float distance = dy * dy * 10000 + dx * dx;
        if (distance < nearest) {
            nearest = distance;
            offset = x > g->bounds.x + g->bounds.w * .5f ? g->text.end : g->text.begin;
        }
    }
    return offset;
}

/* Two-pass writer keeps copy capacity failures transactional. */
typedef struct {
    char *output;
    size_t length;
    bool flatten;
} cui_text_view_copy_writer;
static void cui_text_view_copy_write(cui_text_view_copy_writer *w, cui_str t) {
    if (w->output && t.length) {
        memcpy(w->output + w->length, t.data, t.length);
        if (w->flatten)
            for (size_t i = 0; i < t.length; ++i)
                if (w->output[w->length + i] == '\n')
                    w->output[w->length + i] = ' ';
    }
    w->length += t.length;
}
static void cui_text_view_copy_literal(cui_text_view_copy_writer *w, const char *t) {
    cui_text_view_copy_write(w, (cui_str){t, strlen(t)});
}
static void cui_text_view_copy_escape(cui_text_view_copy_writer *w, cui_str t, bool destination) {
    for (size_t i = 0; i < t.length; ++i) {
        char c = t.data[i];
        if (!destination && c == '\n' && !w->flatten) {
            cui_text_view_copy_literal(w, "  \n");
            continue;
        }
        if (c == '\\' ||
            (destination ? c == '(' || c == ')'
                         : c == '*' || c == '_' || c == '[' || c == ']' || c == '~' || c == '#' ||
                               c == '>' || c == '|' || c == '&' || c == '<' || c == '`'))
            cui_text_view_copy_literal(w, "\\");
        cui_text_view_copy_write(w, (cui_str){t.data + i, 1});
    }
}
static unsigned cui_text_view_copy_ticks(cui_str t) {
    unsigned longest = 0, run = 0;
    for (size_t i = 0; i < t.length; ++i) {
        run = t.data[i] == '`' ? run + 1 : 0;
        if (run > longest)
            longest = run;
    }
    return longest + 1;
}
static void cui_text_view_copy_repeat(cui_text_view_copy_writer *w, const char *t, unsigned n) {
    for (unsigned i = 0; i < n; ++i)
        cui_text_view_copy_literal(w, t);
}
static void cui_text_view_copy_inline(const cui_text_view_state *s, cui_text_view_copy_writer *w,
                                      cui_text_view_range r, size_t block) {
    for (size_t i = 0; i < s->span_count; ++i) {
        const cui_text_view_span *span = &s->workspace.spans[i];
        if (span->block != block || span->text.end <= r.begin || span->text.begin >= r.end)
            continue;
        size_t a = span->text.begin > r.begin ? span->text.begin : r.begin;
        size_t b = span->text.end < r.end ? span->text.end : r.end;
        cui_str t = {s->workspace.rendered + a, b - a};
        bool link = span->link.end > span->link.begin;
        unsigned flags = span->flags, ticks = 0;
        if (link)
            cui_text_view_copy_literal(w, "[");
        if (flags & CUI_TEXT_VIEW_BOLD)
            cui_text_view_copy_literal(w, "**");
        if (flags & CUI_TEXT_VIEW_ITALIC)
            cui_text_view_copy_literal(w, "*");
        if (flags & CUI_TEXT_VIEW_STRIKE)
            cui_text_view_copy_literal(w, "~~");
        if (flags & CUI_TEXT_VIEW_CODE) {
            ticks = cui_text_view_copy_ticks(t);
            cui_text_view_copy_repeat(w, "`", ticks);
            if (t.length && (t.data[0] == '`' || t.data[t.length - 1] == '`'))
                cui_text_view_copy_literal(w, " ");
            cui_text_view_copy_write(w, t);
            if (t.length && (t.data[0] == '`' || t.data[t.length - 1] == '`'))
                cui_text_view_copy_literal(w, " ");
            cui_text_view_copy_repeat(w, "`", ticks);
        } else
            cui_text_view_copy_escape(w, t, false);
        if (flags & CUI_TEXT_VIEW_STRIKE)
            cui_text_view_copy_literal(w, "~~");
        if (flags & CUI_TEXT_VIEW_ITALIC)
            cui_text_view_copy_literal(w, "*");
        if (flags & CUI_TEXT_VIEW_BOLD)
            cui_text_view_copy_literal(w, "**");
        if (link) {
            cui_text_view_copy_literal(w, "](");
            cui_text_view_copy_escape(w,
                                      (cui_str){s->workspace.source + span->link.begin,
                                                span->link.end - span->link.begin},
                                      true);
            cui_text_view_copy_literal(w, ")");
        }
    }
}
static void cui_text_view_copy_table(const cui_text_view_state *s, cui_text_view_copy_writer *w,
                                     size_t start, size_t end, size_t a, size_t b) {
    unsigned row = s->workspace.blocks[start].row;
    unsigned columns = 0;
    cui_text_view_copy_literal(w, "| ");
    w->flatten = true;
    for (size_t i = start; i < end; ++i) {
        const cui_text_view_block *cell = &s->workspace.blocks[i];
        if (cell->row != row) {
            w->flatten = false;
            cui_text_view_copy_literal(w, " |\n");
            if (row == 0) {
                cui_text_view_copy_literal(w, "| ");
                for (unsigned col = 0; col < columns; ++col) {
                    if (col)
                        cui_text_view_copy_literal(w, " | ");
                    cui_text_view_alignment align = s->workspace.blocks[start + col].alignment;
                    cui_text_view_copy_literal(w, align == CUI_TEXT_VIEW_ALIGN_CENTER  ? ":-:"
                                                  : align == CUI_TEXT_VIEW_ALIGN_RIGHT ? "--:"
                                                                                       : ":--");
                }
                cui_text_view_copy_literal(w, " |\n");
            }
            row = cell->row;
            columns = 0;
            cui_text_view_copy_literal(w, "| ");
            w->flatten = true;
        }
        if (columns)
            cui_text_view_copy_literal(w, " | ");
        ++columns;
        size_t begin = cell->text.begin > a ? cell->text.begin : a,
               finish = cell->text.end < b ? cell->text.end : b;
        while (begin < finish && s->workspace.rendered[begin] == ' ')
            ++begin;
        while (finish > begin && s->workspace.rendered[finish - 1] == ' ')
            --finish;
        if (begin < finish)
            cui_text_view_copy_inline(s, w, (cui_text_view_range){begin, finish}, i);
    }
    w->flatten = false;
    cui_text_view_copy_literal(w, " |");
    if (row == 0) {
        cui_text_view_copy_literal(w, "\n| ");
        for (unsigned col = 0; col < columns; ++col) {
            if (col)
                cui_text_view_copy_literal(w, " | ");
            cui_text_view_alignment align = s->workspace.blocks[start + col].alignment;
            cui_text_view_copy_literal(w, align == CUI_TEXT_VIEW_ALIGN_CENTER  ? ":-:"
                                          : align == CUI_TEXT_VIEW_ALIGN_RIGHT ? "--:"
                                                                               : ":--");
        }
        cui_text_view_copy_literal(w, " |");
    }
}
static cui_text_view_error cui_text_view_copy_source(const cui_text_view_state *s,
                                                     cui_text_view_copy_writer *w, size_t a,
                                                     size_t b) {
    bool prior = false, prior_list = false;
    for (size_t i = 0; i < s->block_count; ++i) {
        const cui_text_view_block *block = &s->workspace.blocks[i];
        if (block->text.end <= a || block->text.begin >= b)
            continue;
        if (block->kind == CUI_TEXT_VIEW_TABLE_CELL) {
            size_t start = i, end = i + 1;
            while (start && s->workspace.blocks[start - 1].kind == CUI_TEXT_VIEW_TABLE_CELL &&
                   s->workspace.blocks[start - 1].table == block->table)
                --start;
            while (end < s->block_count &&
                   s->workspace.blocks[end].kind == CUI_TEXT_VIEW_TABLE_CELL &&
                   s->workspace.blocks[end].table == block->table)
                ++end;
            if (prior)
                cui_text_view_copy_literal(w, "\n\n");
            cui_text_view_copy_table(s, w, start, end, a, b);
            prior = true;
            prior_list = false;
            i = end - 1;
            continue;
        }
        size_t begin = block->text.begin > a ? block->text.begin : a;
        size_t end = block->text.end < b ? block->text.end : b;
        if (prior)
            cui_text_view_copy_literal(
                w, prior_list && block->kind == CUI_TEXT_VIEW_LIST_ITEM ? "\n" : "\n\n");
        prior = true;
        prior_list = block->kind == CUI_TEXT_VIEW_LIST_ITEM;
        if (block->kind == CUI_TEXT_VIEW_HEADING) {
            cui_text_view_copy_repeat(w, "#", block->level);
            cui_text_view_copy_literal(w, " ");
        } else if (block->kind == CUI_TEXT_VIEW_QUOTE)
            cui_text_view_copy_literal(w, "> ");
        else if (block->kind == CUI_TEXT_VIEW_LIST_ITEM) {
            if (block->level)
                cui_text_view_copy_literal(w, block->marker_text);
            else
                cui_text_view_copy_literal(w, "-");
            cui_text_view_copy_literal(w, " ");
        }
        if (block->kind == CUI_TEXT_VIEW_CODE_BLOCK) {
            cui_str t = {s->workspace.rendered + begin, end - begin};
            unsigned ticks = cui_text_view_copy_ticks(t);
            if (ticks < 3)
                ticks = 3;
            cui_text_view_copy_repeat(w, "`", ticks);
            cui_text_view_copy_write(w, (cui_str){s->workspace.source + block->resource.begin,
                                                  block->resource.end - block->resource.begin});
            cui_text_view_copy_literal(w, "\n");
            cui_text_view_copy_write(w, t);
            if (!t.length || t.data[t.length - 1] != '\n')
                cui_text_view_copy_literal(w, "\n");
            cui_text_view_copy_repeat(w, "`", ticks);
        } else if (block->kind == CUI_TEXT_VIEW_IMAGE) {
            cui_text_view_copy_literal(w, "![");
            cui_text_view_copy_escape(w, (cui_str){s->workspace.rendered + begin, end - begin},
                                      false);
            cui_text_view_copy_literal(w, "](");
            cui_text_view_copy_escape(w,
                                      (cui_str){s->workspace.source + block->resource.begin,
                                                block->resource.end - block->resource.begin},
                                      true);
            cui_text_view_copy_literal(w, ")");
        } else
            cui_text_view_copy_inline(s, w, (cui_text_view_range){begin, end}, i);
    }
    return CUI_TEXT_VIEW_OK;
}
cui_text_view_error cui_text_view_copy_selection(const cui_text_view_state *s,
                                                 cui_text_view_copy_format f, char *output,
                                                 size_t capacity, size_t *length) {
    if (!s || f < CUI_TEXT_VIEW_COPY_PLAIN || f > CUI_TEXT_VIEW_COPY_SOURCE ||
        (!output && capacity))
        return CUI_TEXT_VIEW_INVALID;
    size_t a = s->anchor < s->cursor ? s->anchor : s->cursor,
           b = s->anchor > s->cursor ? s->anchor : s->cursor;
    cui_str direct = {0};
    if (f == CUI_TEXT_VIEW_COPY_PLAIN || s->format != CUI_TEXT_VIEW_MARKDOWN)
        direct = (cui_str){s->workspace.rendered + a, b - a};
    else if (a == 0 && b == s->text_length && a != b)
        direct = cui_text_view_source(s);
    cui_text_view_copy_writer w = {0};
    cui_text_view_error error = CUI_TEXT_VIEW_OK;
    if (direct.data)
        cui_text_view_copy_write(&w, direct);
    else
        error = cui_text_view_copy_source(s, &w, a, b);
    if (error != CUI_TEXT_VIEW_OK)
        return error;
    if (length)
        *length = w.length;
    if (capacity <= w.length)
        return CUI_TEXT_VIEW_CAPACITY;
    w = (cui_text_view_copy_writer){.output = output};
    if (direct.data)
        cui_text_view_copy_write(&w, direct);
    else
        (void)cui_text_view_copy_source(s, &w, a, b);
    output[w.length] = 0;
    return CUI_TEXT_VIEW_OK;
}
