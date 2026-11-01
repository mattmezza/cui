# Controlled Slider copied-wire conformance follow-up

Source audit of the signed Go/Node/Python subset at `b4f66a31`, plus uncompiled
Rust candidate in `bindings/rust/src/slider_model.rs`. Root separately owns the
finite-span/strict-log fixes and their acceptance gates. This checkpoint does not
edit those languages, claim uniform conformance, or replace the C schema.

## Authority and observed differences

The approved [bridge schema](BRIDGE_SLIDER_MODEL_V1.md#validation-and-atomicity)
(lines218–228) defines explicit vocabulary mapping, absent input allzero, absent
frame0/indexUINT64_MAX and inactive numeric value0. The actual producer is
`src/bridge/cui_bridge_slider_internal.h`:365–366 initializes every byte to0;
:545–547 copies accessibility_value only with HAS_NUMERIC_VALUE. Public
`include/cui/cui_bridge.h`:233 declares input flags, :300 the numeric field; it
does not itself repeat all canonical-zero prose. Treat the signed schema plus
actual producer as evidence, not a guessed metadata rule.

| Copied output invariant | Python `_copy_request` | JS/TS `copyRequest` | Go `sliderReadRequest` | Rust candidate `read_request/read_input` |
|---|---|---|---|---|
| Exact request/receipt schema+sizes, reserved value/domain fields | Rejects | Native addon rejects before copied JS arrays | Rejects | Rejects; static size/offset assertions |
| Unknown request/input vocabularies and masks | Rejects active input vocabularies; absent-input fields currently bypass vocabulary check | Rejects | Rejects active input; absent-input allzero guard | Rejects active input; absent-input allzero guard |
| HAS_INPUT absent ⇒ every input field0 | **Missing** | Rejects | Rejects | Rejects |
| repeat only0/1, finite x/y | **Missing** | Rejects | Rejects | Rejects |
| finite numeric value even without numeric flag | Only tested when numeric flag set | Rejects | Rejects | Rejects |
| HAS_NUMERIC_VALUE absent ⇒ numeric value0 | **Missing** | **Missing** | **Missing** | Rejects; `inactive_numeric` provider/private test prepared |
| Numeric flag ⇒ input present, typeACCESSIBILITY, actionSET_VALUE | Rejects | Rejects | Rejects | Rejects |
| HAS_SOURCE absent ⇒ sourceUNKNOWN0 | **Missing** | Rejects | Rejects | Rejects |
| Absent frame0 / indexUINT64_MAX | Rejects | Rejects | Rejects | Rejects |
| Queue namespace established before publishing a receipt | Python real C poll rejects before enable; conversion accepts None namespace, which accept later rejects | `appNamespaces.has` rejects before publication | Explicit queue namespace guard before C poll | Explicit queue namespace guard before C poll |
| Full u64 identities and no C views over callbacks | Python int / frozen copies | BigInt / frozen copied arrays+records | uint64 / value copies | u64 / owned copies; pending actual runtime |

Exact decoder seams: `bindings/python/cui_bridge/slider_model.py`:325–343;
`bindings/node/slider-model.cjs`:73–88 with native layout checks in
`bindings/node/slider_model_internal.h`:336–343;
`bindings/go/cui/native.go`:3775–3822. TS shares the JS runtime, rather than having
an independent decoder. Rust's candidate uses the same complete optional group;
its records and tests remain unverified until the allocated compile/runtime gate.

All domain adapters must also enforce the actual portable math contract in
`src/base/cui_slider_model.c`:6–10: finite maximum-minus-minimum, with logarithmic
minimum>0 and maximum>minimum. Merely finite individual bounds or positive equal
log bounds are insufficient. Root has aligned Python, JS/TS and Go domain validation with this contract;
Rust candidate validates both authoring and copied domains before C use, pending runtime acceptance.

## Small compatible follow-up

1. Add shared named negative modes to each existing protocol fixture, without
   changing public records, callback timing, native delivery or C producer:
   `absent_input`, `repeat`, `coordinates`, `nonfinite_numeric`,
   `inactive_numeric`, `absent_source`, and existing absent frame/index cases.
   For inactive numeric, use input-present, numeric-flag-absent and numeric=.25;
   include an accessibility INCREMENT/DECREMENT example, which the signed schema
   explicitly canonicalizes. A second keyboard case proves the actual producer's
   wider zero-initialization invariant.
2. Tighten the Python copied decoder for the proven canonical checks. Establish
   an explicit pre-enable conversion policy: prefer reject before publication,
   matching JS/Go/Rust. Existing acceptance already rejects None authority; do
   not describe the current gap as forging acceptable receipts.
3. Add inactive-numeric rejection to JS/TS and Go; keep C-group lookup and legacy
   dispatch unchanged. Synthetic errors must publish no new request/receipt or
   callback and preserve previously copied records. JS tests must verify that
   no WeakMap receipt authority is installed on a failed copy.
4. Run actual current UI acceptance/rejection and old-module/provider cases in
   distinct processes, plus existing affected legacy suites. Preserve unknown
   metadata as unavailable; never normalize it to mouse/key or synthesize values.

Rust candidate already contains the inactive-numeric check and negative mode;
all runtime claims await its own proof. The follow-up table should be revised
from actual landed/tested source, not checked complete because tests were drafted.

## Deliberately not inferred

The producer also emits type-specific zeros for unused key/pointer/accessibility
fields. The current schema and wrappers do not provide one uniform, independently
tested matrix of every such inactive combination. Audit that separately before
adding broader constraints. Likewise, validating current against domain or CANCEL
requested against gesture_domain is distinct from validating each owned value
and domain record; do not accidentally reject legitimate old-domain CANCEL pairs.
None of these copied-wire checks establishes physical native AT, typed SET host
delivery, V2 gradient support or whole-family completion.
