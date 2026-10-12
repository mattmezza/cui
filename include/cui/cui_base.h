/* SPDX-License-Identifier: MIT */
#ifndef CUI_BASE_H
#define CUI_BASE_H
#include "cui.h"
#include "cui_edit.h"
#include "cui_platform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Submit inside a box with the same id. Base controls draw nothing. Values are
 * caller-owned and changes are applied synchronously; return true on change. */
cui_interaction cui_base_button(cui_context *, cui_id, cui_str, bool disabled);
bool cui_base_checkbox(cui_context *, cui_id, cui_str, bool *value, bool disabled);
bool cui_base_switch(cui_context *, cui_id, cui_str, bool *value, bool disabled);
bool cui_base_toggle(cui_context *, cui_id, cui_str, bool *value, bool disabled);
bool cui_base_radio(cui_context *, cui_id, cui_str, int *value, int option, bool disabled);
typedef enum { CUI_CHECK_UNCHECKED, CUI_CHECK_CHECKED, CUI_CHECK_MIXED } cui_check_state;
bool cui_base_check(cui_context *, cui_id, cui_str, cui_check_state *, bool disabled);
bool cui_base_slider(cui_context *, cui_id, cui_str, float *value, float minimum, float maximum,
                     float step, bool disabled);
typedef struct {
    bool changed, submitted;
    cui_edit_result error;
} cui_input_result;
typedef struct {
    cui_context *ui;
    cui_rect viewport;
    float font_size, scroll_x;
} cui_input_hit_geometry;
/* Reusable measured scalar hit test. Invalid input returns SIZE_MAX. */
size_t cui_base_input_hit_test(void *geometry, cui_str, float x, float y);
typedef size_t (*cui_input_hit_test_fn)(void *user, cui_str text, float x, float y);
typedef struct {
    bool dragging;
} cui_input_behavior_state;
typedef struct {
    bool disabled, readonly, required, invalid;
    const cui_platform *clipboard;  /* Optional synchronous host clipboard interface. */
    cui_input_hit_test_fn hit_test; /* UTF-8 byte offset in current text. */
    void *hit_test_user;
} cui_base_input_options;
cui_input_result cui_base_input_ex(cui_context *, cui_id, cui_str, cui_edit *,
                                   cui_input_behavior_state *, cui_base_input_options);
/* Text is owned by the editor; changed concerns text, not cursor movement. */
cui_input_result cui_base_input(cui_context *, cui_id, cui_str, cui_edit *, bool disabled);
/* Standalone allocation-free layout models. */
typedef enum { CUI_POPUP_BELOW, CUI_POPUP_ABOVE, CUI_POPUP_RIGHT, CUI_POPUP_LEFT } cui_popup_side;
typedef struct {
    cui_rect bounds;
    cui_popup_side side;
    bool flipped;
} cui_popup_result;
cui_popup_result cui_popup_place(cui_rect anchor, cui_size popup, cui_rect viewport,
                                 cui_popup_side preferred, float gap);
typedef struct {
    size_t begin, end;
    float before, after, total, scroll;
} cui_virtual_range;
/* heights must be finite and nonnegative; invalid entries are treated as zero. */
cui_virtual_range cui_virtualize(const float *heights, size_t count, float scroll, float viewport,
                                 size_t overscan);
typedef struct {
    int year, month, day;
} cui_date;
bool cui_date_valid(cui_date);
int cui_date_days_in_month(int year, int month);
int cui_date_weekday(cui_date);                     /* Sunday = 0, invalid = -1 */
cui_date cui_date_add_months(cui_date, int months); /* Clamp day; invalid -> zero */
cui_date cui_date_add_days(cui_date, int days);     /* years limited to 1..9999 */
#ifdef __cplusplus
}
#endif
#endif
