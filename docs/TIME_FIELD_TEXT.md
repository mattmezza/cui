# TimeField text values (unreleased)

`cui/cui_time_text.h` provides pure C locale-aware time parsing, formatting and
inclusive same-day bounds validation. These helpers do not change or localize
the existing TimeField widget. Its legacy parser/formatter and typed aggregate
CHANGE observer remain available; see [TimeField events](TIME_FIELD_EVENTS.md).
No foreign binding exposes these records yet.

Initialize `cui_time_text_options_v1` with
`cui_time_text_options_init_v1`; NULL locale selects ASCII digits, colon and
uppercase AM/PM separated by one space. Select H23/H12 and minute/second
precision explicitly. Parsing consumes exactly two digit tokens per numeric
segment and the entire input. H12 requires the configured period; no whitespace
trimming or case folding occurs. Minute precision rejects nonzero seconds in
values/active bounds; it never silently truncates the model. Bounds are inclusive;
minimum greater than maximum is invalid configuration, not an overnight range.

A custom locale supplies ten digits, separator, period separator, AM and PM as
borrowed `cui_str` tokens. Each token is1..16 UTF-8 bytes, contains no NUL, and
all14 must be pairwise prefix-free. Locale/options/token storage stays immutable
through each synchronous call; nothing is retained. No locale registry, OS
lookup, allocation, IME composition or Unicode normalization is provided.

```c
#include <cui/cui_time_text.h>

static bool parse_reminder(cui_str text, cui_time *reminder) {
    cui_time_text_options_v1 options;
    if (cui_time_text_options_init_v1(&options) != CUI_TIME_TEXT_OK)
        return false;
    options.flags = CUI_TIME_TEXT_HAS_MINIMUM_V1 | CUI_TIME_TEXT_HAS_MAXIMUM_V1;
    options.minimum = (cui_time){9, 0, 0};
    options.maximum = (cui_time){17, 30, 0};
    return cui_time_parse_v1(text, &options, reminder) == CUI_TIME_TEXT_OK;
}
```

Parser failure leaves the output unchanged. Formatter output is atomic too:
query required bytes with NULL output/capacity0, then provide at least
required+1 bytes. Length excludes NUL. Insufficient capacity returns
BUFFER_TOO_SMALL and reports required bytes without touching the buffer;
other failures also leave the required-length output unchanged. Writable outputs
must not overlap each other, records or borrowed strings. Maximum formatted text
is160 bytes plus NUL. See the public header and
[reviewed contract](design/TIME_FIELD_COMPLETION.md) for exact failure precedence.

This is a partial pure-value slice. New editor typing/navigation, validation
appearance, localized widget/DateTimePicker presentation, all five bindings,
IME and host accessibility remain separate work.
