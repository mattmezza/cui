# Accessibility

Controls submit an independent semantic snapshot with identity, role, accessible name, bounds, disabled/checked/focused state and scalar value. Compound controls also expose relationships, selection, scroll ranges and modal scopes. Headless tests validate named subsets of these states and keyboard interactions; they do not establish complete family acceptance.

The browser gallery projects these snapshots into DOM controls for focus and assistive technologies. Its native text-input bridge supports composition for Input, Textarea, Editor, Questionnaire, Numeric, OTP, Combobox and TimeField. Per-control filtering and history differ. Headless tests exercise caret and selection painting, pointer placement and Unicode editing. Bidirectional caret geometry, screen-reader interaction and mobile virtual keyboards still require qualification. [IME](IME.md) · [Editing](EDITING.md)

An optional Linux AccessKit adapter publishes native snapshots to AT-SPI. Independent tests use an AT-SPI client on a private display. Native Windows UIA, macOS AX and mobile accessibility bridges remain open. Real screen-reader, contrast, text scaling, reduced-motion and touch-target acceptance are separate v1.0 gates. [Platform evidence](PLATFORM_MATRIX.md) · [v1 readiness](V1_READINESS.md)

## Committed text values

Custom C controls can call `cui_set_semantic_text_value_v1` after `cui_interact`.
Pass well-formed UTF-8 with no embedded NUL. The value is copied into the
frame arena and is separate from the name, description and scalar value.
The first nonempty value allocates an index charged to the context's
`resource_bytes`, at one `cui_str` per configured element. Capacity failure
invalidates the frame. Password and secret semantics withdraw the value.

After a successful frame, call `cui_semantic_text_value_v1` with the index of
the corresponding node from `cui_semantics`. A missing value is an empty
string. Copy the borrowed text before the next frame if an adapter needs it
later. The AccessKit adapter copies into its bounded snapshot and publishes
nonempty values independently of scalar values and names.

Ordered TimeField uses `CUI_ROLE_TIME_INPUT` for the single logical focus
owner and `CUI_ROLE_SPINBUTTON` for each segment. The root value is the
localized committed time. Segments provide the committed text, discrete
number, and range. Linux AT-SPI exposes the time through the default
description when no help text was supplied. The gallery groups the
segments and exposes the active proxy as a spinbutton with its segment
name, numeric value and full-time description. The ordered semantic
snapshot exposes read-only and invalid state while editing drafts and IME
preedit remain transient. Platform value editing has its own action
contract and remains open for TimeField. [TimeField C guide](TIME_FIELD_LOCALIZED.md)
