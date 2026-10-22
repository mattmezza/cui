# Stepper connector and custom-icon checkpoint

Base: `56220821`. Frozen reference: `87d10ae5e1299d1be18670a37570c5402f952e34`.
This proposal changes no implementation yet.

The frozen `crates/component/src/stepper/item.rs` renders a separator after every
item except the last. Its primary color depends on `item_index < selected_index`,
not an explicit complete/error status. Horizontal separators run between indicator
centers with a four-pixel clearance from each indicator; vertical separators run
below each indicator. Width is 1.5/2/3 for XSmall/default/Large; indicator diameter
is 8/18/24/32 for XSmall/Small/default/Large. The centered-text variant has distinct
relative positioning. `trigger.rs` chooses a caller icon instead of the ordinal,
including for passed/current steps; XSmall suppresses both. Selected and passed
indicators use primary background; disabled suppresses activation, not that color.
Arbitrary children and style refinements remain separate source capabilities.

Current `src/components/cui_numeric.c::cui_numeric_stepper_paint` renders titled
buttons and explicit status/description text, without indicators or connectors.
`cui_stepper_navigation` calls the Base request reducer once, then that paint seam.
Its request callbacks run after semantic registration and before paint, retain
captured selection, and report each accepted activation. Legacy mutation and
aggregate CHANGE APIs share the old painter. None need a dispatch change.

Proposed additive surface: new `cui/cui_stepper_visual.h`, leaving `cui_step` and
all existing function signatures/layouts unchanged. A typed `cui_stepper_icon`
contains a `cui_bitmap` plus a presence flag; a present bitmap replaces the
ordinal, with monochrome tint supplied by the indicator foreground. The caller
may prepare it using the existing Icon API, or supply another valid bitmap.
No SVG parser, platform type, asset loader or retained application callback is
added. Bitmap bytes remain borrowed through frame rendering, following existing
bitmap paint lifetime; descriptor arrays/theme/steps are stable through the call.
An absent icon uses the ordinal. Invalid present bitmap fails preflight before
semantics/input callbacks; no silent icon fallback.

`cui_stepper_visual_options` contains vertical and disabled flags, a typed
XS/SM/MD/LG density enum, and optional icons pointer/count. Icons are either absent
with zero count, or exactly one descriptor per step (at most100). This first slice
uses non-centered source composition; centered text and arbitrary source style/
children remain explicit omissions. No resizing of legacy options is required.
`cui_stepper_visual_navigation(ui,id,theme,steps,count,selected,options,handler)`
returns `cui_stepper_navigation_result`. It validates the entire visual descriptor
before invoking the existing Base reducer exactly once, then paints captured
selection. Existing navigation ownership, accepted-input eligibility, callback
order, and no-reentry/descriptor-mutation rules remain authoritative. Existing
legacy entry points and their pixels remain unchanged. A new façade opts into
source-like indicator/connector composition rather than changing old stories.

Connectors are paint-only, nonfocusable and do not generate semantic progress or
requests. Item trigger IDs remain `cui_numeric_part_id(id,1+i)`; decorative IDs
must not overlap this routing range. Passed coloring is index-derived, independent
of caller status. Explicit errors retain a visible status label; this is a CUI
extension, not an upstream source rule. Labels/descriptions remain semantic names
and descriptions, and icon decoration does not replace them. Horizontal layout
is content-sized with bounded nonnegative connector spacers; vertical layout uses
an indicator/title row followed by a short connector before the next item. Exact
responsive flex growth, centered relative separators and source pixel density
are not promised by this bounded slice.

Verification will inspect emitted geometry and software pixels for horizontal/
vertical connectors, primary/border passed states, no trailing connector, custom
icon vs ordinal, XS suppression, alpha/tint and disabled appearance. Independent
expected rectangles/colors avoid copying painter logic into the oracle. Input
vectors cover pointer/Enter/Space, same-index and multiple requests, disabled
items/global disable, invalid descriptors with no callbacks/semantics, and captured
selection despite application owner updates. Existing legacy navigation/numeric
tests remain green. Strict native tests use the private Xvfb wrapper; sanitizer,
C++ header and standalone/amalgamation/package tests cover the new public entry.
SDK/distribution/skill generation and binding omission metadata follow production;
no binding or full Stepper parity claim is made.

Approved production refinement: the optional `design_size` member uses the shared
`cui_design_size` XS..9XL range. Zero preserves source-density geometry; explicit
sizes use shared font/gap and scale source indicator/separator/connector geometry
by the resolved indicator size divided by16. All tokens and invalid preflight are
covered independently of the frozen four source densities. This is app sizing,
not a claim of exact source style or responsive centered composition.
