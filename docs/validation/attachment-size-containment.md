# Attachment size containment

The horizontal card previously used the design control height (32 pixels at Base) for two metadata rows. The subtitle exceeded that height. Fixed metadata row heights also clipped wrapped text. Intrinsic cards now fit measured children while retaining the nominal minimum height. An omitted horizontal width reserves metadata space alongside media and actions; explicit widths remain caller constraints and text wraps within them.

AttachmentGroup with an omitted height follows the actual card height. The gallery uses a metric-scaled card/action width and an uncapped preview height for vertical image cards. Horizontal groups deliberately scroll when their cards exceed the viewport; an offscreen action is available by scrolling rather than squeezing its label.

`test_media.c` checks painted text containment with long wrapping metadata, image, progress and action across all 14 design sizes, four legacy sizes, two font densities and both orientations. A second fixture checks actual group height after layout settles, and actual touch scrolling. Existing status/retry/disabled tests remain in place. Explicit caller heights can still clip content intentionally.

`test_attachment_sizes.py` checks actual WASM draw text against card and active clip bounds across all design sizes, densities and orientations. Full-card checks use a sufficiently wide viewport; a separate narrower viewport checks real subtitle pixels and CDP touch scrolling. This is Chromium evidence, not physical Android or iOS coverage.

Validation: warning-enabled native build; 95/96 CTest checks pass, with only the unchanged generated-distribution freshness check failing on this isolated source branch. ASAN media and media-base-only: 2/2 pass. Chromium headless-shell 1228: 62/62 strict cases pass, including 390px Small/Base both densities, actual subtitle pixels, and touch pan exposing the largest card's Open action followed by exactly one activation. The old pre-fix fixture fails its text-bottom/card-bottom assertion. Chromium 1194 full-browser startup SIGTRAP was a host failure; 1228 runs normally.

Run the browser proof with `uv run tests/web/test_attachment_sizes.py --chromium /path/to/chrome-headless-shell --report /path/to/report.json`. The report's sibling PNG captures the compact Base card at 390px. The test defaults to strict failure reporting.

The optional existing `test_touch_components.py` sweep renders all 78 stories and passes 39/42 cases on this branch. Its Button, Checkbox and Sheet cases fail the helper's viewport-placement check before contact dispatch; no Attachment case fails. This audit does not certify those unrelated interactions.
