# Ordered focus traversal

Reference: gpui-component commit `87d10ae5e1299d1be18670a37570c5402f952e34`.
Its Cargo.lock pins registry `gpui-pre` 0.3.8, checksum
`d7f0bc3ebba070f8981b3f8e905a12f47d27794a30d029e59cdb9edf7a4b3360`.
Source evidence: `crates/base/src/button.rs` defaults index 0 / stop true and
passes both independently; `crates/component/src/button/button.rs` does likewise.
Pinned GPUI `src/tab_stop.rs` defines a signed path of group indices plus leaf
index, lexicographic comparison, then insertion order. Prefixes precede longer
paths. Negative indices are ordered values, not automatic exclusion. Nonstop
handles remain traversal anchors. `src/window.rs` focus_next/focus_prev use the
rendered frame map and wrap at either end. These files were inspected locally
in `/tmp/cui-gpui-reference` and `/tmp/gpui-pre-0.3.8`.

Use `cui_set_tab_index` after registering behavior to opt into signed ordering.
Use `cui_set_tab_stop` independently to enable or disable traversal eligibility.
Use `cui_begin_tab_group` / `cui_end_tab_group` around child submissions for
hierarchical prefixes. Equal complete paths retain semantic submission order.
Groups are independent of focus scopes: scopes constrain eligibility, while
ordered groups constrain ordering. Tab uses the previous published snapshot;
`cui_focus_step_within` uses the current submitted controls within its scope.
Disabled and no-focus controls remain ineligible. Programmatic focus on an
eligible non-stop control anchors the next/previous search at that control's path.

Zero-initialized legacy semantics retain submission order. Legacy set_tab_stop
continues exposing index 0/-1, and legacy negative tab_index remains excluded.
Calling set_tab_index explicitly allows every signed int, including -1, as an
ordered stop. Index and stop setters do not overwrite each other's choice once
signed ordering is enabled.

Storage is bounded to 32 group levels, captured by value in each semantic
snapshot. An overflow begin fails atomically; callers must end only successful
begins. Index range is portable C int (GPUI uses platform isize). This is source
ordering parity, not all GPUI focus-handle lifecycle, retained paint replay,
or accessibility platform parity. Component option mapping is separate work.
