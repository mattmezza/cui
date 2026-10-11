# Accessibility

Controls submit an independent semantic snapshot with identity, role, accessible name, bounds, disabled/checked/focused state and scalar value. Headless tests verify behavior and semantic state. Keyboard paths work independently of theme: Tab/reverse Tab focus, Enter/Space activate, arrows adjust sliders, and modal scope trapping/dismissal uses the core event router.

This is not yet OS screen-reader integration. Windows UIA, macOS AX, Linux AT-SPI, Android and iOS native bridges and AccessKit integration are unimplemented. Compound roles/relations, mixed checkbox semantic state, progress roles, label associations and full radio-group roving focus also remain gaps. Do not describe the framework as accessible on the strength of pixel painting or internal semantics alone.

The preview Input intentionally discloses missing caret/selection paint, pointer positioning and IME. The editor currently moves by Unicode scalar values, so combining sequences and emoji clusters need further work. High contrast is a token theme with tested control colors, not a platform accessibility certification.
