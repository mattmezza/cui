/* SPDX-License-Identifier: MIT */
#ifndef CUI_PICKER_H
#define CUI_PICKER_H
#include "cui_base.h"
#include "cui_overlay.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    cui_date start, end;
} cui_date_selection;
typedef bool (*cui_date_disabled_fn)(void *user, cui_date date);
typedef struct {
    bool range, disabled;
    unsigned first_weekday, months; /* Sunday=0; months 1..3 */
    cui_date minimum, maximum;      /* zero means no bound */
    cui_date_disabled_fn is_disabled;
    void *disabled_user;
    float cell_size;
} cui_calendar_options;
typedef struct {
    cui_date month, cursor;
    bool selecting_end;
} cui_calendar_state;
typedef struct {
    bool changed, activated, navigated;
} cui_calendar_result;
void cui_calendar_init(cui_calendar_state *, cui_date initial);
int cui_date_compare(cui_date a, cui_date b);
/* 42 cells, including neighboring months and invalid zero at year boundaries. */
void cui_calendar_grid(cui_date month, unsigned first_weekday, cui_date output[42]);
bool cui_calendar_date_enabled(cui_date, cui_calendar_options);
cui_id cui_calendar_day_id(cui_id, cui_date);
/* navigation parts 1=previous year,2=previous month,3=next month,4=next year */
cui_id cui_calendar_navigation_id(cui_id, unsigned part);
/* Separate reduction/submission lets popups close before adding a modal scope. */
cui_calendar_result cui_calendar_reduce(cui_context *, cui_id, cui_calendar_state *,
                                        cui_date_selection *, cui_calendar_options);
void cui_calendar_submit(cui_context *, cui_id, cui_calendar_state *, cui_date_selection,
                         cui_calendar_options);
cui_calendar_result cui_base_calendar(cui_context *, cui_id, cui_calendar_state *,
                                      cui_date_selection *, cui_calendar_options);
cui_calendar_result cui_calendar(cui_context *, cui_id, const cui_theme *, cui_calendar_state *,
                                 cui_date_selection *, cui_calendar_options);
typedef struct {
    cui_overlay overlay;
    cui_calendar_state calendar;
} cui_date_picker_state;
typedef struct {
    cui_calendar_options calendar;
    cui_rect viewport;
    cui_str placeholder;
    bool disabled;
    float width;
} cui_date_picker_options;
void cui_date_picker_init(cui_date_picker_state *, cui_id popup, cui_date initial);
/* Base submits trigger semantics and updates dismissal/toggle; layout is caller-owned. */
void cui_base_date_picker_trigger(cui_context *, cui_id, cui_str, cui_date_picker_state *,
                                  bool disabled);
cui_calendar_result cui_base_date_picker(cui_context *, cui_id trigger, cui_id calendar, cui_str,
                                         cui_date_picker_state *, cui_date_selection *,
                                         cui_date_picker_options);
bool cui_date_picker(cui_context *, cui_id, const cui_theme *, cui_str, cui_date_picker_state *,
                     cui_date_selection *, cui_date_picker_options);
typedef struct {
    unsigned hour, minute, second;
} cui_time;
typedef struct {
    unsigned segment;
    unsigned digit_count, draft;
} cui_time_state;
typedef struct {
    bool seconds, twelve_hour, disabled, readonly;
} cui_time_options;
bool cui_time_valid(cui_time);
/* Strict HH:MM[:SS] 24h model format; 12h appearance affects the widget only. */
bool cui_time_parse(cui_str, cui_time *);
size_t cui_time_format(cui_time, bool seconds, char *output, size_t capacity);
cui_id cui_time_segment_id(cui_id, unsigned segment);
bool cui_base_time_field(cui_context *, cui_id, cui_str, cui_time_state *, cui_time *,
                         cui_time_options);
bool cui_time_field(cui_context *, cui_id, const cui_theme *, cui_str, cui_time_state *, cui_time *,
                    cui_time_options);
#ifdef __cplusplus
}
#endif
#endif
