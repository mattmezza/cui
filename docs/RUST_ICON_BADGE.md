# Retained Icon and Badge in Rust

The inbox example shares an SVG between a heading Icon and a Badge on a
button. A second Badge shows the unread count and clears after a child
button click. The indicator takes no layout space.

Run the [complete example](../bindings/rust/examples/icon_badge_inbox.rs)
from `bindings/rust` with the [Rust build prerequisites](RUST_BUILDERS.md).

```sh
cargo run --offline --locked --example icon_badge_inbox
```

That command uses the bundled C bridge and verifies the interaction headlessly.
To open a native window on a display you intend to use, build with SDL and
pass the path to an existing font.

```sh
cargo run --offline --locked --features native --example icon_badge_inbox -- --font /absolute/path/to/font.ttf
```

Set `CUI_BRIDGE_LIB_DIR` to the absolute directory of a prebuilt library to
reuse it. Use `app.supports_icon_presentation()` and
`app.supports_configured_badge()` to test those capabilities independently.
The published alpha.4 C bridge supports [owned SVG resources](RUST_ICON_RESOURCES.md),
named Icons, and text Badges. Retained presentation and the configured Badge
container require the newer bridge.

## Attach SVG and wrap a child

```rust
use cui_builders::{App, BadgeConfig, BadgeVariant, IconConfig, IconLimits,
    IconOptions, IconSource, Kind, Result};

fn main() -> Result<()> {
    let mut app = App::headless(400., 240.)?;
    let root = app.root(Kind::Column, "inbox")?;
    let mark = app.create_icon_resource(&IconSource::named("check"), IconLimits::default())?;
    app.configured_icon(root, "synced", &IconConfig::new(mark))?;
    let badge = app.configured_badge(root, "sent-badge", &BadgeConfig {
        name: "Synced".into(),
        variant: BadgeVariant::Artwork {
            resource: mark,
            options: IconOptions { width: Some(12.), height: Some(12.), ..Default::default() },
        },
        ..Default::default()
    })?;
    app.build_child(badge, Kind::Button, "sent")?.text("Sent mail")?.finish();
    app.step(0.016)?;
    Ok(())
}
```

- Use `BadgeConfig::count(8)` for counts up to 99. Set a different maximum
  with `BadgeVariant::Count { value, maximum }`. A zero count hides the
  marker while keeping the child in place. `BadgeVariant::Dot` and
  `BadgeVariant::NamedIcon` require no owned resource.
- Use `component_disabled` on a Badge to disable its subtree. Child buttons
  use `on_click` normally. The runnable example records the action inside
  the callback and updates the Badge after `dispatch` or in `run_with`.
- `icon_config` and `badge_config` copy settings. `icon_result` and
  `badge_result` report the latest invocation. Artwork failure can coexist
  with a successful frame. Named Badge artwork has no exact artwork
  status. Reconfiguration and omission clear the observation until the next
  invocation. Reading a result never dispatches events.
- Remove or reset a retained Icon, and remove or reconfigure an artwork
  Badge, before releasing its resource. A reference prevents release.
  `reset_icon` restores the legacy named Icon settings. For option precedence
  and limits, see the [Icon contract](BRIDGE_ICON_PRESENTATION.md) and
  [Badge contract](BRIDGE_BADGE.md).
