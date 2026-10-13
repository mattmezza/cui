# Date locales and direct year selection

Calendar and DatePicker now expose an interactive year header. Activate it to
choose from a 20-year grid (four columns, five rows), page backward/forward in
20-year steps, choose a year, then a month and an actual day. Selecting the year
or month navigates without changing the selected date. A date in 1980 takes
three year-page clicks from the gallery's 2026 starting year, instead of hundreds
of month steps. The final page ends at 9999 and has one blank cell; year zero is
never selectable. Navigation uses icon chevrons, not text `<`/`>` characters.

The gallery adds a Date locale selector for Calendar and DatePicker. Its default
is ISO (legacy). Built-in locale tables provide English US, English UK, Italian,
French, German, Spanish and Japanese month/weekday labels, week starts, numeric
date ordering and accessible control labels. US starts on Sunday and formats
MM/DD/YYYY; UK/Italian/French/Spanish start on Monday and format DD/MM/YYYY;
German uses DD.MM.YYYY; Japanese starts on Sunday and uses YYYY/MM/DD.
The dates remain Gregorian model values, independent of presentation.

## Public API

Choose an explicit borrowed locale in Calendar options:

```c
cui_calendar_options options = {
    .locale = cui_date_locale_builtin(CUI_DATE_LOCALE_IT)
};
cui_calendar(ui, calendar_id, &theme, &calendar_state, &selection, options);
```

Or supply the same options to DatePicker:

```c
cui_date_picker_options options = {
    .calendar = {.locale = cui_date_locale_builtin(CUI_DATE_LOCALE_EN_GB)},
    .viewport = {0, 0, width, height}
};
cui_date_picker(ui, trigger_id, &theme, cui_string("Date"), &picker_state,
                &selection, options);
```

A NULL locale preserves the existing ISO trigger/day labels and caller's
`first_weekday` (including zero-initialized Sunday). With an explicit locale, its
week start wins unless `first_weekday_override` is true. For example, set the
flag plus `.first_weekday = 6` for Saturday. Existing zero initializers and
Calendar state initialization remain valid.

`cui_date_format_locale(date, locale, buffer, capacity)` returns the required
byte length excluding NUL, and NUL-terminates byte truncation when capacity is
small (a truncated custom UTF-8 separator may end mid-codepoint). Capacity
zero may use a NULL buffer. An invalid date returns zero and clears a supplied
buffer. `cui_date_locale_builtin()` returns NULL for an unknown enum value.

Custom `cui_date_locale` tables and their strings are caller-owned and borrowed.
Keep them alive while options reference them. Supply UTF-8 `months[12]` and
`weekdays[7]` (Sunday-indexed), week start, numeric `order` and a literal
`separator`. Missing labels fall back to English. Calendar, year/month choice
and four navigation labels can also be customized. A zero order is ISO; supported
other orders are month/day/year, day/month/year and year/month/day. The separator
is literal text, not a strftime/ICU pattern. This API does not parse localized
input, discover OS locales, handle alternative calendar systems or guarantee
fonts covering every label. Supply suitable fonts for Japanese/custom scripts.

## Interaction, constraints and focus

Year and month cells use stable `cui_calendar_year_id()` and
`cui_calendar_month_id()` IDs. The year header is navigation part 5; existing
parts 1–4 retain their month/year behavior in day view and parts 1/4 page the
active year grid. Calendar state records the view, year page and keyboard cursor.
Initialize state with `cui_calendar_init()` or `cui_date_picker_init()`.

Arrows move by one cell horizontally or four cells vertically in the grids.
Home/End move to the page's first/last eligible entry. PageUp/PageDown change the
year page by 20 years, or change the displayed year in month view. In day view,
PageUp/PageDown change month (Ctrl/Meta changes year); existing arrow and
Ctrl/Meta-arrow behavior remains available. Enter/Space use ordinary control
activation. Focus follows the selected year, month and first enabled day.
Escape returns from a year/month view to days; in DatePicker, a second Escape
closes the popup and restores trigger focus.

Disabled controls cannot change view or selection. A year/month is enabled only
if it contains at least one date passing disabled/minimum/maximum/filter rules.
Date filters remain caller callbacks and can be consulted repeatedly when
checking a grid; keep them pure and inexpensive. Range selection is unchanged:
only actual enabled day activation starts/completes a range, and the popup closes
after the second day. Hard boundaries 1 and 9999 remain respected.

## Verification

`test_picker.c` covers all seven built-in formats and week starts, explicit week
start overrides, custom UTF-8 separators/labels, safe truncation, ISO fallback,
actual year/month/day controls, leap-day 1980 selection, min/max/filter and
disabled rejection, range selection, keyboard navigation, focus transitions,
Escape and year boundaries. Existing Picker coverage is retained.

Run the strict real browser suite after building the gallery:

```sh
uv run tests/web/test_date_locale_year.py --directory build/web \
  --chromium /path/to/chromium
```

It exercises all seven locales in both Calendar and DatePicker with mouse and
actual Chromium CDP touch (28 cases), checks centered/equal-width year grids,
localized month and weekday ordering, actual selected model value 19800229,
localized trigger paint/accessibility, popup closing and two-stage Escape.
It uses configuration DOM controls and real component input, never direct
reducer calls. Screenshots/reports are written to `/tmp/cui-date-locale-year`.
This does not establish physical mobile device, OS IME or every font coverage.

Warning-enabled native build and the two documented API examples compiled. Full
CTest passed 93/94; only root-owned generated distribution was stale pending
regeneration. Picker/base-only passed ASAN/UBSAN (2/2). The strict locale/year
suite passed 28/28, existing touch coverage 42/42 and 16/16, and the full browser
suite rendered all 78 stories, compiled all 78 excerpts and passed its 390 px
layout and hosted-path checks.
