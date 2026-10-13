/* SPDX-License-Identifier: MIT */
#include "cui/cui_effects.h"
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static bool effects_extent(unsigned w, unsigned h, size_t stride, size_t *extent) {
    if (!w || !h || SIZE_MAX / w < 4 || stride < (size_t)w * 4 ||
        (size_t)(h - 1) > (SIZE_MAX - (size_t)w * 4) / stride)
        return false;
    *extent = (size_t)(h - 1) * stride + (size_t)w * 4;
    return true;
}
static bool effects_buffer(cui_effect_buffer b, size_t *extent) {
    return b.pixels && effects_extent(b.width, b.height, b.stride, extent) && b.size >= *extent;
}
static bool effects_overlap(const void *a, size_t an, const void *b, size_t bn) {
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < an : x - y < bn;
}
static bool effects_pair(cui_bitmap s, cui_effect_buffer d, size_t *sn, size_t *dn) {
    if (!s.pixels || !effects_extent(s.width, s.height, s.stride, sn) || !effects_buffer(d, dn) ||
        s.width != d.width || s.height != d.height)
        return false;
    return (s.pixels == d.pixels && s.stride == d.stride) ||
           !effects_overlap(s.pixels, *sn, d.pixels, *dn);
}
static unsigned char effects_byte(double v) {
    return (unsigned char)(v <= 0 ? 0 : v >= 255 ? 255 : floor(v + 0.5));
}
bool cui_effect_linear_gradient(cui_effect_buffer d, const cui_gradient_stop *s, size_t n,
                                float angle) {
    size_t extent;
    if (!effects_buffer(d, &extent) || !s || !n || n > SIZE_MAX / sizeof(*s) || !isfinite(angle) ||
        effects_overlap(s, n * sizeof(*s), d.pixels, extent))
        return false;
    for (size_t i = 0; i < n; ++i)
        if (!isfinite(s[i].position) || s[i].position < 0 || s[i].position > 1 ||
            (i && s[i].position <= s[i - 1].position))
            return false;
    double radians = fmod((double)angle, 360.0) * 0.017453292519943295;
    double dx = cos(radians), dy = sin(radians);
    if (fabs(dx) < 1e-12)
        dx = 0;
    if (fabs(dy) < 1e-12)
        dy = 0;
    double xx = dx * (d.width - 1), yy = dy * (d.height - 1);
    double low = fmin(0, xx) + fmin(0, yy), span = fabs(xx) + fabs(yy);
    for (unsigned y = 0; y < d.height; ++y)
        for (unsigned x = 0; x < d.width; ++x) {
            double t = span ? (x * dx + y * dy - low) / span : 0;
            size_t i = 0;
            while (i + 1 < n && t > s[i + 1].position)
                ++i;
            cui_color a = s[i].color, b = a;
            double f = 0;
            if (i + 1 < n && t > s[i].position) {
                b = s[i + 1].color;
                f = (t - s[i].position) / (s[i + 1].position - s[i].position);
            }
            double alpha = a.a * (1 - f) + b.a * f;
            unsigned char *p = d.pixels + y * d.stride + (size_t)x * 4;
            p[0] = effects_byte(alpha ? (a.r * a.a * (1 - f) + b.r * b.a * f) / alpha : 0);
            p[1] = effects_byte(alpha ? (a.g * a.a * (1 - f) + b.g * b.a * f) / alpha : 0);
            p[2] = effects_byte(alpha ? (a.b * a.a * (1 - f) + b.b * b.a * f) / alpha : 0);
            p[3] = effects_byte(alpha);
        }
    return true;
}
bool cui_effect_opacity(cui_bitmap s, cui_effect_buffer d, float opacity) {
    size_t sn, dn;
    if (!isfinite(opacity) || opacity < 0 || opacity > 1 || !effects_pair(s, d, &sn, &dn))
        return false;
    for (unsigned y = 0; y < s.height; ++y)
        for (unsigned x = 0; x < s.width; ++x) {
            const unsigned char *p = s.pixels + y * s.stride + (size_t)x * 4;
            unsigned char *q = d.pixels + y * d.stride + (size_t)x * 4;
            q[0] = p[0];
            q[1] = p[1];
            q[2] = p[2];
            q[3] = effects_byte(p[3] * opacity);
        }
    return true;
}
size_t cui_effect_blur_scratch_floats(unsigned w, unsigned h) {
    if (!w || !h || (size_t)w > SIZE_MAX / h / 4 / sizeof(float))
        return 0;
    return (size_t)w * h * 4;
}
static double effects_channel(const unsigned char *p, unsigned c) {
    return c == 3 ? p[3] : p[c] * (double)p[3] / 255;
}
static unsigned effects_clamp_index(int64_t i, unsigned n) {
    return i < 0 ? 0 : (uint64_t)i >= n ? n - 1 : (unsigned)i;
}
bool cui_effect_blur(cui_bitmap s, cui_effect_buffer d, unsigned r, float *tmp, size_t count) {
    size_t sn, dn, need = cui_effect_blur_scratch_floats(s.width, s.height);
    if (!effects_pair(s, d, &sn, &dn) || !need || !tmp || (uintptr_t)tmp % _Alignof(float) != 0 ||
        count < need || effects_overlap(tmp, need * sizeof(float), s.pixels, sn) ||
        effects_overlap(tmp, need * sizeof(float), d.pixels, dn))
        return false;
    if (!r) {
        for (unsigned y = 0; y < s.height; ++y)
            memmove(d.pixels + y * d.stride, s.pixels + y * s.stride, (size_t)s.width * 4);
        return true;
    }
    double divisor = 2.0 * r + 1;
    for (unsigned y = 0; y < s.height; ++y)
        for (unsigned c = 0; c < 4; ++c) {
            const unsigned char *row = s.pixels + y * s.stride;
            double sum = effects_channel(row, c) * ((double)r + 1);
            unsigned last = r < s.width - 1 ? r : s.width - 1;
            for (unsigned i = 1; i <= last; ++i)
                sum += effects_channel(row + (size_t)i * 4, c);
            if (r > last)
                sum += effects_channel(row + (size_t)(s.width - 1) * 4, c) * (r - last);
            for (unsigned x = 0; x < s.width; ++x) {
                tmp[((size_t)y * s.width + x) * 4 + c] = (float)(sum / divisor);
                unsigned remove = effects_clamp_index((int64_t)x - r, s.width);
                unsigned add = effects_clamp_index((int64_t)x + r + 1, s.width);
                sum += effects_channel(row + (size_t)add * 4, c) -
                       effects_channel(row + (size_t)remove * 4, c);
            }
        }
    for (unsigned x = 0; x < s.width; ++x) {
        double sum[4] = {0};
        unsigned last = r < s.height - 1 ? r : s.height - 1;
        for (unsigned c = 0; c < 4; ++c) {
            sum[c] = tmp[(size_t)x * 4 + c] * ((double)r + 1);
            for (unsigned i = 1; i <= last; ++i)
                sum[c] += tmp[((size_t)i * s.width + x) * 4 + c];
            if (r > last)
                sum[c] += tmp[((size_t)(s.height - 1) * s.width + x) * 4 + c] * (r - last);
        }
        for (unsigned y = 0; y < s.height; ++y) {
            unsigned char *p = d.pixels + y * d.stride + (size_t)x * 4;
            double alpha = sum[3] / divisor;
            for (unsigned c = 0; c < 3; ++c)
                p[c] = effects_byte(alpha > 0 ? sum[c] / divisor * 255 / alpha : 0);
            p[3] = effects_byte(alpha);
            unsigned remove = effects_clamp_index((int64_t)y - r, s.height);
            unsigned add = effects_clamp_index((int64_t)y + r + 1, s.height);
            for (unsigned c = 0; c < 4; ++c)
                sum[c] += tmp[((size_t)add * s.width + x) * 4 + c] -
                          tmp[((size_t)remove * s.width + x) * 4 + c];
        }
    }
    return true;
}
