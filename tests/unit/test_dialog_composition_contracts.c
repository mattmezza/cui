/* SPDX-License-Identifier: MIT */
#include "dialog_composition_helpers.h"
static unsigned total(const dialog_fixture *f) {
    unsigned n = 0;
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i)
        n += f->calls[i];
    return n;
}
static void rejected_before_callbacks(bool styled) {
    dialog_fixture f;
    dialog_fixture_init(&f, styled, NULL);
    f.composition.present = 1u << CUI_DIALOG_BODY;
    f.metric[CUI_DIALOG_BODY] = 800;
    f.poison = true; /* helper permits intentional Core error; callback must not run */
    f.composition.omit |= 1u << CUI_DIALOG_BODY;
    dialog_fixture_frame(&f);
    assert(cui_last_error(f.ui) != CUI_OK && total(&f) == 0);
    f.composition.omit &= ~(1u << CUI_DIALOG_BODY);
    cui_clear_error(f.ui);
    f.poison = false;
    dialog_fixture_frame(&f);
    assert(f.calls[CUI_DIALOG_BODY] == 1);
    /* Zero and negative available widths are explicit atomic rejection. */
    for (unsigned i = 0; i < 2; ++i) {
        f.width = i ? 31 : 32;
        f.poison = true;
        dialog_fixture_frame(&f);
        assert(cui_last_error(f.ui) != CUI_OK && total(&f) == 0);
        cui_clear_error(f.ui);
    }
    f.width = 400;
    f.poison = false;
    dialog_fixture_frame(&f);
    assert(f.calls[CUI_DIALOG_BODY] == 1);
    dialog_fixture_destroy(&f);
}
static void omitted_and_empty(bool styled) {
    dialog_fixture f;
    dialog_fixture_init(&f, styled, NULL);
    f.sizing.mode = CUI_SCROLL_BODY_AUTO;
    f.sizing.requested_height = 0;
    f.composition.present = 1u << CUI_DIALOG_BODY;
    /* Explicit empty FLEX body remains an authored callback and allocation. */
    dialog_fixture_frame(&f);
    assert(f.calls[CUI_DIALOG_BODY] == 1);
    assert(dialog_semantic(&f, cui_surface_part_id(50, 6)));
    /* Omitted body has no callback/scroll semantic; storage survives end_frame. */
    f.composition.present = 0;
    f.composition.omit |= 1u << CUI_DIALOG_BODY;
    dialog_fixture_frame(&f);
    assert(!f.calls[CUI_DIALOG_BODY]);
    assert(!dialog_semantic(&f, cui_surface_part_id(50, 6)));
    f.composition.omit &= ~(1u << CUI_DIALOG_BODY);
    f.composition.present = 1u << CUI_DIALOG_BODY;
    f.metric[CUI_DIALOG_BODY] = 900;
    dialog_fixture_frame(&f);
    assert(f.calls[CUI_DIALOG_BODY] == 1 && f.surface.body_scroll == 0);
    dialog_fixture_destroy(&f);
}
static void title_references(bool styled) {
    for (unsigned mode = 0; mode < 5; ++mode) {
        dialog_fixture f;
        dialog_fixture_init(&f, styled, NULL);
        f.composition.name = mode == 1 || mode == 2 ? cui_string("Named owner") : (cui_str){0};
        f.composition.omit = 1u << CUI_DIALOG_DESCRIPTION;
        if (mode == 1)
            f.composition.omit |= 1u << CUI_DIALOG_TITLE;
        if (mode >= 3)
            f.composition.present = 1u << CUI_DIALOG_TITLE;
        if (mode == 4)
            f.composition.slots[CUI_DIALOG_TITLE] = (cui_dialog_slot){0};
        f.metric[CUI_DIALOG_TITLE] = 20;
        dialog_fixture_frame(&f);
        const cui_semantic *owner = dialog_semantic(&f, 50);
        const cui_semantic *title = dialog_semantic(&f, cui_surface_part_id(50, 4));
        assert(owner);
        bool expected_title = mode >= 2;
        assert((title != NULL) == expected_title);
        assert(owner->details.labelled_by == (expected_title ? cui_surface_part_id(50, 4) : 0));
        if (title)
            assert(title->role == CUI_ROLE_LABEL);
        assert(f.calls[CUI_DIALOG_TITLE] == (mode == 3 ? 1u : 0u));
        /* Independently validate every published nonzero label reference. */
        size_t count;
        const cui_semantic *semantics = cui_semantics(f.ui, &count);
        for (size_t j = 0; j < count; ++j)
            if (semantics[j].details.labelled_by)
                assert(dialog_semantic(&f, semantics[j].details.labelled_by));
        dialog_fixture_destroy(&f);
    }
}
int main(void) {
    title_references(false);
    title_references(true);
    rejected_before_callbacks(false);
    rejected_before_callbacks(true);
    omitted_and_empty(false);
    omitted_and_empty(true);
    puts("Dialog descriptor/width/omission recovery passed");
    return 0;
}
