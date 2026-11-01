# Remaining retained consumer audit after the 38-case proof

Source-only checkpoint. No new reducer policy, production changes or runtime
coverage beyond signed eae79383. All added fixtures below remain unexecuted.
See [the canonical consumer inventory](SCOPE_INPUT_CANCEL_CONSUMERS.md) for the integration contract.

| Actual path | Submission/order and current contract | Classification / next oracle |
| --- | --- | --- |
| Table resize/reorder (`cui_table_reduce_common`) | Reduces before current semantics. Retained resizing/dragging consumes raw MOVE/UP before target checks; no consumed check. Borrowed column array changes in place. Width/move observers run synchronously after successful operation; programmatic changes silent; cancel does not roll back. | Source-confirmed mutation path after explicit Core cancellation. Actual nested `--retained-table-resize`, `--retained-table-reorder`, `--retained-table-idle` prepared. Require no stale width/order/callback and no orphan local press; no reset implementation until actual failure. |
| Color panel (`cui_base_color_picker`) | Submits root/part interactions, ORs old dragging bits into local pressed. Raw target pointer/key samples ignore consumption, mutate HSV/RGBA, rebuild dragging bits even with zero samples. Public bool reports change and invalidates; no synchronous change callback in this Base helper. | Source-confirmed mutation/idle state path. Actual nested `--retained-color-drag`, `--retained-color-idle` prepared; compare color channels and press bits. No policy change or broader color-picker work. |
| Sheet resize (`cui_surface_update_impl` / resize_event) | Surface loop skips consumed before resize helper. Retained resizing clears on global CANCEL, no_sheet_resize/kind change, routed primary UP; no idle capture-loss check. Size mutations/invalidation stay in original surface timing. | Stale consumed MOVEUP size path already guarded by source, but local resizing can persist idle. Must distinguish harmless retained flag versus later mutation with an actual Sheet fixture; not declared fixed or broken from source alone. |
| Chrome title move (`cui_base_title_bar`) | Current interaction then target&&!consumed gate before move/menu actions. UP unconditionally clears move_armed/moved, even when consumed. CANCEL or non-target DOWN clears pending click/press. Host perform is synchronous and capability-gated. | Stale consumed MOVEUP host action appears guarded; idle armed and double-click history persist. Need mock-host actual idle/late-action proof before any reset rule. No native window test is needed or claimed. |
| Single-line input (`cui_base_input_reduce_event`) | Clears dragging/word state on CANCEL and consumed primary UP before consumed/target guard. Fresh primary DOWN clears word state and starts selection. No idle capture-loss reset. Edits/notifications follow the existing checked synchronous engine operation. | Consumed terminal already cleans press state; idle flags still require proof. Do not erase model selection/history or fabricate edit/CHANGE notification. TEXT/composition remain outside this helper's selective suppression contract. |
| Textarea (`cui_base_textarea`) | Layout/current interaction precede event loop. CANCEL clears dragging, but consumed/target check precedes primary-UP selection cleanup. External edit revision also clears dragging; no general idle loss check. | Selection mutation from consumed samples guarded; consumed terminal may leave local drag flag. Need actual selection/idle and genuine fresh-DOWN oracle before policy. No fake selection/edit callback. |
| Editor multi-selection | Clears multi_pointer/column_dragged on global CANCEL or focused/revision/style mismatch. Consumed check precedes pointer-UP cleanup; no generic idle loss check. | Source boundary only; user deprioritized Editor, no polish/implementation planned here. Do not infer safety from Textarea or broaden text/IME behavior. |
| Typed Slider (`cui_base_slider_model_events_v1`) | Existing consumed check and post-reduction capture-loss cancellation: when snapshot ready, dragging and !cui_pressed(captured_target), emits CAPTURE_LOST with no input. Earlier option/source/domain/axis/omission precedence and immutable owner snapshot remain intact. | Existing focused tests cover Escape/capture loss. Actual nested `--typed-capture-lost` prepared to exercise this explicit helper, idle CANCEL/no fake input and fresh BEGIN/COMMIT. No typed reducer change proposed. |

## Prepared fixtures and honest boundaries

The six prioritized modes use the actual public Base reducers in a still-open
nested Dialog child while the parent is controlled closed/reopened. Table uses
real header/resize hit boxes and synchronous width/move observers; state is not
manually seeded as dragging. Color uses its actual hue part and model. Typed
Slider uses actual thumb capture, existing CAPTURE_LOST request and fresh gesture.
All borrowed model/state/bounds storage survives end-frame and is destroyed only
after the completed frame. These source fixtures have not compiled or run yet.

If Table/Color fail, propose only clearing their owned local pending press flags
and suppressing explicitly canceled samples under the opt-in marker, preserving
already delivered model changes. A fake width/move/completion callback or rollback
would contradict their existing contract; exact disposition still requires the
actual failure and root review. No generic ordinary consumed semantics change.

Source checks alone do not establish all-phase safety. For example the raw Table
and Color paths lack an obvious general ordinary/inert admission guard, whereas
the explicit cancellation operation rejects inert/layout-only/measurement.
Those legacy behavior questions must not be silently folded into this lifetime
slice or addressed with a new global dispatcher policy.

TEXT/COMPOSITION still retain their original event payload, target, consumption
and model-session rules, as the 38-case matrix proved. That preservation is not
proof that a retained text editor inside a reopened owner has complete lifecycle
closure. Any suspected stale-text or composition-session mutation needs its own
strict actual counterexample and reviewed policy; no new selection is proposed.
