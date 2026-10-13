/* SPDX-License-Identifier: MIT */
#include "cui/cui_plot_view.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

cui_plot_axis_options cui_plot_axis_defaults(float width, float height) {
    return (cui_plot_axis_options){.width = width, .height = height, .x_line = true};
}
float cui_plot_axis_gutter(float size) {
    return isfinite(size) && size > 0 ? size + 8 : 0;
}
static bool axis_valid(const cui_plot_axis_options *o) {
    if (!o || !isfinite(o->width) || !isfinite(o->height) || o->width <= 0 || o->height <= 0 ||
        o->width > 16384 || o->height > 16384 || !isfinite(o->x) || !isfinite(o->y) ||
        o->x_count > CUI_PLOT_AXIS_LIMIT || o->y_count > CUI_PLOT_AXIS_LIMIT ||
        (o->x_count && !o->x_labels) || (o->y_count && !o->y_labels) ||
        o->x_side > CUI_PLOT_AXIS_START || o->y_side > CUI_PLOT_AXIS_START ||
        o->x_side < CUI_PLOT_AXIS_END || o->y_side < CUI_PLOT_AXIS_END)
        return false;
    for (size_t k = 0; k < 2; ++k) {
        const cui_plot_axis_text *labels = k ? o->y_labels : o->x_labels;
        size_t count = k ? o->y_count : o->x_count;
        for (size_t i = 0; i < count; ++i)
            if ((!labels[i].text.data && labels[i].text.length) || !isfinite(labels[i].tick) ||
                fabsf(labels[i].tick) > 16384 || !isfinite(labels[i].font_size) ||
                labels[i].font_size < 0 || labels[i].font_size > 1024 ||
                labels[i].align < CUI_ALIGN_START || labels[i].align > CUI_ALIGN_END)
                return false;
    }
    return fabsf(o->x) <= 16384 && fabsf(o->y) <= 16384;
}
cui_plot_result cui_plot_axis_layout(cui_context *ui, const cui_plot_axis_options *o,
                                     cui_plot_axis_label_geometry *out, size_t capacity,
                                     size_t *required) {
    if (!ui || !required || (!out && capacity) || !axis_valid(o))
        return CUI_PLOT_INVALID;
    cui_plot_axis_label_geometry geometry[CUI_PLOT_AXIS_LIMIT * 2];
    size_t used = 0;
    for (size_t k = 0; k < 2; ++k) {
        if (!(k ? o->has_y : o->has_x))
            continue;
        const cui_plot_axis_text *labels = k ? o->y_labels : o->x_labels;
        size_t count = k ? o->y_count : o->x_count;
        for (size_t i = 0; i < count; ++i) {
            float size = labels[i].font_size ? labels[i].font_size : 10;
            cui_size measured = cui_measure_text(ui, labels[i].text, size);
            if (!isfinite(measured.width) || !isfinite(measured.height) || measured.width < 0 ||
                measured.height < 0 || measured.width > 16384 || measured.height > 16384)
                return CUI_PLOT_INVALID;
            float x = k ? o->y + (o->y_side == CUI_PLOT_AXIS_START ? -2 : 2) : labels[i].tick;
            float y = k ? labels[i].tick - size / 2
                        : o->x + (o->x_side == CUI_PLOT_AXIS_START ? -(size + 4) : 6);
            if (labels[i].align == CUI_ALIGN_CENTER)
                x -= measured.width / 2;
            if (labels[i].align == CUI_ALIGN_END)
                x -= measured.width;
            geometry[used++] =
                (cui_plot_axis_label_geometry){{x, y, measured.width, measured.height}, i, !k};
        }
    }
    *required = used;
    if (capacity < used)
        return CUI_PLOT_CAPACITY;
    if (used)
        memcpy(out, geometry, used * sizeof *out);
    return CUI_PLOT_OK;
}
static cui_color resolved(cui_color color, cui_color fallback) {
    return color.a ? color : fallback;
}
static void axis_paint(cui_context *ui, cui_id id, const cui_theme *theme,
                       const cui_plot_axis_options *o, const cui_plot_axis_label_geometry *geometry,
                       size_t count) {
    cui_color stroke = resolved(o->stroke, theme->border);
    if (o->has_x && o->x_line) {
        cui_begin_absolute_box(ui, id ^ UINT64_C(0xa1), (cui_box){.background = stroke},
                               (cui_rect){0, o->x - .5f, o->width, 1});
        cui_end_box(ui);
    }
    if (o->has_y && o->y_line) {
        cui_begin_absolute_box(ui, id ^ UINT64_C(0xa2), (cui_box){.background = stroke},
                               (cui_rect){o->y - .5f, 0, 1, o->height});
        cui_end_box(ui);
    }
    for (size_t i = 0; i < count; ++i) {
        const cui_plot_axis_text *label = geometry[i].horizontal ? o->x_labels + geometry[i].index
                                                                 : o->y_labels + geometry[i].index;
        cui_begin_absolute_box(ui, id ^ UINT64_C(0x789ae632a981c000) ^ (i + 1), (cui_box){0},
                               geometry[i].bounds);
        cui_text(ui, label->text, label->font_size ? label->font_size : 10,
                 resolved(label->color, theme->muted));
        cui_end_box(ui);
    }
}
cui_plot_result cui_plot_axis(cui_context *ui, cui_id id, const cui_theme *theme,
                              const cui_plot_axis_options *o) {
    if (!theme || !id)
        return CUI_PLOT_INVALID;
    cui_plot_axis_label_geometry geometry[512];
    size_t count;
    cui_plot_result result = cui_plot_axis_layout(ui, o, geometry, 512, &count);
    if (result != CUI_PLOT_OK)
        return result;
    cui_begin_box(
        ui, id,
        (cui_box){
            .width = o->width, .height = o->height, .fixed_width = true, .fixed_height = true});
    axis_paint(ui, id, theme, o, geometry, count);
    cui_end_box(ui);
    return CUI_PLOT_OK;
}
cui_plot_result cui_plot_axis_ticks(const cui_plot_scale *scale, size_t desired,
                                    cui_plot_numeric_tick *out, size_t capacity, size_t *required) {
    if (!required || (!out && capacity) || desired > CUI_PLOT_AXIS_LIMIT)
        return CUI_PLOT_INVALID;
    double ticks[1024];
    size_t count;
    cui_plot_result result = cui_plot_ticks(scale, desired, ticks, 1024, &count);
    if (result != CUI_PLOT_OK)
        return result;
    if (count > CUI_PLOT_AXIS_LIMIT) {
        *required = count;
        return CUI_PLOT_CAPACITY;
    }
    double pixels[256];
    for (size_t i = 0; i < count; ++i) {
        result = cui_plot_map(scale, ticks[i], pixels + i);
        if (result != CUI_PLOT_OK)
            return result;
        if (fabs(pixels[i]) > 16384)
            return CUI_PLOT_INVALID;
    }
    *required = count;
    if (capacity < count)
        return CUI_PLOT_CAPACITY;
    for (size_t i = 0; i < count; ++i) {
        snprintf(out[i].text, sizeof out[i].text, "%.17g", ticks[i]);
        out[i].value = ticks[i];
        out[i].label = (cui_plot_axis_text){
            .text = cui_string(out[i].text), .tick = (float)pixels[i], .align = CUI_ALIGN_CENTER};
    }
    return CUI_PLOT_OK;
}

