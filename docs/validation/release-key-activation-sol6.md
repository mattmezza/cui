# Bounded opt-in release-key activation

Core semantic details now offer `release_key_activation`. Zero retains the current
Enter-down/Space-up policy. An opted control uses unmodified Enter/Space key down
to latch its current focus generation and activates on either unmodified
activation-key up while that generation and target still match. Repeated downs
refresh the latch. The source stores a generation rather than the particular key;
therefore Enter down followed by Space up remains eligible. Other key downs or
ups, modifiers, IME/preferred-character input, cancellation and actual focus
transitions clear the pending latch. Eligibility still uses the previous rendered
semantic snapshot, including clipping and modal checks, and current composition
can reject disabled or removed controls. Generation exhaustion permanently closes
this opt-in path; it does not alter zero-option legacy controls.

For accepted release-key queries and copied snapshots, Core rechecks the
currently submitted control's release policy, focusability, focus identity,
role continuity and modal eligibility. Changing details to legacy or `no_focus`
after `cui_interact` therefore makes a later accepted query return false and
leaves snapshot output untouched. The boolean already returned by
`cui_interact` cannot be undone. The setter and end-of-frame publication also
clear pending keys when the Source policy disappears, preventing a later
Source-to-legacy-to-Source toggle from reviving an old down. Unrelated key ups
clear Source pending state even while the published policy is inactive.

The pending latch is one bounded Core value; each accepted release keeps a
bounded slot generation and the down event's frame/origin. Synthetic touch
cancellation inserts and reorders events, so Core remaps origin identifiers with
events. Consuming a down invalidates dependent release slots even if routing has
already completed; consuming a release invalidates its own slot. These guards
apply before ordinary control notification and to later accepted snapshot
queries. Core routes the entire queued batch before composition. A callback
already notified cannot be undone by a later consumer, and this policy does not
provide capture/bubble propagation or complete source Button parity.

Headless fixtures are `tests/unit/test_release_key_activation.c` and
`tests/unit/test_release_key_overflow.c`. The latter includes private Core source
so it can drive the generation to `UINT64_MAX` without a public test hook.
Compile with strict C11 warnings, run both, then run the repository CTest suite.
The integrator registers these two unique test targets in CMake.
`tests/web/test_release_key_core.py` builds the Core-only reactor with pinned
Zig 0.16 and drives real browser keydown/keyup events through Chromium 1208.
The portable standalone fixture also passed under Wasmtime 49 with 80 billion
fuel and under native ASan/UBSan/leak checks. Full native CTest under invisible
Xvfb passed 171/172; the distribution freshness check fails until the
integrator regenerates `dist/cui.h`. Hardware GPU validation is deferred.
