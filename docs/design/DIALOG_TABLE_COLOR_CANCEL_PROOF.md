# Table/Color lifetime counterexamples and narrow candidate plan

Unpublished test proof and source-only reset proposal. No production reducer,
Core or public API change. The existing 38-case proof and its frozen no-helper traces stay
preserved. Compiler was released after this bounded six-case batch.

## Actual captured paths

Session 41096: strict C11/O3/Wall/Wextra/Wpedantic/Werror one fixture TU/link passed,
CPU 1 / nice 19 against existing objects/archives; private-Xvfb six diagnostics yielded
five strict failures and one success (CTest exit 8, 0.77 s). Initial Core capture and
reducer pending-state assertions all passed; no flags were manually seeded.

- Table resize: stale MOVEUP changed width100→138 and emitted width observer1.
- Table reorder: stale MOVEUP changed caller columns[1,2]→[2,1], move observer1.
- Table idle: no new samples, width100 unchanged, resizing column1 remained armed.
- Color drag: post-DOWN RGB204/255/0 became204/0/255 from stale MOVEUP.
- Color idle: same post-DOWN color retained, but dragging bit1 remained armed.
- Existing typed Slider path passed: explicit helper caused one CAPTURE_LOST with
  no input/metadata flags, then genuine new gesture BEGIN/COMMIT succeeded. No
  typed reducer change is needed for this proved path; other permutations remain
  separate coverage, not a universal inference.

The [canonical receipt](../validation/dialog-retained-consumer-counterexamples.json)
records the fixture and log hashes. Worker checkpoint `b8e7075a` additionally
records source/object/archive hashes and the full first log. Earlier failures
are not rewritten as passing checks.

## Exact proposed test-only Table reset

In a copied `cui_table_reduce_common`, compute the existing explicit current-scope
marker once after argument/selection validation and existing disabled/loading
clear, before old selection/hash/row/column normalization and event reduction.
Only when true, clear:

```
s->resizing = 0;
s->dragging = 0;
s->drag_active = false;
s->scrollbar.dragging_x = s->scrollbar.dragging_y = false;
```

Do not modify resize_x/resize_width/drag_x, model widths/order, selection, sort,
cursor, scroll offsets, viewport/load fields or observer registrations. New real
DOWN overwrites the old scalar origins through existing code. Existing normal
model/selection normalization still occurs in its old order; lifetime reset does
not create a new rollback boundary.

Inside the old raw loop, keep global CANCEL cleanup in its original position;
then skip a consumed sample only when marker=true, before resize/reorder/selection
operations and any resulting observer. No fake width/move/completion/selection
callback. Already delivered model operations and copied observer facts remain
applied. When marker=false, ordinary consumed semantics and all callback timing
remain byte-for-byte source-equivalent statements to frozen original reduction.

## Exact proposed test-only Color reset

After existing initialization/external-color synchronization and root interaction,
before building local pressed[7] or submitting part interactions, read marker;
when true set only `s->dragging = 0`. Do not restore HSV/color/last_color or clear
initialization. Existing Core capture has already been canceled explicitly.

Keep existing global CANCEL local pressed/bit cleanup first in the raw loop;
then skip consumed samples only when marker=true, before palette activation or
pointer/key mutation. Preserve final per-part semantics, rebuilding dragging,
last_color, bool changed and invalidation in their existing order. No new callback
or fake change/invalidation is emitted merely for ending a pending press. A color
change delivered before parent reset stays the comparison baseline, not initial
red. No readonly/disabled precedence change.

## Compatibility and next proof

Both candidates are exact checked copies inside the existing builddir, no public
field/symbol/record/ABI growth, no allocation or event replay and no dispatcher
change. No-helper code paths retain existing ordinary consumption behavior.
Table/Color callbacks and owned storage retain their existing lifetime contract.

Before publication: keep the five failures, make the copied candidate pass with
fresh DOWN success; test mouse/touch and idle, and preserve earlier width/order/
color mutations plus existing callback counts across reset. Freeze exact original
Table/Color bytes and run independent fieldwise no-helper traces across ordinary
consumption, keys, CANCEL, disabled/readonly/loading options and successful width/
move observer ordering. No context/pointer/padding memcmp. Rerun the 38 prior gates.
Sheet/Chrome/text remain separately unproved; TEXT/composition selection is not
broadened here. Root review remains required before any production transfer.
