/* SPDX-License-Identifier: MIT */
#include "cui/cui_color_hsla_entry.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
/* Linker wrapping affects the real editor's allocations, not a public test hook. */
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static long fail_after = -1;
static bool fail(void) {
    if (fail_after < 0)
        return false;
    if (!fail_after)
        return true;
    --fail_after;
    return false;
}
void *__wrap_malloc(size_t n) {
    return fail() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t a, size_t b) {
    return fail() ? NULL : __real_calloc(a, b);
}
void *__wrap_realloc(void *p, size_t n) {
    return fail() ? NULL : __real_realloc(p, n);
}
static bool equal(cui_color_hsla a, cui_color_hsla b) {
    return a.h == b.h && a.s == b.s && a.l == b.l && a.a == b.a;
}
int main(void) {
    cui_color_hsla exact = {.1234567f, .2345678f, .3456789f, .5f};
    cui_color_hsla_entry_state s = {0};
    fail_after = 0;
    assert(cui_color_hsla_entry_init(&s, (cui_color_hsla_value){true, exact}) ==
           CUI_COLOR_HSLA_ENTRY_ALLOCATION_FAILED);
    fail_after = -1;
    assert(!s.edit && !s.draft[0]);
    assert(cui_color_hsla_entry_init(&s, (cui_color_hsla_value){true, exact}) ==
           CUI_COLOR_HSLA_ENTRY_OK);
    assert(equal(s.committed.color, exact) && equal(s.preview.color, exact));
    char display[10];
    memcpy(display, s.draft, sizeof display);
    assert(cui_color_hsla_entry_sync(&s, s.committed, false) == CUI_COLOR_HSLA_ENTRY_OK);
    assert(equal(s.committed.color, exact) && !memcmp(display, s.draft, sizeof display));
    assert(cui_color_hsla_entry_preview(&s, (cui_color_hsla){1, .9f, .8f, .7f}, false) ==
           CUI_COLOR_HSLA_ENTRY_OK);
    assert(equal(s.committed.color, exact));
    assert(cui_color_hsla_entry_restore(&s, false) == CUI_COLOR_HSLA_ENTRY_OK &&
           equal(s.preview.color, exact));
    cui_color_hsla_commit_result commit =
        cui_color_hsla_entry_commit_hex(&s, CUI_COLOR_HSLA_COMMIT_SELECT);
    assert(commit.committed && commit.close_requested && commit.value.color.a == 127 / 255.f);
    commit = cui_color_hsla_entry_commit_value(&s, exact, CUI_COLOR_HSLA_COMMIT_UPDATE, false);
    assert(commit.committed && !commit.close_requested && equal(s.committed.color, exact));
    assert(cui_color_hsla_entry_set_draft(&s, cui_string("#12")) == CUI_COLOR_HSLA_ENTRY_OK);
    assert(equal(s.preview.color, exact));
    assert(cui_color_hsla_entry_sync(&s, s.committed, false) == CUI_COLOR_HSLA_ENTRY_CONFLICT);
    assert(cui_color_hsla_entry_restore(&s, false) == CUI_COLOR_HSLA_ENTRY_CONFLICT);
    assert(cui_color_hsla_entry_commit_hex(&s, CUI_COLOR_HSLA_COMMIT_SELECT).outcome ==
           CUI_COLOR_HSLA_ENTRY_INCOMPLETE);
    unsigned failures = 0;
    for (long budget = 0; budget < 30; ++budget) {
        unsigned char snapshot[sizeof s];
        memcpy(snapshot, &s, sizeof s);
        cui_edit_state before = cui_edit_get_state(s.edit);
        fail_after = budget;
        cui_color_hsla_entry_outcome result =
            cui_color_hsla_entry_sync(&s, (cui_color_hsla_value){true, {1, 1, .5f, .5f}}, true);
        fail_after = -1;
        if (result == CUI_COLOR_HSLA_ENTRY_OK)
            break;
        assert(result == CUI_COLOR_HSLA_ENTRY_ALLOCATION_FAILED);
        assert(!memcmp(snapshot, &s, sizeof s));
        cui_edit_state after = cui_edit_get_state(s.edit);
        assert(before.revision == after.revision && before.cursor == after.cursor &&
               before.can_undo == after.can_undo);
        ++failures;
    }
    assert(failures >= 3 && s.committed.color.h == 1 && s.committed.color.a == .5f && !s.dirty);
    assert(!cui_edit_get_state(s.edit).can_undo);
    assert(cui_color_hsla_entry_commit_value(&s, (cui_color_hsla){NAN, 0, 0, 0},
                                             CUI_COLOR_HSLA_COMMIT_SELECT, true)
               .outcome == CUI_COLOR_HSLA_ENTRY_INVALID_ARGUMENT);
    commit = cui_color_hsla_entry_commit_value(&s, (cui_color_hsla){.87f, 0, 0, 1},
                                               CUI_COLOR_HSLA_COMMIT_SELECT, true);
    assert(commit.committed && commit.close_requested && s.committed.color.h == .87f);
    assert(cui_color_hsla_entry_commit_hex(&s, CUI_COLOR_HSLA_COMMIT_UPDATE).value.color.h == 0);
    assert(cui_color_hsla_entry_sync(&s, (cui_color_hsla_value){0}, false) ==
           CUI_COLOR_HSLA_ENTRY_OK);
    assert(!s.committed.present && !s.preview.present && !s.draft[0]);
    assert(cui_color_hsla_entry_sync(&s, (cui_color_hsla_value){true, {0, 0, 0, 0}}, false) ==
               CUI_COLOR_HSLA_ENTRY_OK &&
           s.committed.present);
    cui_color_hsla_entry_destroy(&s);
    cui_color_hsla_entry_destroy(&s);
    assert(cui_color_hsla_entry_restore(&s, true) == CUI_COLOR_HSLA_ENTRY_INVALID_ARGUMENT);
    return 0;
}
