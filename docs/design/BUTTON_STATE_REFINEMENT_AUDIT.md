# Button state-style refinement audit

Design checkpoint, 2026-10-20. **Proposal, not implementation or parity promotion.**
CUI baseline: `ebcf5211`. Frozen GPUI Kit 0.7.1:
`87d10ae5e1299d1be18670a37570c5402f952e34`, verified against
`/tmp/cui-gpui-reference`; see [baseline](../upstream-baseline.md).
Only this document changes. The manifest's “complete source state-style
refinement” row remains `not_started`; this does not mean hover and click
painting are absent. All 78 component entries remain untouched.

## Source authority

All upstream links below pin the frozen commit, not moving main:

- [Component Button](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/button/button.rs): custom variant fields 40–47, compound hover helpers 355–368, eligibility 505–521, render 600–880, palette resolution 900–1376.
- [Base Button](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/button.rs): semantic style API 59–78, resolution 141–150, selected/disabled tests 447–497.
- [Shared state resolver](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/state_style.rs): instance, active value states, disabled ordering; refinements only replace supplied fields.
- [Focus painting](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/styled.rs): `FocusLine` and `focus_style`, 235–296, plus outer-band `focus_ring` immediately following.

CUI authority: `include/cui/cui_components.h:98–165`,
`include/cui/cui_theme.h:73–77`, `src/components/cui_components.c`
(`cui_button_paint_state`, `cui_button_paint`, metadata and submit helpers),
and `src/theme/cui_theme.c:54–82`. Line references describe this checkpoint.

## Frozen contract and present coverage

| State or layer | Frozen behavior | Current CUI / remaining work |
| --- | --- | --- |
| Normal, hover, active | Per-variant background, border, foreground; hover/active only while interactive and neither selected nor open | Existing palette has normal/hover/active backgrounds and outline equivalents. Pointer hover and pressed paint already work. Foreground and border cannot be independently supplied per state. |
| Selected / open | `selected || open`; chosen selected style suppresses pointer hover/active refinements | Both flags already select active presentation. Most variants agree structurally; Ghost still uses hover-dependent foreground and its selected fill must be checked separately against source `secondary_active`. |
| Disabled | Last semantic layer; no shadow; variant-specific background/border alpha, muted foreground at 0.5 | Inertness and foreground exist. Filled border uses blanket 0.5 instead of source primary/danger/warning/success/info/custom 0.15 and secondary 1.5. Default and outline have separate handling already. |
| Loading | Inert, normal colors except selected/open presentation, whole-control opacity 0.8 unless disabled; disabled wins | Existing paint clears hover/pressed for loading and uses whole-box opacity 0.8. Preserve this, including selected/open combinations; do not conflate loading and disabled colors. |
| Ghost hover | Accent background, half alpha in dark mode; accent foreground. Active uses button-active fill and normal Ghost foreground | CUI uses full accent in either mode and hover foreground even when pressed/selected. Explicit state precedence needs correction. |
| Link / Text | Transparent; Link normal/hover/active foreground, always underlined; Text foreground alpha 0.9/1/0.7 | Existing special cases implement these basic transitions; custom palette bypass has only one foreground. Keep real link role and underline behavior. |
| Focus | Existing border tinted ring; borderless Ghost edge line, Link/Text outside line, filled inside foreground line at alpha 0.6. Theme outer ring optionally adds band | CUI uniformly replaces border with focus color and width 2. Visibility/hide switch exists, geometry and theme ring mode do not match. |
| Instance refinement | Component clones caller root style, applies it after normal defaults, and replays it inside selected and disabled styles | No Button paint/refinement callback. Geometry options and custom palette cover a subset; no general final root/content style layer. |
| Semantic refinement | Base exposes selected and disabled builders; shared resolver applies instance → selected → disabled | No corresponding Button public semantic style facade. Base contract differs from Component replay described above; do not blindly copy checkbox ordering and claim both. |
| Compound group hover | Internal helper joins named group; idle member gets hover background at 0.5, held flag supports open compound control | No Button option. Helpers are `pub(crate)`, so this is compound integration scope, not a public standalone Button builder requirement. |
| Custom shadow | Custom variant requests shadow; theme shadow gate, disabled removes it | No Button shadow field; existing box/draw primitives should be evaluated before proposing any renderer extension. |

