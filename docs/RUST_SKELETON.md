# Responsive Skeleton in Rust

Use `App::responsive_skeleton` to insert a decorative loading shape in a
retained Rust UI. The [workspace profile example](../bindings/rust/examples/skeleton_profile.rs)
draws a fixed avatar and responsive text bars. It replaces the shapes with
loaded content when the user clicks **Finish loading**.

```rust
use cui_builders::{App, Kind, SkeletonConfig, SkeletonWidth};

let mut app = App::headless(480., 320.)?;
let root = app.root(Kind::Column, "profile")?;
let name = app.responsive_skeleton(root, "profile-name", SkeletonConfig {
    width: SkeletonWidth::Fraction(0.65),
    radius: 6.,
    secondary: true,
    ..Default::default()
})?;
app.step(0.25)?;
let sample = app.skeleton_presentation(name)?;
```

`SkeletonConfig::default()` fills the parent's inner width and sets the
height to 16 logical pixels. Choose `Fixed` in logical pixels, `Fraction`
in (0, 1], or `Grow`. A positive `max_width` caps fixed or grow widths.
`background_rgba` optionally replaces the theme fill with a numeric
`0xRRGGBBAA` color, including transparent black. `reduced_motion` makes
the shape static. A zero `duration` selects the two-second pulse.

The app owns each node. Set its style through `configure_skeleton` or
`Builder::skeleton_presentation`. Read it back with `skeleton_config`.
`skeleton_presentation` returns a copied sample from the latest successful
frame. Reconfiguration, omission and frame failure can return `None`.
Times use the Core clock. A zero `next_wake_seconds` means static, and the
native runner keeps its existing cadence. Keep loading announcements and
completion in the application.

Run the windowless example with the bundled-source crate from the
repository root on Linux:

```sh
cd bindings/rust
cargo run --example skeleton_profile
```

Install the native development dependencies from the
[Rust setup guide](RUST_BUILDERS.md) to open a window:

```sh
cargo run --features native --example skeleton_profile -- \
  --font ../../third_party/inter/Inter-Regular.otf
```

For the dynamic build, set `CUI_BRIDGE_LIB_DIR` to the directory containing
the matching shared bridge. Check `supports_skeleton_presentation` before
using the new API with an older library. The legacy `Kind::Skeleton` and
`visual_size` route works with alpha.4. Frame-based C additionally exposes
custom easing and the full Core box descriptor.
