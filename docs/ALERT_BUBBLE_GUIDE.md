# Alert and Bubble slots

Source audit: GPUI Kit 0.7.1, commit `87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/component/src/alert.rs` and `bubble.rs`. This is an original C implementation inspired by those contracts.

Alert retains INFO as its zero kind. `CUI_ALERT_DEFAULT` adds a neutral surface. Info, success, warning and error use the corresponding theme foreground, border and tinted surface. A banner removes the border/radius and omits title text and the title callback. `title_content`, `content`, and `action` replace or extend the text slots in that order. Dismissal still returns activation and optionally changes caller-owned `visible`.

`cui_bubble()` accepts message text or a custom `content` callback, incoming/outgoing alignment, an optional reaction callback and top/bottom ordering. Filled text uses `on_primary`; semantic variants use their kind foreground. Secondary/muted use a subdued neutral surface; outline uses background and border; ghost removes surface and padding. `cui_bubble_foreground()` supplies the same foreground when composing through the legacy begin/end API.

```c
cui_bubble(ui, id, theme, (cui_bubble_options){
    .variant = CUI_BUBBLE_SUCCESS,
    .alignment = CUI_BUBBLE_OUTGOING,
    .width = 400,
    .message = cui_string("Saved successfully"),
    .announcement = CUI_BUBBLE_ANNOUNCE_POLITE
});
```

Callbacks run synchronously with borrowed context/theme and a semantic foreground. Use percentage-width boxes to fit the current parent; a callback must balance all begin/end calls and must provide its own controls through base-backed components. Custom text chooses the supplied foreground explicitly. Supply `accessible_name` when custom content has no message text. Callback/user storage remains caller-owned; no retained allocation occurs. Invalid Bubble enums omit the component.

The convenience Bubble reserves up to 80% of its row for the surface (100% for ghost) and aligns its content within that region. A positive width specifies row width; zero uses parent width. With `state == NULL`, reactions occupy normal flow above/below, aligned along the row. Supply a unique zero-initialized caller-owned `cui_bubble_state` to measure the first frame and settle an overlapping pill on the selected surface edge. The pill extends three quarters of its height outside the surface, reserves room within the row to keep adjacent bubbles from covering controls, and inherits ancestor clipping. Geometry changes request a bounded measurement redraw; changes to custom slot content require incrementing `content_revision`. Width, variant, alignment, reaction presence and theme font/padding changes trigger measurement automatically. The cache must not be shared between IDs. Exact first-frame anchored placement is not promised: Core has no current-frame measured-anchor attachment primitive. There is no separate BubbleGroup API; compose a column using existing layout boxes. These remain explicit parity gaps.

Alert publishes ALERT; Bubble defaults to a passive GROUP, with POLITE mapping to STATUS and ASSERTIVE to ALERT. These roles request semantics only. Actual announcements depend on host/native/browser accessibility adapters and are not proven by headless role tests. No reaction area itself becomes a tab stop; controls composed within it preserve their own button semantics.

Headless evidence: `tests/unit/test_alert_bubble_slots.c` exercises both themes, every Alert kind/Bubble variant, slot ordering, incoming/outgoing placement, top/bottom reactions, first-frame measurement and settled overlap without perpetual redraw, passive status semantics and disabled action controls. Existing content tests cover controlled dismissal. The gallery Bubble story demonstrates semantic kinds, alignment and measured reaction pills. `tests/web/test_alert_bubble_slots.py` checks actual WASM paint colors, placement and reaction ordering at desktop/mobile widths. Pointer/Enter activation is covered headlessly and real Chromium CDP touch toggles are covered in the browser. Native screen-reader announcement delivery remains unverified.