typedef struct {
    cui_plot_line line;
    bool closed;
} plot_segment;
static bool point_valid(cui_plot_point p) {
    return isfinite(p.x) && isfinite(p.y) && fabs(p.x) <= 16384 && fabs(p.y) <= 16384;
}
static cui_plot_result segments(const cui_plot_shape *shape, plot_segment *lines, size_t *count) {
    if (!shape || shape->count > 4096 || (shape->count && !shape->commands) ||
        !isfinite(shape->stroke_width) || shape->stroke_width < 0 || shape->stroke_width > 1024)
        return CUI_PLOT_INVALID;
    size_t used = 0, begin = 0;
    cui_plot_point previous = {0}, start = {0};
    bool active = false;
    for (size_t i = 0; i < shape->count; ++i) {
        const cui_plot_path_command *c = shape->commands + i;
        if (c->kind < CUI_PLOT_PATH_MOVE || c->kind > CUI_PLOT_PATH_CLOSE)
            return CUI_PLOT_INVALID;
        if (c->kind != CUI_PLOT_PATH_CLOSE && !point_valid(c->point))
            return CUI_PLOT_INVALID;
        if (c->kind == CUI_PLOT_PATH_MOVE) {
            previous = start = c->point;
            active = true;
            begin = used;
            continue;
        }
        if (!active)
            return CUI_PLOT_INVALID;
        if (c->kind == CUI_PLOT_PATH_CLOSE) {
            if (used == 4096)
                return CUI_PLOT_CAPACITY;
            lines[used++] = (plot_segment){{previous, start}, true};
            for (size_t j = begin; j < used; ++j)
                lines[j].closed = true;
            previous = start;
            active = false;
            continue;
        }
        if (c->kind == CUI_PLOT_PATH_CUBIC) {
            if (!point_valid(c->control1) || !point_valid(c->control2))
                return CUI_PLOT_INVALID;
            cui_plot_point from = previous;
            for (size_t j = 1; j <= 16; ++j) {
                if (used == 4096)
                    return CUI_PLOT_CAPACITY;
                double t = (double)j / 16, u = 1 - t;
                cui_plot_point next = {u * u * u * from.x + 3 * u * u * t * c->control1.x +
                                           3 * u * t * t * c->control2.x + t * t * t * c->point.x,
                                       u * u * u * from.y + 3 * u * u * t * c->control1.y +
                                           3 * u * t * t * c->control2.y + t * t * t * c->point.y};
                lines[used++] = (plot_segment){{previous, next}, false};
                previous = next;
            }
        } else {
            if (used == 4096)
                return CUI_PLOT_CAPACITY;
            lines[used++] = (plot_segment){{previous, c->point}, false};
            previous = c->point;
        }
    }
    *count = used;
    return CUI_PLOT_OK;
}
static void blend(unsigned char *p, cui_color c) {
    double a = c.a / 255., old = p[3] / 255., alpha = a + old * (1 - a);
    if (alpha == 0) {
        memset(p, 0, 4);
        return;
    }
    p[0] = (unsigned char)lround((c.r * a + p[0] * old * (1 - a)) / alpha);
    p[1] = (unsigned char)lround((c.g * a + p[1] * old * (1 - a)) / alpha);
    p[2] = (unsigned char)lround((c.b * a + p[2] * old * (1 - a)) / alpha);
    p[3] = (unsigned char)lround(alpha * 255);
}
cui_plot_result cui_plot_raster(unsigned char *pixels, unsigned width, unsigned height,
                                size_t stride, size_t capacity_bytes, const cui_plot_shape *shapes,
                                size_t count, cui_color clear) {
    if (!pixels || !width || !height || width > 1024 || height > 1024 ||
        stride < (size_t)width * 4 || stride > SIZE_MAX / height || count > 64 ||
        (count && !shapes))
        return CUI_PLOT_INVALID;
    if (capacity_bytes < stride * height)
        return CUI_PLOT_CAPACITY;
    plot_segment lines[4096];
    size_t used, total = 0;
    for (size_t i = 0; i < count; ++i) {
        cui_plot_result result = segments(shapes + i, lines, &used);
        if (result != CUI_PLOT_OK)
            return result;
        total += used;
    }
    if (total && (size_t)width * height > 64000000 / total)
        return CUI_PLOT_CAPACITY;
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x) {
            unsigned char *p = pixels + y * stride + x * 4;
            p[0] = clear.r;
            p[1] = clear.g;
            p[2] = clear.b;
            p[3] = clear.a;
        }
    for (size_t i = 0; i < count; ++i) {
        (void)segments(shapes + i, lines, &used);
        for (unsigned y = 0; y < height; ++y)
            for (unsigned x = 0; x < width; ++x) {
                double px = x + .5, py = y + .5;
                bool inside = false, stroke = false;
                for (size_t j = 0; j < used; ++j) {
                    cui_plot_point a = lines[j].line.start, b = lines[j].line.end;
                    if (lines[j].closed && ((a.y > py) != (b.y > py)) &&
                        px < (b.x - a.x) * (py - a.y) / (b.y - a.y) + a.x)
                        inside = !inside;
                    if (shapes[i].stroke.a && shapes[i].stroke_width > 0) {
                        double dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
                        double t =
                            length ? fmax(0, fmin(1, ((px - a.x) * dx + (py - a.y) * dy) / length))
                                   : 0;
                        if (hypot(px - a.x - t * dx, py - a.y - t * dy) <=
                            shapes[i].stroke_width / 2)
                            stroke = true;
                    }
                }
                unsigned char *p = pixels + y * stride + x * 4;
                if (inside && shapes[i].fill.a)
                    blend(p, shapes[i].fill);
                if (stroke)
                    blend(p, shapes[i].stroke);
            }
    }
    return CUI_PLOT_OK;
}
cui_plot_result cui_plot_view(cui_context *ui, cui_id id, const cui_theme *theme,
                              const cui_plot_view_options *o) {
    if (!ui || !id || !theme || !o || (!o->name.data && o->name.length) ||
        !cui_bitmap_valid(o->bitmap) || o->bitmap.width > 16384 || o->bitmap.height > 16384)
        return CUI_PLOT_INVALID;
    cui_plot_axis_label_geometry geometry[512];
    size_t labels = 0;
    if (o->axis) {
        if (o->axis->width != o->bitmap.width || o->axis->height != o->bitmap.height)
            return CUI_PLOT_INVALID;
        cui_plot_result result = cui_plot_axis_layout(ui, o->axis, geometry, 512, &labels);
        if (result != CUI_PLOT_OK)
            return result;
    }
    cui_rect bounds = cui_bounds(ui, id);
    cui_begin_box(ui, id,
                  (cui_box){.width = (float)o->bitmap.width,
                            .height = (float)o->bitmap.height,
                            .fixed_width = true,
                            .fixed_height = true});
    (void)cui_interact(ui, id, CUI_ROLE_IMAGE, o->name, o->disabled, false, 0);
    cui_set_semantic_details(
        ui, id, (cui_semantic_details){.pointer_activatable = o->interactive, .no_focus = true});
    bool painted = cui_paint_bitmap(ui, id ^ UINT64_C(0xe18a5902),
                                    (cui_bitmap_paint){.bitmap = o->bitmap, .opacity = 1},
                                    (cui_box){.width = (float)o->bitmap.width,
                                              .height = (float)o->bitmap.height,
                                              .fixed_width = true,
                                              .fixed_height = true});
    if (o->axis)
        axis_paint(ui, id, theme, o->axis, geometry, labels);
    cui_end_box(ui);
    if (painted) {
        size_t count;
        const cui_event *events = cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; ++i)
            if (o->interactive && !o->disabled && events[i].type == CUI_EVENT_POINTER_UP &&
                cui_event_activation(ui, i) == id) {
                if (o->on_activate)
                    o->on_activate(ui, id,
                                   (cui_plot_point){events[i].x - bounds.x, events[i].y - bounds.y},
                                   o->user);
                (void)cui_consume_event(ui, i);
            }
    }
    return painted ? CUI_PLOT_OK : CUI_PLOT_CAPACITY;
}

