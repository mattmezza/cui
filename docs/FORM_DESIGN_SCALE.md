# Design sizes for forms and numeric controls

Styled `cui_number_input`, `cui_form`, `cui_input_group`, and `cui_otp` accept
`options.design_size` from `CUI_DESIGN_XS` through `CUI_DESIGN_9XL`.
`CUI_DESIGN_INHERIT` is zero and retains the existing `.size` behavior, so existing
zero-initialized options keep their dimensions. BASE and MD share metrics.
The explicit design scale uses `cui_design_measure` with theme font density
(`theme.font_size / 16`). NumberInput also honors its explicit `.density`.

NumberInput resolves its font, line height, padding, and height through
`cui_input_measure`. Each step button uses one control height of width; its SVG
icon uses the design indicator size. Prefix, suffix, and editor text share the
resolved font. Choose a width large enough for both step buttons and your text,
particularly for the largest sizes. The existing custom `.size` is a legacy
alternative; explicit design sizes select the shared metrics.

Form scales label and description typography and internal field spacing.
Default column gap is three design gaps; row gap is one. Positive `gap_x`,
`gap_y`, `label_text_size`, and `label_width` remain explicit overrides. The
callbacks for controls, labels, descriptions, and footer receive a borrowed theme
copy whose font, control height, padding, and gap match the chosen size. Use those
metrics in callback content instead of capturing the original theme. The copy
is valid only during the callback; the caller's theme is unchanged. A child that
also explicitly chooses a design size resolves that size using the callback
theme's font density, so avoid applying the same scale twice.

InputGroup applies the shared input metrics to its editor, addons, and loading
icon. A positive `.font_size` changes text while preserving design control height
and padding. Positive addon widths remain explicit. Inferred addon widths are
bounded to their row, and inline addons leave room for the editor. An impossible
sum of explicit widths returns `CUI_FORM_INVALID` before input or activation is
processed. Narrow groups reduce editor horizontal padding as needed to retain a
usable viewport. Addon content is clipped to its bounds; callbacks receive the
resolved width. Block-start and block-end addons have independent width budgets.

OTP scales cell dimensions, text, intra-group gap, and group gap. Positive
`cell_size`, `font_size`, `gap`, and `group_gap` override those defaults. Group gap
is three design gaps. Existing grouping, digit input, completion, masking, and
blink behavior are unchanged. Core layout stores box gaps in whole logical
pixels; SVG raster extents round up fractional design sizes.

Invalid design enums or unsupported theme density return the API's existing
invalid-argument result before reducers mutate values, drafts, or state.
Base APIs and `cui_form_plan` do not interpret the design scale or call theme
measurement functions. Styled Form resolves gaps before calling its base planner;
custom base-only compositions provide their own metrics as before.
