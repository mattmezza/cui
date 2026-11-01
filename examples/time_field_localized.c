/* SPDX-License-Identifier: MIT */
#include <cui/cui_time_field_v1.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    cui_time value;
    cui_time_locale_v1 locale;
    cui_time_field_options_v1 options;
    cui_time_field_state_v1 state;
    unsigned notifications;
    cui_time_change last_change;
} reminder;

static void changed(void *user, const cui_time_change *change) {
    reminder *owner = user;
    /* External owner bookkeeping is allowed. The active value/state/options,
     * theme, token storage and UI remain borrowed until compose returns. */
    ++owner->notifications;
    owner->last_change = *change; /* Owned scalar copy, no borrowed text view. */
}

static bool initialize(reminder *owner) {
    memset(owner, 0, sizeof(*owner));
    if (cui_time_locale_init_v1(&owner->locale) != CUI_TIME_TEXT_OK ||
        cui_time_field_options_init_v1(&owner->options) != CUI_TIME_TEXT_OK)
        return false;
    static const char *digits[10] = {"٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩"};
    for (unsigned i = 0; i < 10; ++i)
        owner->locale.digits[i] = cui_string(digits[i]);
    owner->locale.separator = cui_string("·");
    owner->locale.am = cui_string("ص");
    owner->locale.pm = cui_string("م");
    /* The initializer's exact period separator is one ASCII space. All14
     * tokens are nonempty, <=16 UTF-8 bytes and pairwise prefix-free. */
    owner->options.text.locale = &owner->locale;
    owner->options.text.cycle = CUI_TIME_TEXT_H12;
    owner->options.text.precision = CUI_TIME_TEXT_SECOND;
    owner->options.text.flags = CUI_TIME_TEXT_HAS_MINIMUM_V1 | CUI_TIME_TEXT_HAS_MAXIMUM_V1;
    owner->options.text.minimum = (cui_time){9, 0, 0};
    owner->options.text.maximum = (cui_time){17, 30, 0};
    owner->options.segment_names[0] = cui_string("ساعة");
    owner->options.segment_names[1] = cui_string("دقيقة");
    owner->options.segment_names[2] = cui_string("ثانية");
    owner->options.segment_names[3] = cui_string("فترة");
    /* Pure parsing is explicit: no ASCII fallback, case folding or OS locale. */
    if (cui_time_parse_v1(cui_string("٠٩·٣٠·٠٠ ص"), &owner->options.text, &owner->value) !=
        CUI_TIME_TEXT_OK)
        return false;
    return cui_time_field_state_init_v1(&owner->state, owner->value, &owner->options) ==
           CUI_TIME_TEXT_OK;
}

static void base_boxes(cui_context *ui) {
    /* Base submits semantics only. The application supplies actual boxes with
     * the same root/part IDs; this example displays all four configured parts. */
    cui_begin_box(ui, 20, (cui_box){.row = true, .gap = 4});
    for (unsigned i = 0; i < 4; ++i) {
        cui_begin_box(ui, cui_time_segment_id(20, i), (cui_box){.width = 60, .height = 32});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}

static cui_time_field_result_v1 compose(cui_context *ui, const cui_theme *theme, reminder *owner,
                                        bool styled) {
    cui_time_handler observer = {changed, owner};
    if (styled)
        return cui_time_field_v1(ui, 20, theme, cui_string("Reminder"), &owner->state,
                                 &owner->value, &owner->options, observer);
    cui_time_field_result_v1 result = cui_base_time_field_v1(
        ui, 20, cui_string("Reminder"), &owner->state, &owner->value, &owner->options, observer);
    if (result.status == CUI_TIME_TEXT_OK)
        base_boxes(ui);
    return result;
}

static bool frame(cui_context *ui, const cui_theme *theme, reminder *owner, bool styled,
                  cui_time_field_result_v1 *result) {
    if (!cui_begin_frame(ui, 640, 200, .016f))
        return false;
    cui_begin_box(ui, 100, (cui_box){.padding = 12});
    *result = compose(ui, theme, owner, styled);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    /* A real host renders draw synchronously here. Base boxes intentionally
     * have no product styling; Styled supplies visible text/background/border. */
    (void)draw;
    return result->status == CUI_TIME_TEXT_OK && cui_last_error(ui) == CUI_OK;
}

int main(int argc, char **argv) {
    bool styled = !(argc > 1 && strcmp(argv[1], "base") == 0);
    reminder owner;
    if (!initialize(&owner))
        return 1;
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 200});
    if (!ui)
        return 1;
    cui_theme theme = cui_theme_light();
    cui_time_field_result_v1 result;
    /* First submission establishes geometry; the next ordinary frame admits
     * input. Never invent a frame generation or replay the first input. */
    bool ok =
        frame(ui, &theme, &owner, styled, &result) && frame(ui, &theme, &owner, styled, &result);
    if (ok) {
        cui_focus(ui, cui_time_segment_id(20, CUI_TIME_FIELD_MINUTE_V1));
        ok = cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}) &&
             frame(ui, &theme, &owner, styled, &result);
    }
    if (ok) {
        printf("%02u:%02u:%02u changed=%d callbacks=%zu validation=%u\n", owner.value.hour,
               owner.value.minute, owner.value.second, result.changed ? 1 : 0, result.change_count,
               (unsigned)result.validation);
    }
    cui_destroy(ui);
    return ok ? 0 : 1;
}
