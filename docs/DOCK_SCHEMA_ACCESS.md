# Dock schema accessor failures

The schema writer checks `cui_dock_node_get`, `cui_dock_panel_at` and
`cui_dock_child_at` before using their outputs. These public accessors return false
for a stale node ID, an inappropriate node kind or an unavailable index; failure
does not initialize the caller's output. The writer now returns `CUI_DOCK_INVALID`
when a required read fails instead of consuming an uninitialized key, child or
slot size. This also resolves GCC 16's optimized release warning about the panel
key without substituting a fictitious zero key or size.

Export already traverses into private scratch nodes/sizes and publishes the caller's
buffers only after valid traversal. An accessor failure preserves those buffers
and the caller's count values. A callback's edits to its live model remain its own
edits; export does not roll them back. Callbacks must keep the model alive throughout
export, and this change does not provide thread safety or a general snapshot of
arbitrary concurrent/reentrant mutations.

The regression test uses only valid public operations. A dump callback for panel 1
removes upcoming panel 2, then supplies a valid leaf state. In a tabs-only model the
next panel index is unavailable; in a split model removing the second leaf collapses
the parent and makes the next child lookup unavailable. Both exports must stop with
`CUI_DOCK_INVALID`, invoke no second dump callback, and preserve output bytes and
counts. The remaining model is serialized and restored successfully. Existing
schema JSON fixtures, round trips, version-1 persistence and model equality tests
remain unchanged and run in the same suite.

Validation uses GCC 16 release optimization with warnings treated as errors,
headless CTest through the repository's invisible-display runner, and the complete
Dock test executable under ASAN/UBSAN. Distribution and SDK copies are regenerated
by the integrator.
