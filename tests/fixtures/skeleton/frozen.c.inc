/* Frozen from 956a853c src/components/cui_content.c; names only remapped. */
#define cui_skeleton frozen_skeleton
#define cui_skeleton_ex frozen_skeleton_ex
static float cui_content_positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
void cui_skeleton(cui_context *ui, cui_id id, const cui_theme *t, float width, float height,
                  float radius, float phase, bool secondary, bool reduced) {
    if (!ui || !t)
        return;
    phase = isfinite(phase) ? phase - floorf(phase) : 0;
    float alpha = reduced ? 1 : .75f + .25f * cosf(phase * 6.283185307f);
    cui_color color = t->border;
    color.a = (uint8_t)(color.a * alpha * (secondary ? .5f : 1));
    cui_begin_box(ui, id,
                  (cui_box){.width = cui_content_positive(width, 160),
                            .height = cui_content_positive(height, 16),
                            .radius = isfinite(radius) && radius >= 0 ? radius : t->radius,
                            .background = color});
    cui_end_box(ui);
    if (!reduced)
        cui_invalidate(ui);
}
bool cui_skeleton_ex(cui_context *ui, cui_id id, const cui_theme *theme, cui_loading_motion *state,
                     cui_skeleton_options o, cui_loading_motion_result *out) {
    if (!ui || !id || !theme || !state || !out || !isfinite(o.width) || o.width <= 0 ||
        o.width > 4096 || !isfinite(o.height) || o.height <= 0 || o.height > 4096 ||
        !isfinite(o.radius) || o.radius < 0 || o.radius > 4096 || !isfinite(o.duration) ||
        o.duration < 0 || o.duration > 60)
        return false;
    cui_loading_motion next = *state;
    cui_loading_motion_result result;
    if (!cui_loading_motion_sample(&next, o.now_seconds, o.duration ? o.duration : 2,
                                   o.reduced_motion || cui_reduced_motion(ui), true, o.ease,
                                   o.ease_user, &result))
        return false;
    /* The reference has no loading boolean, status role, or busy relationship. */
    cui_color color = theme->secondary;
    color.a = (uint8_t)(color.a * result.opacity * (o.secondary ? .5f : 1));
    cui_begin_box(
        ui, id,
        (cui_box){.width = o.width, .height = o.height, .radius = o.radius, .background = color});
    cui_end_box(ui);
    *state = next;
    *out = result;
    return true;
}
#undef cui_skeleton
#undef cui_skeleton_ex
