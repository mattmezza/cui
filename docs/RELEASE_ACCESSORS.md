# Checked accessor callers

Dock and table components check model getter results before reading output values.
Visibility and render callbacks can change the model between traversal steps; a
failed getter skips the unavailable item or reports an error where traversal is
required to complete.

Dock lifecycle capture builds its next snapshot in temporary storage. Allocation
failure returns `CUI_DOCK_CAPACITY`; invalidated traversal returns
`CUI_DOCK_INVALID`. Both preserve the previously pending snapshot and clear the
capture guard. Callback edits to the model itself are not rolled back. Capture
sets the existing dispatch guard while evaluating callbacks to prevent reentrant
capture or dispatch from publishing incomplete data.

`tests/unit/test_release_accessors.c` uses explicit checks that remain active in
optimized builds. It covers unchanged outputs on failed getters, visibility
callbacks removing panels, lifecycle snapshot preservation after callback edits
and allocation failure, and table header/label callbacks shrinking bound data.
On GNU-compatible ELF linkers, define `CUI_TEST_ALLOCATOR` and link with
`-Wl,--wrap=malloc` to enable the allocation failure case. Other platforms can run
the remaining cases without this definition.

This is focused traversal hardening, not a guarantee that arbitrary mutation of
all borrowed application data during callbacks is supported.