cui_plot_result cui_plot_grid(cui_context *ui, cui_id id, const cui_theme *theme,
                              const cui_plot_grid_options *o, cui_color stroke) {
    if (!ui || !id || !theme)
        return CUI_PLOT_INVALID;
    cui_plot_rect quads[4096];
    size_t count;
    cui_plot_result result = cui_plot_grid_layout(o, quads, 4096, &count);
    if (result != CUI_PLOT_OK)
        return result;
    cui_begin_box(ui, id,
                  (cui_box){.width = (float)o->width,
                            .height = (float)o->height,
                            .fixed_width = true,
                            .fixed_height = true});
    stroke = resolved(stroke, theme->border);
    for (size_t i = 0; i < count; ++i) {
        cui_plot_rect q = quads[i];
        cui_begin_absolute_box(ui, id ^ UINT64_C(0x891ac3714f982000) ^ (i + 1),
                               (cui_box){.background = stroke},
                               (cui_rect){(float)q.x_min, (float)q.y_min,
                                          (float)(q.x_max - q.x_min), (float)(q.y_max - q.y_min)});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return CUI_PLOT_OK;
}

cui_plot_dot_options cui_plot_dot_defaults(void) {
    return (cui_plot_dot_options){.size = 4, .use_theme_fill = true};
}
static bool mark_extent(cui_size extent) {
    return isfinite(extent.width) && isfinite(extent.height) && extent.width > 0 &&
           extent.height > 0 && extent.width <= 16384 && extent.height <= 16384;
}
static bool dot_options(cui_plot_dot_options o) {
    return isfinite(o.size) && o.size >= 0 && o.size <= 1024;
}
static void dot_paint(cui_context *ui, cui_id id, const cui_theme *theme,
                      cui_plot_dot_geometry geometry, cui_plot_dot_options o) {
    cui_color fill = o.use_theme_fill ? theme->primary : o.fill;
    cui_color border = o.custom_stroke ? o.stroke : fill;
    cui_begin_absolute_box(ui, id,
                           (cui_box){.background = fill,
                                     .border = border,
                                     .border_width = 1,
                                     .radius = (float)geometry.radius},
                           (cui_rect){(float)geometry.frame.x_min, (float)geometry.frame.y_min,
                                      (float)o.size, (float)o.size});
    cui_end_box(ui);
}
cui_plot_result cui_plot_dots(cui_context *ui, cui_id id, const cui_theme *theme, cui_size extent,
                              const cui_plot_sample *samples, size_t count,
                              cui_plot_dot_options o) {
    if (!ui || !id || !theme || !mark_extent(extent) || !dot_options(o) || count > 4096 ||
        (count && !samples))
        return CUI_PLOT_INVALID;
    cui_plot_dot_geometry geometry;
    for (size_t i = 0; i < count; ++i)
        if (samples[i].defined) {
            if (!point_valid((cui_plot_point){samples[i].x, samples[i].y1}))
                return CUI_PLOT_INVALID;
            cui_plot_result result = cui_plot_dot_layout(
                (cui_plot_point){samples[i].x, samples[i].y1}, o.size, &geometry);
            if (result != CUI_PLOT_OK)
                return result;
        }
    cui_begin_box(ui, id,
                  (cui_box){.width = extent.width,
                            .height = extent.height,
                            .fixed_width = true,
                            .fixed_height = true});
    for (size_t i = 0; i < count; ++i)
        if (samples[i].defined) {
            (void)cui_plot_dot_layout((cui_plot_point){samples[i].x, samples[i].y1}, o.size,
                                      &geometry);
            dot_paint(ui, id ^ UINT64_C(0x721dd761552a0000) ^ (i + 1), theme, geometry, o);
        }
    cui_end_box(ui);
    return CUI_PLOT_OK;
}
static cui_plot_point radial_dot(cui_plot_radial_sample sample, cui_size extent) {
    double angle = fmod(sample.angle, 2 * acos(-1.));
    return (cui_plot_point){extent.width / 2 + sample.radius1 * sin(angle),
                            extent.height / 2 - sample.radius1 * cos(angle)};
}
cui_plot_result cui_plot_radial_dots(cui_context *ui, cui_id id, const cui_theme *theme,
                                     cui_size extent, const cui_plot_radial_sample *samples,
                                     size_t count, cui_plot_dot_options o) {
    if (!ui || !id || !theme || !mark_extent(extent) || !dot_options(o) || count > 4096 ||
        (count && !samples))
        return CUI_PLOT_INVALID;
    cui_plot_dot_geometry geometry;
    for (size_t i = 0; i < count; ++i)
        if (samples[i].defined) {
            if (!isfinite(samples[i].angle) || !isfinite(samples[i].radius1) ||
                samples[i].radius1 < 0)
                return CUI_PLOT_INVALID;
            cui_plot_point point = radial_dot(samples[i], extent);
            if (!point_valid(point))
                return CUI_PLOT_INVALID;
            cui_plot_result result = cui_plot_dot_layout(point, o.size, &geometry);
            if (result != CUI_PLOT_OK)
                return result;
        }
    cui_begin_box(ui, id,
                  (cui_box){.width = extent.width,
                            .height = extent.height,
                            .fixed_width = true,
                            .fixed_height = true});
    for (size_t i = 0; i < count; ++i)
        if (samples[i].defined) {
            (void)cui_plot_dot_layout(radial_dot(samples[i], extent), o.size, &geometry);
            dot_paint(ui, id ^ UINT64_C(0x7397d895aac70000) ^ (i + 1), theme, geometry, o);
        }
    cui_end_box(ui);
    return CUI_PLOT_OK;
}
cui_plot_bar_options cui_plot_bar_defaults(void) {
    return (cui_plot_bar_options){.alignment = CUI_PLOT_BAR_BOTTOM, .use_theme_fill = true};
}
cui_plot_result cui_plot_bars(cui_context *ui, cui_id id, const cui_theme *theme, cui_size extent,
                              const cui_plot_bar_sample *samples, size_t count,
                              const cui_plot_bar_options *o) {
    if (!ui || !id || !theme || !o || !mark_extent(extent) || count > 4096 || (count && !samples) ||
        !isfinite(o->band_width) || o->band_width < 0 || o->band_width > 16384 ||
        o->alignment < CUI_PLOT_BAR_BOTTOM || o->alignment > CUI_PLOT_BAR_RIGHT)
        return CUI_PLOT_INVALID;
    float corners[] = {o->corners.top_left, o->corners.top_right, o->corners.bottom_left,
                       o->corners.bottom_right};
    for (size_t i = 0; i < 4; ++i)
        if (!isfinite(corners[i]) || corners[i] < 0 || corners[i] > 1024)
            return CUI_PLOT_INVALID;
    cui_plot_bar_geometry geometry;
    for (size_t i = 0; i < count; ++i)
        if (samples[i].has_cross && samples[i].has_value) {
            if (!isfinite(samples[i].cross) || !isfinite(samples[i].baseline) ||
                !isfinite(samples[i].value) || fabs(samples[i].cross) > 16384 ||
                fabs(samples[i].baseline) > 16384 || fabs(samples[i].value) > 16384)
                return CUI_PLOT_INVALID;
            cui_plot_result result =
                cui_plot_bar_layout(o->alignment, samples[i].cross, o->band_width,
                                    samples[i].baseline, samples[i].value, &geometry);
            if (result != CUI_PLOT_OK)
                return result;
            if (fabs(geometry.frame.x_max) > 16384 || fabs(geometry.frame.y_max) > 16384)
                return CUI_PLOT_INVALID;
        }
    cui_begin_box(ui, id,
                  (cui_box){.width = extent.width,
                            .height = extent.height,
                            .fixed_width = true,
                            .fixed_height = true});
    for (size_t i = 0; i < count; ++i)
        if (samples[i].has_cross && samples[i].has_value) {
            (void)cui_plot_bar_layout(o->alignment, samples[i].cross, o->band_width,
                                      samples[i].baseline, samples[i].value, &geometry);
            cui_color fill = o->use_theme_fill ? theme->primary : o->fill;
            if (o->fill_for)
                fill = o->fill_for(samples + i, i, geometry.frame, o->alignment, o->user);
            cui_begin_absolute_box(
                ui, id ^ UINT64_C(0x61498ec3a7980000) ^ (i + 1),
                (cui_box){.background = fill, .corner_radii = o->corners, .custom_corners = true},
                (cui_rect){(float)geometry.frame.x_min, (float)geometry.frame.y_min,
                           (float)(geometry.frame.x_max - geometry.frame.x_min),
                           (float)(geometry.frame.y_max - geometry.frame.y_min)});
            cui_end_box(ui);
        }
    cui_end_box(ui);
    return CUI_PLOT_OK;
}
