# Segmented Tabs: opt-in source shadow branches

This bounded appearance slice uses gpui-kit 0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`: [TabBar indicator](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/tab/tab_bar.rs#L195), [Tab branches](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/tab/tab.rs#L650), and [private raised helper](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/styled.rs#L114). It does not complete Tabs/TabBar or certify GPUI GPU AA/kernel/color identity. Prior actual CUI draw-list evidence in `docs/validation/shadow-consumers-next33.md` showed no DROP layers in measured/static segmented paths.

Append `shadow_policy` to `cui_tabs_options`:

| Policy | Segmented behavior |
| --- | --- |
| LEGACY=0 | Existing paint, no added shadows; zero initialization remains compatible. |
| SOURCE_TAB=1 | Standalone-like selected enabled inner cell uses public XS; no shared indicator, even if animation supplied. |
| SOURCE_BAR=2 | Requires caller-owned animation and horizontal orientation. Before measurement, selected enabled inner cell uses public SM. Once measured, the shared indicator uses the private raised pair. |

SOURCE_TAB is a styling mode on the existing CUI tab-list API, not a new claim that the standalone GPUI Tab composition/semantics are fully implemented. SOURCE_BAR explicitly requires the existing retained state instead of hiding an allocation; source retains its indicator through window-keyed state automatically. Other variants ignore valid policies and retain their paint/animation behavior. Invalid policy or unsupported SOURCE_BAR configuration returns false before Base reduction or content/render callbacks; `cui_tabs_shadow_policy_valid` offers an explicit check because the existing widget return is a changed flag.

Ordered tuples are `(x,y,sigma,spread,black alpha)` in logical pixels:

* Standalone XS: `(0,1,2,0,13)`.
* First-measure SM: `(0,1,3,0,26)`, then `(0,1,2,-1,26)`.
* Measured private raised: `(0,1,1.5,0,26)`, then `(0,1,1,-1,26)`.

Public SM/XS are used only where source calls those presets. The measured pair is not substituted with either public preset. Its caster is the spring-resolved inner indicator, never the text label or entire trough. Background is theme background; inner height and radius follow the applicable tab metrics. Source four-size radius selection follows the size enum independently of density. CUI's expanded design sizes use the existing inner-height radius mapping as an explicit extension. Core applies inherited opacity once (half opacity gives alpha6 for XS and13 for SM/private). The inner surface follows the shadows.

Disabled standalone/first-measure tabs have no shadow, matching source `TabStyle.shadow=false`. Source TabBar's shared indicator admission does not reject a disabled selected index: CUI preserves the externally controlled disabled selection and emits the measured indicator, while keeping disabled tab appearance, inert interaction and absence of its panel. The disabled outer background can cover parts of the indicator; tests must compose that background rather than compare shadows alone to white. No disabled activation or implicit selection repair is introduced.

The initial measurement requests a follow-up frame. Existing caller spring/transition behavior remains in use. Keep `cui_tab_animation` alive through `cui_end_frame`, since existing bounds tracking writes there; give each retained tab-list owner its own state and reset it on rebinding as before. The new prepaint payload contains only copied geometry/radius/color, never a pointer into that state. It emits two DROP commands plus the surface, with an explicit five-command budget including Core's two clip commands. Layout masks/modal/opacity and bounded frame storage follow Core's existing contract. Invalid policy is a pre-reduction failure; a later global Core frame/resource failure is not a rollback promise for already accepted widget events or callback side effects. Renderers still prepare their resources before clearing targets.

Existing CUI scrolling clips both axes, whereas the pinned source wrapper specifically hides horizontal overflow. This slice preserves the scrolling API and documents possible vertical-tail clipping. Vertical SOURCE_BAR is unsupported atomically; SOURCE_TAB can style CUI's vertical extension. Generic Source StyleRefinement, standalone composition API, all TabBar lifecycle/animation/scroll differences and full family parity remain outside this appearance slice.

## Review and registration receipt

Root registers new tests and regenerates distribution; this worker changes no global configuration/gallery/backend files.

* `tests/unit/test_tabs_source_shadows.c`: actual composed Core commands for all four source sizes, both themes, exact tuples/order/caster/radius/opacity, genuine first measure, measured disabled selection, real pointer enable/disable behavior, controlled selection changes, scroll caster translation/stability, immutable paint-copy lifetime, unchanged nonsegmented/legacy paint, invalid policy/missing state/vertical failure before callbacks or retained-state changes.
* `tests/integration/tabs_source_shadows_fixture.h` and `test_tabs_source_shadows_native.c`: copy actual component draw payloads then destroy the originating context; omit TEXT identically in each consumer for geometry/alpha proof without a hardcoded font. Software, SDL Renderer, SDL GPU and Sokol agree at straight interior/outside samples within three byte levels for40 frames: five branches × light/dark × opaque/translucent × DPR1/2. Uses real DISPLAY=:0 and constrained utility GPU window; plain Xvfb is not Vulkan evidence. Link cui_software/cui_sdl3/cui_sdl_gpu/cui_sokol and their native dependencies; define GL_GLEXT_PROTOTYPES. No wrapper symbols needed.
* `uv run tests/web/test_tabs_source_shadows.py --output OUTPUT --chromium PATH`: pinned Playwright1.58, Zig0.16 WASI reactor and actual composed Core/Components fixture. Chromium1208 passes40 frames, exact branch tuples plus Canvas/WASM pixels against ordered surface/clip/shadow compositing. Initial oracle omitted the retained disabled outer background and failed; corrected reference preserves the exact byte-tolerance assertion. This is standalone proof, not Gallery wiring.

Strict native warning build passes. Full CTest166/167 fails only stale generated `distribution`; Root regenerates. Scoped ASan/UBSan/leak checking passes. Actual Wasmtime49 WASI headless tests pass. Native proof `/tmp/cui-tabs-source-native.log`; browser proof `/tmp/cui-tabs-source-browser-corrected.log`; CTest `/tmp/cui-tabs-source-final-ctest.log`; sanitizer `/tmp/cui-tabs-source-asan-final.log`; WASI `/tmp/cui-tabs-source-wasi-final.log`. New tests are not yet part of the isolated baseline CTest count. No native font pixel or physical screen-reader claim is made.
