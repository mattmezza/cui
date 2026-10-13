# Accessibility

Controls submit an independent semantic snapshot with identity, role, accessible name, bounds, disabled/checked/focused state and scalar value. Compound controls also expose relationships, selection, scroll ranges and modal scopes. Headless tests validate named subsets of these states and keyboard interactions; they do not establish complete family acceptance.

The browser gallery projects these snapshots into DOM controls for focus and assistive technologies. Its hidden native text-input bridge supports composition for Input, Textarea, Editor and Questionnaire. Caret/selection painting, pointer positioning and Unicode grapheme/word editing have tests. Bidirectional caret geometry, full browser screen-reader testing and mobile virtual-keyboard acceptance remain unfinished. See [IME](IME.md) and [editing](EDITING.md).

Native Windows UIA, macOS AX, Linux AT-SPI, Android/iOS accessibility bridges and AccessKit integration remain unimplemented. Real screen-reader, contrast, text scaling, reduced-motion and touch-target acceptance is required before v1.0. High contrast is a token theme with tested control colors, not platform certification. See [v1 readiness](V1_READINESS.md) and [platform evidence](PLATFORM_MATRIX.md).
