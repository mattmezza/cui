/* SPDX-License-Identifier: MIT */
#ifndef CUI_SPRING_INTERNAL_H
#define CUI_SPRING_INTERNAL_H
#include "cui/cui_theme.h"
#include <math.h>
/* Owned private arithmetic value, never a public state-layout alias. */
typedef struct {
    double position, velocity, target, updated_at, next_wake_seconds;
    bool initialized;
} cui_spring_sample_internal;
static inline void cui_spring_step_internal(cui_spring_sample_internal *s, double target,
                                            cui_spring_settings policy, double now, bool reduced) {
    s->next_wake_seconds = 0;
    if (!s->initialized || reduced || !policy.travel || policy.response == 0) {
        s->position = s->target = target;
        s->velocity = 0;
        s->updated_at = now;
        s->initialized = true;
        return;
    }
    if (s->position == target && s->velocity == 0) {
        s->target = target;
        return;
    }
    double dt = fmax(0, now - s->updated_at), w = 6.283185307179586 / policy.response,
           z = policy.damping;
    double y = s->position - s->target, v = s->velocity, position = y, velocity = v;
    if (z <= 1 && z * w * dt > 700) {
        position = velocity = 0;
    } else if (z < 1) {
        double wd = w * sqrt(1 - z * z), phase = fmod(dt, 6.283185307179586 / wd) * wd;
        double decay = exp(-z * w * dt), c = cos(phase), sn = sin(phase);
        position = decay * (y * c + (v + z * w * y) / wd * sn);
        velocity = decay * (v * c - (z * w * v + w * w * y) / wd * sn);
    } else if (z == 1) {
        double decay = exp(-w * dt), b = v + w * y;
        position = decay * (y + b * dt);
        velocity = decay * (v - w * b * dt);
    } else {
        double root = sqrt(z * z - 1), r1 = -w / (z + root), r2 = -w * (z + root);
        double a = (v - r2 * y) / (r1 - r2), b = y - a, e1 = exp(r1 * dt), e2 = exp(r2 * dt);
        position = a * e1 + b * e2;
        velocity = r1 * a * e1 + r2 * b * e2;
    }
    s->position = s->target + position;
    s->velocity = velocity;
    s->target = target;
    s->updated_at = fmax(s->updated_at, now);
    if (!isfinite(s->position) || !isfinite(s->velocity) ||
        (fabs(s->position - target) <= policy.epsilon && fabs(s->velocity) <= policy.epsilon)) {
        s->position = target;
        s->velocity = 0;
    } else
        s->next_wake_seconds = now + 1. / 60;
}
#endif
