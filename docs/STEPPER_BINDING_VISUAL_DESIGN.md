# Additive Stepper visual authoring through builders

The bridge keeps its existing selected Item and aggregate CHANGE event model.
An opt-in `set_stepper_visual(enabled, vertical, density)` selects the existing C
visual navigation renderer. Defaults remain the legacy renderer. Shared
`design_size` becomes available on Stepper. Existing owned `image_rgba` storage
also accepts Item children of Stepper as per-step icons; bytes are copied and
replacement is transactional, bounded by the existing 16 MiB/2048-pixel limits.
Removal follows existing node ownership teardown. No public C component, theme,
state, or event architecture changes are required.

The visual renderer receives a selected-index snapshot. Its requests are captured
locally and reduced into the existing selected Item after rendering; the existing
aggregate CHANGE is queued once only when the final requested selection differs.
Applications observe the usual bridge dispatch phase, not C REQUEST callbacks.
The opt-in frame paints the prior selection, and the next submission paints the
accepted one. Legacy selection/render/callback behavior remains unchanged.

Expose `stepper_visual` plus existing shared sizing and Item image methods through
Python, JS/TS, Go and Rust builders. Density is md/xs/sm/lg, orientation is a bool;
connectors belong to the current visual composition and have no independent
foreign toggle. Keep the bridge's existing 64-Item limit (the C visual API allows
100). Missing icons use ordinals; XS hides icons/ordinals while still validating
owned inputs. Existing Stepper disabled and Item disabled state applies.

Verification covers legacy and visual mode switching, shared sizing/orientation,
all densities, copied/replaced/removed icons, wrong-parent/invalid dimensions,
public layout and software pixels, pointer/keyboard selection, same-selected
suppression and exactly one CHANGE through the existing dispatch phase. Exercise
actual headless language runtimes and compile TS authoring. Native tests use the
private display wrapper; no desktop windows or dependency installation in offline
policy tests. Update generated SDK sources and consumer skill resources.
