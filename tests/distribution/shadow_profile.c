/* SPDX-License-Identifier: MIT */
/* Copied headers only: no repository-private include path or Core implementation. */
#include "cui.h"
#define CUI_EFFECTS_IMPLEMENTATION
#include "cui_effects.h"
#include <assert.h>
int main(void) {
    cui_drop_shadow_paint p = {.caster = {0, 0, 16384, 2}};
    cui_shadow_drop_profile_internal d;
    cui_shadow_inset_profile_internal i;
    cui_shadow_profile_support_internal s;
    cui_drop_shadow_raster q;
    assert(!cui_effect_drop_shadow_query(p, 64, 0, 0, &q));
    assert(cui_shadow_drop_prepare_internal(p, 64, 0, 0, &d));
    assert(cui_shadow_drop_support_internal(&d, &s));
    assert(s.left == 0 && s.top == 0 && s.right == 1048576 && s.bottom == 128);
    assert(cui_shadow_drop_eval_internal(&d, .5f, .5f) == 1);
    p.layer.spread = 2;
    assert(cui_shadow_inset_prepare_internal(p, 1, 0, 0, &i));
    assert(cui_shadow_inset_support_internal(&i, &s));
    assert(!s.left && !s.top && !s.right && !s.bottom);
    assert(cui_shadow_inset_eval_internal(&i, .5f, .5f) == 0);
    return 0;
}
