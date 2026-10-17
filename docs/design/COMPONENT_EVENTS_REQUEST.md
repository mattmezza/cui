# Component event API: requested work

Status: requirements recorded; design checkpoint and implementation are pending.
This document defines requested work, not an implemented API or approved design.
Schedule after the current responsive, typography, video and release work reaches
a suitable checkpoint. The maintainer expects to pause this session shortly.

Before production changes, commit independently reviewable artifacts covering:

- Existing state ownership, results, callbacks, typed payloads, dispatch timing,
  handler/userdata retention and payload lifetimes for every interactive family.
- Proposed shared contract, alternatives, compatibility strategy and slices.
- Binding support distinguished from documented omissions and justified exceptions.

Scope includes buttons, inputs, checkboxes, switches, toggles, sliders,
selections, menus and dialogs. Preserve component-specific semantics; identical
interfaces and every event category on every component are not requirements.

The design must distinguish activation, proposed value requests, confirmed value
notifications, submission and lifecycle events. Preserve typed values without
untyped value pointers. Define shared metadata and handler/userdata conventions.
Acceptance/rejection, input consumption, propagation and platform default
prevention are separate decisions; none implies another. General capture/bubble
dispatch requires explicit architectural justification.

Specify synchronous callback ordering, multiplicity within a frame,
disabled/loading/inert and omitted behavior, reentrancy, callback-driven state
changes and supported removal/shutdown. Context destruction during dispatch is
unsupported unless explicitly designed otherwise. Preserve available pointer,
keyboard and accessibility evidence without fabricated metadata.

Specify payload ownership, borrowed lifetimes, safe deferred copies, allocation
bounds and whether handler/userdata are submission-borrowed or retained. Keep
convenient frame results with explicit aggregation: reading results must never
dispatch callbacks, replay mutations or repeat interactions. Applications choose
one side-effect path. Compatibility wrappers must preserve legacy timing,
ordering and mutation behavior; end-of-frame callback movement is observable.

If the design requires a new input-dispatch architecture, incompatible ownership
or breaking behavior, complete the proposal and report the architectural decision
before implementing that change. Otherwise proceed through compatible slices.

## Separate async host example

Use one explicitly documented optional host/network stack, reusing supported
dependencies where possible. Keep networking and thread pools out of Core.
Handlers enqueue URL fetches and return promptly. Demonstrate loading feedback,
duplicate prevention, worker-owned request data, synchronized completion delivery
and waking an idle UI loop. Apply UI changes and call CUI on the UI thread.
Cover failure/retry, stale responses, cancellation and shutdown with pending
work. Distinguish logical cancellation from actual transport cancellation and
test the precise guarantee with injected transport and controlled completions.
Tests require no external network. Blocking I/O in a synchronous handler still
blocks the UI thread.

## Verification and delivery

Audit legacy tests first. Add meaningful headless tests for ordering/multiplicity,
acceptance independently of consumption, callback/result coexistence, inactive
and omitted components, payload/userdata lifetimes, callback-driven mutation,
supported teardown, available input evidence and all async lifecycle cases.
Retain behavioral legacy regressions, not compilation alone.

Preserve portable C11, dependency direction and all 78 manifest entries. Update
applicable bridges, bindings, examples, human/LLM documentation, source metadata,
generated distributions and consumer skill guidance. Generate skill resources
with the existing tooling; never edit generated files by hand. Build with
warnings and use `python scripts/test_local.py --build-dir build` for Linux
native checks. Browser tests are headless; unsupported GPU checks stay deferred.

Commit completed verified slices. Final delivery includes the completed matrix,
exceptions, real API examples, compatibility/migration guidance, binding support,
tests and limitations. Do not claim uniformity from partial coverage. Workers
leave merging and pushing to the integrator.
