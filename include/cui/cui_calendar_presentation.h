/* SPDX-License-Identifier: MIT */
#ifndef CUI_CALENDAR_PRESENTATION_H
#define CUI_CALENDAR_PRESENTATION_H
#include "cui_picker.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Additive opt-ins; zero is exactly the legacy Calendar path. No clock access.
 * Invalid/zero today omits decoration. Selected paint takes precedence; disabled
 * today retains muted foreground. Today does not affect eligibility/selection.
 * Month toggle applies only to a single displayed month (months 0 or 1).
 * Locale strings and policy/user resources retain cui_picker.h's borrowed
 * lifetimes; this value-only record is not retained. No new event dispatch or
 * typed observer bridge is implied. Existing CHANGE observes selection only. */
typedef struct {
    bool month_toggle;
    cui_date today;
} cui_calendar_presentation_options;
/* Stable additional header ID, independent of the legacy year header (part 5).
 * Submitted only when month_toggle is effective. */
cui_id cui_calendar_month_toggle_id(cui_id root);
cui_calendar_result cui_calendar_reduce_ex(cui_context *, cui_id, cui_calendar_state *,
                                           cui_date_selection *, cui_calendar_options,
                                           cui_calendar_presentation_options);
void cui_calendar_submit_ex(cui_context *, cui_id, cui_calendar_state *, cui_date_selection,
                            cui_calendar_options, cui_calendar_presentation_options);
cui_calendar_result cui_base_calendar_ex(cui_context *, cui_id, cui_calendar_state *,
                                         cui_date_selection *, cui_calendar_options,
                                         cui_calendar_presentation_options);
cui_calendar_result cui_calendar_ex(cui_context *, cui_id, const cui_theme *, cui_calendar_state *,
                                    cui_date_selection *, cui_calendar_options,
                                    cui_calendar_presentation_options);
#ifdef __cplusplus
}
#endif
#endif
