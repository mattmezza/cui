# Button built-in state painting

This slice corrects built-in Button painting against GPUI Kit 0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`. It adds no public API.
The broader [state refinement audit](design/BUTTON_STATE_REFINEMENT_AUDIT.md)
still describes proposed work, including callbacks, focus rendering, shadows,
compound hover and theme fidelity. This slice does not complete that parity row.

For Ghost, pointer hover uses the accent foreground and accent fill; dark
mode halves the fill alpha. Pressed uses the default active fill and normal
Ghost foreground. Selected/open uses the secondary active fill and normal
Ghost foreground, suppressing pointer hover and pressed changes. Outlined
selected Ghost uses the default active fill instead. These are distinct source
states, even when a theme happens to give them the same colors.

Outlined selected Link and Text retain their normal foreground. Filled
selected Link uses active link ink; filled selected Text uses foreground alpha
0.7. Outlined Ghost/Link/Text retain outline border geometry with transparent
built-in borders, matching the source's orthogonal outline construction.
Explicit custom palettes keep their own background, border and foreground
values; selected remains the custom active background.

Disabled wins over selected/open and loading. Filled intent/custom borders use
alpha multiplied by 0.15; Secondary uses 1.5, clamped to 255. Outlines use
0.5. The existing built-in Default rule remains half-alpha input background
and input border. Ghost/Link/Text keep transparent disabled backgrounds and
muted foreground at half alpha. Loading keeps its normal or selected/open
colors, suppresses pointer styling, and fades the whole control to 0.8 unless
also disabled. Returning to an interactive state restores normal hover paint.

Source evidence:
[Button render and variant resolvers](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/button/button.rs),
particularly render lines 703–733, outline selected lines 1274–1283, and
variant disabled lines 1316–1376. CUI resolves these formulas using its existing
solid-color palette. CUI theme defaults, RGB token derivation and source
OkLab/non-solid backgrounds remain separate limitations. Focus-ring behavior
is unchanged. Activation reduction, callbacks, aggregate bools and sizing are
unchanged; selected/open still do not implicitly set toggled semantics.

`tests/unit/test_button_state_refinement.c` checks both themes, all variants,
custom translucent colors, outlined/filled disabled borders, Ghost hover and
pressed/selected/open combinations, disabled plus selected/loading, loading
fade and recovery. It asserts semantic inertness/busy/presentation separation,
text and border draw colors, and actual software-rendered blank interior
pixels. Text commands are checked directly and omitted from software pixel
rendering because the portable test has no font backend. Loading icons remain
in the rendered list; pixel samples avoid their slot.

Validation used strict portable C11 warnings (`-Wall -Wextra -Wpedantic
-Werror`) and a private CMake build rooted in the isolated worktree. The new
fixture was registered only in that private build's CTest file. Seven tests
passed through `python scripts/test_local.py --build-dir
/tmp/cui-button-state-build`: the new fixture, Button behavior, passive
lifecycle, activation blocker, callback events, click event and click snapshot.
Root integration must register the new fixture in canonical CMake and refresh
owned consumer guidance/generated resources. No native windows were opened.
