/* SPDX-License-Identifier: MIT */
#include "cui/cui_numeric.h"
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
bool cui_number_parse(cui_str text, double *value) {
    if (!value || !text.data || !text.length || text.length > 127)
        return false;
    size_t i = 0, digits = 0;
    if (text.data[i] == '+' || text.data[i] == '-')
        ++i;
    while (i < text.length && text.data[i] >= '0' && text.data[i] <= '9') {
        ++i;
        ++digits;
    }
    if (i < text.length && text.data[i] == '.') {
        ++i;
        while (i < text.length && text.data[i] >= '0' && text.data[i] <= '9') {
            ++i;
            ++digits;
        }
    }
    if (!digits)
        return false;
    if (i < text.length && (text.data[i] == 'e' || text.data[i] == 'E')) {
        ++i;
        if (i < text.length && (text.data[i] == '+' || text.data[i] == '-'))
            ++i;
        size_t start = i;
        while (i < text.length && text.data[i] >= '0' && text.data[i] <= '9')
            ++i;
        if (i == start)
            return false;
    }
    if (i != text.length)
        return false;
    const char *radix = localeconv()->decimal_point;
    size_t radix_length = strlen(radix);
    if (!radix_length || radix_length > 16)
        return false;
    char adapted[144];
    size_t n = 0;
    for (i = 0; i < text.length; ++i) {
        if (text.data[i] == '.') {
            memcpy(adapted + n, radix, radix_length);
            n += radix_length;
        } else
            adapted[n++] = text.data[i];
    }
    adapted[n] = 0;
    char *end = NULL;
    errno = 0;
    double parsed = strtod(adapted, &end);
    if (!end || *end || !isfinite(parsed) || (errno == ERANGE && parsed == 0))
        return false;
    *value = parsed;
    return true;
}
size_t cui_number_format(double value, char *out, size_t capacity) {
    if (!isfinite(value)) {
        if (out && capacity)
            out[0] = 0;
        return 0;
    }
    char buffer[96];
    int written = snprintf(buffer, sizeof(buffer), "%.17g", value);
    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        if (out && capacity)
            out[0] = 0;
        return 0;
    }
    const char *radix = localeconv()->decimal_point;
    size_t radix_length = strlen(radix), length = 0;
    for (size_t i = 0; i < (size_t)written;) {
        char c = buffer[i];
        if (radix_length && !strncmp(buffer + i, radix, radix_length)) {
            c = '.';
            i += radix_length;
        } else
            ++i;
        if (out && length + 1 < capacity)
            out[length] = c;
        ++length;
    }
    if (out && capacity)
        out[length < capacity ? length : capacity - 1] = 0;
    return length;
}
bool cui_number_step(double *value, int direction, cui_number_options o) {
    if (!value || !isfinite(*value) || !direction || o.disabled || o.readonly)
        return false;
    if ((o.has_minimum && !isfinite(o.minimum)) || (o.has_maximum && !isfinite(o.maximum)) ||
        (o.has_minimum && o.has_maximum && o.minimum > o.maximum))
        return false;
    double step = o.step_by ? o.step_by(o.step_user, *value, direction < 0 ? -1 : 1) : o.step;
    if (!isfinite(step) || step <= 0)
        return false;
    double before = *value;
    if (direction < 0 && o.has_minimum && before <= o.minimum)
        return false;
    if (direction > 0 && o.has_maximum && before >= o.maximum)
        return false;
    double after = direction < 0 ? before - step : before + step;
    if (o.has_minimum && after < o.minimum)
        after = o.minimum;
    if (o.has_maximum && after > o.maximum)
        after = o.maximum;
    if (!isfinite(after) || after == before)
        return false;
    *value = after;
    return true;
}
size_t cui_pagination_items(size_t current, size_t total, unsigned radius, cui_page_item out[11]) {
    if (!out || !total)
        return 0;
    if (current < 1)
        current = 1;
    if (current > total)
        current = total;
    if (radius > 3)
        radius = 3;
    size_t candidates[9], count = 0;
    candidates[count++] = 1;
    size_t begin = current > radius ? current - radius : 1;
    size_t end = total - current < radius ? total : current + radius;
    for (size_t page = begin;; ++page) {
        if (page != 1 && page != total)
            candidates[count++] = page;
        if (page == end)
            break;
    }
    if (total != 1)
        candidates[count++] = total;
    size_t n = 0, previous = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t page = candidates[i];
        if (previous && page - previous == 2)
            out[n++] = (cui_page_item){.page = previous + 1};
        else if (previous && page - previous > 2)
            out[n++] = (cui_page_item){.ellipsis = true};
        out[n++] = (cui_page_item){.page = page};
        previous = page;
    }
    return n;
}
