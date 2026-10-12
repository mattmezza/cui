/* SPDX-License-Identifier: MIT */
#include "cui/cui_status.h"
#include <math.h>
cui_tag_options cui_tag_options_default(void) {
    return (cui_tag_options){.variant = CUI_TAG_SECONDARY, .size = CUI_STATUS_MEDIUM};
}
cui_marker_options cui_marker_options_default(void) {
    return (cui_marker_options){.width = 480, .duration = 2, .shimmer_spread = .3f};
}
bool cui_base_marker(cui_marker_options o, const cui_marker_child *children, size_t count,
                     cui_marker_plan *out) {
    if (!out || (count && !children) || count > CUI_MARKER_MAX_CHILDREN ||
        o.variant < CUI_MARKER_PLAIN || o.variant > CUI_MARKER_BORDER ||
        o.alignment < CUI_MARKER_ALIGN_DEFAULT || o.alignment > CUI_MARKER_ALIGN_END ||
        o.loading_style < CUI_MARKER_SPINNER || o.loading_style > CUI_MARKER_SHIMMER ||
        !isfinite(o.width) || o.width <= 0 || !isfinite(o.now_seconds) || o.now_seconds < 0 ||
        !isfinite(o.duration) || o.duration < 0 || !isfinite(o.shimmer_spread) ||
        o.shimmer_spread < 0)
        return false;
    bool icon = false, content = false;
    for (size_t i = 0; i < count; i++) {
        if (children[i].kind < CUI_MARKER_ICON || children[i].kind > CUI_MARKER_DIRECT ||
            !isfinite(children[i].width) || children[i].width < 0 ||
            (children[i].text.length && !children[i].text.data))
            return false;
        icon |= children[i].kind == CUI_MARKER_ICON;
        content |= children[i].kind == CUI_MARKER_TEXT || children[i].kind == CUI_MARKER_CONTENT;
    }
    double duration = fmax(o.duration, .001), raw = o.now_seconds / duration;
    if (!isfinite(raw))
        return false;
    float phase = (float)(o.once ? fmin(raw, 1) : fmod(raw, 1));
    bool animated = o.loading && !o.reduced_motion &&
                    (o.loading_style == CUI_MARKER_SPINNER ? !icon : content) &&
                    !(o.once && raw >= 1);
    cui_marker_plan p = {
        .alignment = o.alignment == CUI_MARKER_ALIGN_DEFAULT
                         ? (o.variant == CUI_MARKER_SEPARATOR ? CUI_MARKER_ALIGN_CENTER
                                                              : CUI_MARKER_ALIGN_START)
                         : o.alignment,
        .spinner = o.loading && o.loading_style == CUI_MARKER_SPINNER && !icon,
        .animated = animated,
        .phase = o.reverse ? 1 - phase : phase,
        .rich_opacity = o.loading && !o.reduced_motion && o.loading_style == CUI_MARKER_SHIMMER
                            ? .6f + .4f * (float)(.5 + .5 * cos(phase * 6.283185307179586))
                            : 1,
        .next_wake_seconds = animated ? o.now_seconds + 1.0 / 60 : 0};
    *out = p;
    return true;
}
