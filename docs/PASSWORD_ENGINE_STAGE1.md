# Password engine prerequisite, Stage 1

This batch adds pure U+2022 scalar masking/offset helpers and hard presentation
preparation to `cui_edit_apply_checked`. It supplies no password Input widget,
secure semantics, clipboard policy, browser/native secure bridge or reveal UI.
Those remain coordinated Stage 2 work. The source mask character/scalar behavior
comes from GPUI Kit `87d10ae5e1299d1be18670a37570c5402f952e34`,
[masked display](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/element.rs#L2548).

`cui_edit_mask_text` emits three UTF-8 bytes per Unicode scalar, including combining
marks and each scalar of a ZWJ/flag sequence. It is not grapheme counting. Capacity
includes NUL; whole-source UTF8, checked scalar multiplication and known
source/destination/output aliases precede writes. Failure preserves destination
and output. Mask/unmask offset helpers require original scalar boundaries or
three-byte bullet boundaries; outputs are atomic and disjoint from the source.
Storage remains caller-owned, and helper outputs borrow caller bytes. Ordinary
edit movement/grapheme rules are unchanged by these pure mappings.

A checked request may provide `cui_edit_presentation`: synchronous `prepare`, user
pointer/exact byte extent, optional descriptor extent and zero to three exact
caller workspaces. All declared spans must be accurately sized, disjoint from
each other/model/session/original request payload. Unused array entries are zero;
non-NULL user requires nonzero extent. Invalid configuration is rejected before
callback. Allocated/private callback resources are caller-owned too, but need no
span declaration when freshly allocated disjoint storage; they must never alias
model storage. Direct caller-buffer mutation, undeclared aliases, retained candidate
pointers, external side effects and cross-thread access violate the borrowing
contract and cannot be globally protected in C.

Preparation receives complete real candidate text (after opt-in normalization),
not bullets. It returns only OK, CAPACITY or appended PRESENTATION; other codes
become INVALID_ARGUMENT. Use CAPACITY for allocator/fixed-budget failure and
PRESENTATION for an installed geometry factory failing to create a representation.
No factory or frontend allocator is called by the engine itself. The callback is
responsible for cleaning partial resources before returning a failure. Same-model
mutators/void destroy/cache-writing getters are guarded; attempted mutation causes
INVALID_ARGUMENT and preserves logical state. Pure getters remain available.

The gate runs before acceptance predicates and model mutation. Resource failure
is `rejected=false` and does not invoke acceptance or invalid-old healing.
Successful preparation is followed by the original candidate predicate, and by
the previous-value predicate only when candidate fails. Empty acceptance bypass
remains; hard preparation still applies to an actual edit resulting in empty text.
An unchanged no-op range has no candidate/stage. No callback replay occurs during
commit. Zero presentation preserves the legacy path; hard presentation currently
rejects multiline models explicitly, while ordinary multiline APIs stay unchanged.

Replacement prepares its normalized full candidate before cursor/history/document
writes. IME UPDATE prepares the full prospective composed value, and COMMIT
prepares once before document commit, with original UTF8 endpoint validation and
normalization mapping unchanged. Failed preparation restores reused prior view
bytes and frees only unbound engine grow storage. Preedit/document/session remain
unchanged. START/CANCEL do not prepare: frontend must retain a safe committed-view
representation before starting composition so cancel can publish it without a
later allocation. Stage 1 does not implement that frontend lifecycle itself.
Undo/Redo prepare the target recorded full value before saving history state or
changing index/selection. They bypass acceptance/normalization as before. A hard
budget may block replay with CAPACITY without discarding its history entry.

Returned `presentation_prepared` means prepare returned OK, even if a later
predicate/reentry failure rejected the operation. `presentation_applied` means the
checked operation adopted that candidate. Consume/publish or discard caller staging
before another request. No completion callback runs inside the engine; publication,
cache release and other frontend effects lie outside engine rollback. All borrowed
presentation fields are removed/restored when the checked call returns. Do not
promise unchanged external callback state or entire-allocation identity: rollback
preserves logical document/history/selection/coalescing/session ownership and prior
view defined bytes/NUL, while internal scratch and unused tails are unspecified.

`tests/unit/test_password_engine.c` covers independently enumerated scalar maps,
alias/capacity/output rollback, prepare→predicate ordering and invalid-old healing,
normalized candidate, unsupported callback codes, same-model destroy, fixed-budget
and injected preparation/allocator failures, exact replay/IME prepare counts,
typing grouping across failure, prior fixed/growing view ownership, three declared
workspaces/descriptor, and a rejected 4500-byte preedit requiring staged grow
allocation. Factory-failure coverage is a callback spy returning PRESENTATION,
not an actual native font/backend password validation. Frontend forced-factory
lifetime/pixel/secure-host tests remain Stage 2 requirements.

Integrator registers `test_password_engine` linked only to `cui`, including WASI,
and regenerates distribution. No global build files or generated headers were
modified by this worker.
