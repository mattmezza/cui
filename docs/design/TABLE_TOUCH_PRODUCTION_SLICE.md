# Production Table touch source checkpoint

Base: exact integrated32dfddb3, branch `feat/table-touch-admission` in the reused
worker tree. Historical Dialog/lifecycle proof remains preserved separately at
`feat/dialog-composition-current` / c0dc4b96. The original checkpoint was uncompiled; subsequent verification is recorded below.

Only production changes: additive read-only `cui_event_hovered` in Core/header;
existing `touch_drag_priority = !disabled && column.movable` on Table headers;
indexed query for TOUCH drop admission while retaining the literal mouse
`cui_hovered` branch. No marker, scope cancellation, contact mutation, Dialog
component, foreign facade, renderer or other reducer transfer. No context field,
public record/config growth, new allocator, retained graph or dispatcher.

The query transfers the corrected modal and same-root rules proved in historical
c0dc4b96, with production private names. Current modal membership is determined
from the target's prior full-ID ancestry independently of caller stack. Existing
retained nearest layout semantic parents distinguish ordinary descendants from
later interactive siblings. Higher layers/paint roots still block descendants.
Old `cui_hovered` code remains unchanged.

Planned focused validation against THIS exact current Core/Table, without any
lifecycle prototype:

- `event_hover_source` and `event_hover_distribution`: public behavior fixture,
  ordinary same-root overlap geometry, current/retained modal transitions,
  descendant/passive/popup/absolute controls, invalid/nonfinite/disabled/phase
  cases. Same-TU test-only read-only witnesses inspect actual retained plane and
  ancestry; poison witness restores event coordinates. No private struct mirror.
- Eight `table_touch_*` source modes and selective/umbrella equivalents: both
  directions, later mouse sample, tap sort/subthreshold, disabled/loading/column
  disabled, pinned boundary, body/nonmovable-header pan with actual scroll-offset
  change, multicontact cancellation and genuine fresh recovery.
- `table_touch_legacy`: byte-frozen exact base Table, renamed symbols only,
  independent contexts, fieldwise state/model/result/callback comparisons over31
  frames each. Ordinary consumed mouse/key/CANCEL behavior and disabled/loading
  transitions remain covered. Frozen source SHA is recorded separately.
- Existing Table base/events/selection tests and applicable Core/header/ABI
  compatibility checks remain necessary. Canonical SDK/amalgamation/consumer
  skill generation and required installed-skill tests follow review.

The test-only Core witness includes exact production source (or generated Core
implementation for the distribution case), and introduces no public declaration.
The standalone Table touch fixture uses only ordinary production Core/Table.
Umbrella distribution compilation shares the existing test implementation target.
No complete Dialog, general raw-consumer lifecycle, bindings or GPU claim.

## First production verification

Approved source f179c8a0 compiled against exact production Core/Table without the
lifecycle prototype. Session5083 passed strict compilation/linking of source,
query witness, generated Core, selective Table and complete umbrella consumers.
Source/fixtures used C11 O3/Werror; scoped generated implementation consumers
used O0/Werror to bound optimizer work. C++11 signature and existing80/144-byte
record checks passed. Approved cached archives supplied unchanged modules;
explicit actual Core/Table objects supplied this slice's behavior.

All36 native/C++ runtime checks passed via private-Xvfb CTest. Initial40 total
cases passed39: only the offline shared-umbrella guard lacked the new approved
consumer allowlist entry. The exact two-line guard/fixture-list repair passed4/4
without recompilation or production edits. Other offline usage/header/V1 ABI
checks passed. Canonical SDK/dist/skill generation and freshness checks passed.
The integrator ran the required 18-test installed skill suite on revision
139695bc: all passed in 42.274 seconds. All nine recorded source/test hashes
match the compiled worker receipt. See the validation receipt.
No CMake full-library rebuild, desktop window, dependency install or second tree.