The foreground/border limitation is independent of default theme fidelity.
CUI's theme constructor derives hover/active using RGB scale 0.9/0.75 and
outline alpha 0.1/0.2/0.4. Frozen source consumes distinct theme tokens and
uses OkLab mixing in several outline/custom paths. Existing palette overrides
can supply exact background values, but that is not proof that defaults match
all frozen light/dark themes, gradients, or alpha composition. Upstream
`Background` supports more than solid CUI `cui_color`; gradient parity remains
an explicit separate limitation. Do not silently label RGB scaling as OkLab.

The private `content_style` helper (source 301–307) refines the inner content
row and icon size for compound controls. It is not a standalone public API.
Arbitrary `Styled` modifiers include layout, opacity, typography and other
GPUI fields; a bounded CUI style facade must name its supported subset instead
of claiming an unrestricted GPUI style engine.

## Independently reviewable implementation slices

1. **Built-in state correction, no new API.** Resolve normal/hover/pressed,
   selected/open, disabled and loading explicitly before painting. Fix dark
   Ghost alpha, Ghost selected/pressed foreground, and disabled border alpha.
   Audit custom, outlined Ghost/Link/Text too: source selects outline style
   before selected special cases. Preserve existing aggregate activation bool,
   callback order, event multiplicity, focus eligibility and explicit toggled
   semantics. Test all variants across light/dark and outlined/filled; include
   hover+pressed, hover+selected, open+loading, selected+disabled, recovery and
   custom alpha. Assert actual software pixels/draw colors and semantic states,
   not just resolver output. State expectations must cite the source formulas.
2. **Compatible opt-in refinement facade.** Propose a Button-specific portable
   paint record and callbacks using existing component `user` conventions,
   borrowing the established checkbox paint-facade pattern where useful.
   Support root box and foreground first, named instance/selected/disabled
   layers, then explicit hover/active/focus customization if required. Resolve
   whether API models Base ordering or Component caller-style replay before
   coding; document both and test conflicts on the same field. Defaults must
   retain the reviewed built-in resolver; callbacks may alter paint, never
   activation reduction. Cover border, foreground, alpha, radius and explicit
   zero/transparent values; do not use zero as “unset.” Scope geometry changes
   carefully because layout/hit-test/semantic bounds must agree. Test callback
   order and single invocation, disabled precedence, loading opacity and
   bounded content. A separate additive header can avoid concurrent public
   header ownership, subject to root review.
3. **Focus rendering.** Implement source border/edge/inside/outside decisions
   and optional outer band with existing Core draw facilities where possible.
   Specify theme switch and opt-in compatibility before changing defaults.
   Verify keyboard focus, pointer focus policy, custom edges/corners, clipped
   ancestors, transparent fills, and no geometry shift at multiple densities.
   GPU-specific checks remain deferred when unavailable under Xvfb.
4. **Compound hover and custom shadows.** Audit consumers before public API:
   internal source group-hover is a cross-control presentation input, not
   evidence for a new global dispatch architecture. Prefer explicit bounded
   borrowed state or group-composition paint input. Test idle half-hover,
   actual hovered/pressed child priority, held/open, disabled/loading and
   lifetime. Shadow addition requires theme gate, disabled suppression and
   clipping/opacity tests; keep it separate from state callbacks.
5. **Token/color fidelity follow-up.** Compare frozen theme token generation
   with CUI registry/defaults, including OkLab mixing and non-solid backgrounds.
   Decide supported portable representation and compatibility policy separately.
   This audit does not authorize theme-wide changes or claim every preset is
   pixel-identical. State callback support alone cannot close this limitation.

Slices 1–3 are useful standalone Button work. Slices 4–5 have compound/theme
ownership implications and need their own approval. Keep shared XS through
9XL sizing and current CUI extensions; do not introduce a second size scale.

## Evidence and validation boundary

Inspected existing `tests/unit/test_button_behavior.c`: it exercises interactive
hover/click/focus, disabled/loading, selected semantics, variant/size loops and
custom normal/hover/active backgrounds (around 268–286). Existing
`test_button_passive_lifecycle.c`, `test_button_activation_blocker.c`,
`test_button_callback_events.c`, `test_button_click_event.c`,
`test_button_click_snapshot.c`, content/label/artwork/numeric tests and their
web/integration counterparts provide regression anchors. Their existence is
not evidence for the full cross-product above. This docs-only checkpoint did
not run native/browser tests or claim new runtime verification.

For production slices, strict C11 warnings and affected tests must run through
`python scripts/test_local.py --build-dir <private-build>`; browser probes run
headless against actual WASM. Root owns CMake registration, metadata/matrix
promotion, generated SDK/distribution and consumer-skill refresh. Reconcile
source evidence and remaining limitations feature by feature before changing
this row; neither hover support nor a callback API proves full Button parity.
