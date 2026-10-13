# Control-family feature audit, next21

Audited CUI commit `88aecbabecee61a6dd55ce157039c6ba0edaaa7b` against the local
v0.7.1 reference commit `87d10ae5e1299d1be18670a37570c5402f952e34` in
`/tmp/cui-gpui-reference`. Production sources and the manifest were not changed.
The companion JSON maps every one of the 27 existing ledger features to exact
source/test evidence, a proposed status and its limits. These proposals certify
only the stated behavior, never a whole family or all platforms.

| Family | Proposed ledger corrections | Remaining source or contract gaps |
|---|---|---|
| Checkbox | Mark unchecked/checked/mixed and disabled/keyboard/semantic state tested. | Static check/dash; reference spring mark animation, integrated tooltip and richer inherited/wrapped label styling remain incomplete. Accessibility snapshots are not an OS bridge. |
| Radio | Mark controlled selection and disabled/keyboard semantics tested. | Shared integer exclusivity is tested, but the combined group-exclusivity/roving-focus feature stays pending: no public RadioGroup wrapper, arrow roving or automatic set position. Sibling selection is coherent after scheduled redraw. |
| Switch | Mark keyboard/pointer/disabled and semantic checked/theme tested; update obsolete limitation text. | Native track/thumb geometry, four theme/size combinations, label focus/wrapping, controlled callback request, interruptible spring/deadline and reduced motion are covered. Tooltip/custom content/label-left/custom-color options exist but are not all asserted by the cited unit test. Exact full-reference rendering is unverified. |
| Focus Trap | Mark all three narrowly named features tested. | Modal cycling/nesting/restoration and disabled/removed cleanup are asserted. Nonmodal scopes are not traps. OS accessibility modal ownership and every host/device remain unverified. |
| Clipboard | Mark adapter read/write, capability failure handling and headless tests tested; remove obsolete “input paste/cut pending” statement. | SDL roundtrip and Input mock-host cut/paste/undo/readonly/rejection exist. No reusable reference Clipboard widget with dynamic value provider, copied callback and timed Copy/Check icon exists. Browser fixture is application composition, not that API. |
| Label | Mark only UTF-8 text/theme size tested. | Current wrapper draws text only. Reference secondary text, masking, prefix/full highlight matching, attributed ranges and rich style builders are absent. Control association and disabled/error options are absent; those ledger names also exceed the pinned display Label's own contract. |
| Kbd | Existing platform formatting is tested; newly direct accessible-name test strengthens evidence. | Supplied chord name is supported, not a semantic description field. Modifier formatting does not normalize reference key aliases/case/arrows/Enter. No action-binding lookup, appearance toggle, outline option or grouped API. Grouping is broader than the reference single-stroke primitive too. |
| Skeleton | Keep size/layout and theme/motion tested; loading semantics stays pending. | No semantics; busy/loading ownership belongs to caller. Reference also has no explicit loading node. CUI uses border color and cosine alpha, while reference uses skeleton token and two-second bounce/ease; exact color/motion parity remains incomplete. |
| Spinner | Keep size/theme and reduced-motion/status tested with direct new assertions; animation scheduling stays pending. | Active redraw invalidation exists but caller owns phase. No deadline/duration/easing API. Eight-dot alpha ring differs from reference rotating Loader icon; custom icon/color/easing absent. |

“Tested” here means the precisely described subset has meaningful assertions.
For Kbd, rename the ambiguous ledger “accessible description” to “accessible
name” before adopting its proposed status; `details.description` is not provided.
For Clipboard, capability failures mean the tested public Input host failure path,
not an exhaustive injection of SDL operating-system errors. These distinctions
must survive any manifest update.

The new `tests/unit/test_control_family_audit.c` directly tests Label UTF-8 bytes,
font/color across four themes; Kbd LABEL name; Spinner 32px bounds, eight dots,
Loading STATUS and reduced/active redraw; and Skeleton dimensions/radius,
secondary opacity and phase pulse. It also asserts the meaningful negative
capability: Label and Skeleton create no semantic node. It contains no production
changes. Existing `test_components`, `test_base`, `test_core`, `test_overlay`,
`test_input`, `test_content` and `test_sdl3` supply the remaining evidence.

Validation commands (run in the isolated worktree):

```sh
cmake -S . -B build -G Ninja
cmake --build build -j 4
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  tests/unit/test_control_family_audit.c build/libcui.a -lm \
  -o "$TMPDIR/test_control_family_audit"
"$TMPDIR/test_control_family_audit"
ctest --test-dir build --output-on-failure
```

Warning build and focused standalone test pass. Existing CTest passes 94/95;
only `distribution` fails because this committed integration source contains
public Avatar/Message changes not yet regenerated into the root-owned dist.
`sdl3_services` passes in the configured adapter test environment. This audit did
not run a new browser suite or physical-device tests. Existing browser test
sources were inspected as secondary evidence, not represented as fresh runtime
results. Root can register the new focused test in CMake/WASI during integration.
Logs are in `/home/matteo/.cache/cui-validation/tmp-storage/control-family-audit`.
