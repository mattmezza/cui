# Private CPU V2 owned-frame extraction checkpoint

This refines `SDL_RENDERER_V2.md` after mechanical relocation `9ee20d6f`.
It is a proposal and measurement fixture, not an implemented renderer feature.
No SDL/public API or Core change is included. Actual native/WASM text/no-text
measurements satisfy the 2048-byte frame guard; production extraction still
requires root review of this checkpoint.

## Exact proposed retained control fields

The measurement-only `tests/unit/cpu_v2_frame_layout_proposal.h` defines:

- `state`: EMPTY, PREPARING, READY, RELEASING.
- `self`: address sentinel rejecting a copied/moved live ownership handle.
- `workspace`: existing CPU/metadata pointers, capacities/used counters, free
  callback/user and existing workspace teardown guard.
- `pixels`, `width`, `height`, `stride`, `bytes`: READY tight RGBA output view.
- With text compiled: embedded counted `text_owner` and `text_session`; the latter
  points to the former at its final address throughout recording and teardown.

No duplicate font owner/64-face table, retained options copy, font pointer,
command pointer, descriptors or clip stack is required in the frame. Commands,
background/shadow descriptors and glyph-range metadata retain their existing
workspace storage until release. After READY, painting is finished; the output
needs only its packed view and live ownership blocks. Borrowed font/input graphs
remain stable through release, matching the existing transaction contract.

The request has width/height, three explicit `{pointer,bytes}` exclusions, checked
count and optional owner-busy pointer. Frame and request records are automatically
excluded before reading command resources. The old software wrapper supplies its
owner, published pixel span and old target-view exclusions. No externally
published destination is required by the common prepare operation.

## Narrow extraction and preserved behavior

Move the existing body into private prepare without changing its validation,
planning arithmetic, allocation order, command order, filters or byte arithmetic.
Pure validation may inspect EMPTY control but must not mutate it. After all pure
preflight, set PREPARING and owner busy at the current first-callback boundary.
Use embedded workspace/text owner/session directly; never copy live owners.
Prepare the same resources and completely paint the existing scratch allocation.
Only then publish the tight output view and READY. No additional allocation or
second CPU image is introduced. No source lifetime is shortened.

The legacy helper keeps its signatures and target validations. It uses one local
frame, prepares, copies active rows to the old target, and releases before clearing
`target->busy`. It preserves the original published-target exclusions and optional
owner-busy contract (the initialized adapter clears that flag after return).
Allocation request vectors, peak bytes, callback order and old pixel/padding
oracles must remain exact. V1 software functions and fixed object layout remain
untouched. Existing private name/path forwarders remain available.

Init on a zero/EMPTY frame allocates nothing. Init must not retire a live frame.
Prepare on non-EMPTY or a copied live sentinel returns LIFECYCLE without callbacks.
Release on EMPTY is harmless; PREPARING/RELEASING release is a no-op and cannot
clear outer busy or queue cancellation. READY release enters RELEASING before the
first callback, ends text session, ends workspace, then canonicalizes EMPTY.
Preparation failure uses an internal teardown owner: it enters RELEASING before
cleanup and returns EMPTY, never calls the public release on PREPARING. Recursive
free callbacks cannot double-free. The adapter retains busy through later native
cleanup; this frame does not own or clear the adapter's lifecycle guard.

## Measurement and gates

`test_cpu_v2_frame_layout.c` exports compact integer object constants for frame
size/alignment, every ownership/output offset, request/workspace/plan sizes,
64-entry clip-stack bytes and conditional text-owner/session offsets/sizes.
Compile native and pinned WASM with/without CUI_HAS_TEXT without linking/running.
Record compiler/flags/source hashes and four actual layouts before extraction.
The frame is fixed stack control separate from heap quotas. Report one frame +
request + existing clip-stack/plan storage, not a total compiler/dependency stack
bound; scalar spills/call frames and dependency stacks remain additional.

After production approval, test EMPTY repetition, READY lifetime, PREPARING
allocation/provider reentry, RELEASING recursive free, attempted init/prepare/move,
all allocation failures and retry. Assert no output/guard publication before pure
preflight, no double free, stable session.owner, and exact callbacks/order. Retain
frozen old/new allocation/layout/pixel and all command/text fixtures. Exercise
both no-text and no-Bidi rejection, resource aliases, target padding, owner
exclusions and complete READY bytes independently of a destination. Register
actual CMake tests and copied SDK closure. SDL transport remains a separate gate.

## Actual four-layout measurement

GCC 16.2.1 native and pinned Zig 0.16.0 Clang wasm32-unknown-wasi compiled
C11 at O2 with UNDEBUG, Wall/Wextra/Wpedantic/Werror; no engine link or execution.
The retained Zig Clang driver uses explicit pinned WASI/include roots for -S.
System Zig 0.17.0 first rejected its cc-driver-generated unused -c under -Werror;
that diagnostic remains in the local log, with no warning suppression.

| Layout | Frame / align | Request | Workspace | Plan | Named owners subtotal |
| --- | ---: | ---: | ---: | ---: | ---: |
| Native no text | 120 / 8 | 72 | 72 | 80 | 2320 |
| Native text | 808 / 8 | 72 | 72 | 80 | 3008 |
| WASM no text | 64 / 4 | 40 | 36 | 40 | 2192 |
| WASM text | 412 / 4 | 40 | 36 | 40 | 2540 |

Subtotal = one frame + request + one existing 2048-byte clip stack + plan;
it is explicitly not total stack usage. Text owner/session are already inside
frame (native 656/32 bytes, WASM 332/16); no duplicate is counted or proposed.
Full offsets, source hashes and exact commands are in the measurement receipt.
