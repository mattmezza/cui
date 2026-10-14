# Button tab ordering

The frozen gpui-component source commit is
`87d10ae5e1299d1be18670a37570c5402f952e34`. Component Button fields and defaults
in `crates/component/src/button/button.rs` are `tab_index: isize = 0` and
`tab_stop: bool = true`; methods at lines 472 and 480 update them independently.
Render chains `track_focus`, `tab_index`, `tab_stop` at lines 816–818. Base Button
in `crates/base/src/button.rs` follows the same independent defaults and mapping.
The pinned GPUI registry dependency is `gpui-pre` 0.3.8 (Cargo.lock checksum
`d7f0bc3ebba070f8981b3f8e905a12f47d27794a30d029e59cdb9edf7a4b3360`).
Its `src/tab_stop.rs` compares signed group paths lexicographically, then
submission order. These exact sources were inspected in `/tmp/cui-gpui-reference`
and `/tmp/gpui-pre-0.3.8`; see Core's `TAB_ORDER.md` for traversal evidence.

CUI Button maps `.custom_tab_index = true, .tab_index = index` onto Core's
explicit signed ordering, and `.skip_tab` independently onto tab-stop eligibility.
For example index -1 remains a traversable ordered value when custom_tab_index
is true and skip_tab is false. No sentinel interpretation is applied to that
explicit index. Zero-initialized Button options preserve existing defaults and
legacy semantic index 0/-1 for existing skip_tab users.

Wrap Button submissions in Core's begin/end_tab_group calls to form nested
paths. Button does not create a group of its own. Content, hover, and accepted
activation callbacks compose with the copied Button descriptor. A callback
changing the caller's options cannot retroactively change the current Button's
submitted index, eligibility, or copied activation callback. Core snapshot
publication preserves these values for the next frame's Tab traversal.

Disabled/loading still suppress interaction. skip_tab preserves pointer and
programmatic focus eligibility; Core's no_focus suppresses all focus eligibility.
The frozen Button exposes no no_focus property, so this change does not invent
one: applications can refine submitted Core semantic details when needed.
Focus scopes and modal filtering remain Core behavior. Pointer-focus prevention
and focus-ring presentation remain separate existing Button options.

Supported scope: Button option mapping, signed hierarchical traversal through
Core, and existing copied accepted-event behavior. This does not claim all GPUI
focus handle lifecycle, retained paint replay, accessibility adapters, or platform
isize range parity; CUI indices use portable C int and bounded Core groups.
