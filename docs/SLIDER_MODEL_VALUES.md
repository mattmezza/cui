# Typed Slider values and math (unreleased)

Include `<cui/cui_slider_model.h>`. This C-only slice implements owned value/domain
records, validation, endpoint proposal constraints, visual mapping and state
initialization. It does not implement UI submission, reducers, requests/callbacks,
Styled presentation, bridge/binding APIs or native accessibility delivery. The
whole Slider family remains partial; the [typed model design](design/SLIDER_MODEL_ABI_V1.md)
describes those planned slices separately. Legacy float APIs are unchanged.

```c
#include <cui/cui_slider_model.h>
cui_slider_model_domain_v1 domain = {10, 1000, 5, CUI_SLIDER_MODEL_LOGARITHMIC, 0};
cui_slider_model_value_v1 confirmed = {CUI_SLIDER_MODEL_RANGE, 0, 100, 400};
cui_slider_model_value_v1 proposed;
if (cui_slider_model_value_constrain_v1(domain, confirmed,
                                       CUI_SLIDER_MODEL_START, 453, &proposed)) {
    /* proposed.start is 400: the other endpoint limits the snapped proposal.
       Application acceptance is a separate decision; confirmed was not changed. */
}
double fraction;
cui_slider_model_to_fraction_v1(domain, confirmed.end, &fraction);
cui_slider_model_state_v1 state;
cui_slider_model_state_init_v1(sizeof state, &state); /* no UI/owner binding */
```

All helpers are pure, allocation-free and synchronous. No context, theme, heap,
callback or external lifetime is retained. Failed output-producing calls preserve
all output bytes; inputs are passed by value, so output may alias original caller
value storage. Domain/value nested records use exactly the named v1 schema;
unknown tags and nonzero reserved fields reject. The state initializer alone has
a versioned top-level record: exact sizeof(v1) is required, all bytes are
initialized, version/size are set, active thumb is END and every other field is
zero. State has no pointers. Its reserved future capture/owner fields are not
usable as an implemented reducer API.

SINGLE uses end only and ignores inactive start, including NaN; constrained output
canonicalizes start to positive zero. RANGE requires two finite ordered endpoints
inside the domain, never sorting or swapping. Current values need not lie on the
step lattice. Domain requires finite minimum/maximum/step, minimum<=maximum,
finite span and nonnegative step. Linear zero span is valid; logarithmic additionally
requires 0<minimum<maximum. Infinite span is explicitly invalid even when endpoints
are finite. Validation does not snap or change caller storage.

`cui_slider_model_value_constrain_v1` validates current values and a finite
requested endpoint. SINGLE accepts END only; RANGE accepts START or END. It clamps
to the domain, rounds to the nearest **minimum-origin** step (exact midpoint goes
toward the larger value), then clamps to its neighbor. An off-grid neighbor may
win. Exact minimum/maximum stay reachable; step zero is continuous. This differs
from upstream zero-origin stepping and preserves the reviewed CUI controlled
policy. It proposes a value only; there is no implicit acceptance or event.

Snapping reduces operands separately instead of forming an overflowing quotient.
An error-free subtraction residual preserves tiny origin offsets at half-step
boundaries; fixed bounded floating expansions compare ties without a heap or a
long-double precision assumption. Arithmetic expects the normal round-to-nearest
floating environment and compiler preservation of operations (no fast-math
reassociation). Outputs are binary floating results, not arbitrary-precision
rational storage. Near-lattice rounding is tested against exact binary64 inputs
in a high-precision Decimal oracle, including subnormal offsets around midpoints.

`cui_slider_model_to_fraction_v1` and `cui_slider_model_from_fraction_v1` map an
increasing visual fraction, independently of orientation or reverse fill. Fractions
must be finite in [0,1], values inside the domain; neither extrapolates nor snaps.
Linear zero span maps its sole value to 0 and every fraction to that sole value.
Other endpoints map exactly to 0/1 and back. Logarithmic mapping uses value-ratio
logs, close-ratio log1p/expm1 and an extreme-ratio fallback; it does not divide
maximum/minimum when that would overflow, or subtract two rounded equal logs for
adjacent large values. Linear interpolation uses one fused multiply-add. Visual
log mapping does not redefine future additive keyboard steps or semantic units.

The independent tests cover invalid records/output atomicity, copied initialized
state, exact endpoints/zero span, min-origin/neighbor rules, tiny-step quotient
overflow, subnormal half-step offsets, near-maximum adjacent bounds, extreme
positive logarithmic ratios and monotonic mappings. The strict C11 source,
selective distribution and umbrella-selective fixtures plus the independent
Decimal oracle passed all four private-Xvfb tests. On the tested 64-bit Linux ABI,
value/domain/state sizes are 24/32/160 bytes. The oracle uses 1200 digits for exact
binary64 snapping/linear algebra and 180 digits for logarithms: 252 numerical
cases observed 0 ULP snapping/linear error, at most 1 ULP forward-log error,
0 ULP in sampled adjacent/subnormal inverse cases and 174 ULP worst inverse-log
error. Exact endpoints and the ±DBL_TRUE_MIN origin midpoint counterexamples
have zero-tolerance checks.

These are observations, not a proven universal error bound or a correct-rounding
promise. The chosen regression thresholds are 2 ULP for ordinary snapping/linear,
8 ULP forward-log, 1 ULP adjacent/subnormal inverse and 512 ULP other inverse-log
cases; exact cases remain zero tolerance. Worst inverse input was minimum
`0x0.0000000000001p-1022`, maximum `0x1.fffffffffffffp+1023`, fraction `0x1p-2`:
observed `0x1.6a09e667f3c7ap-550`, Decimal-rounded reference
`0x1.6a09e667f3bccp-550`. The oracle emits complete worst-case inputs/results for
future reproduction. Transcendental results are not promised bit-identical across
libm implementations. No whole-family completion is claimed.
