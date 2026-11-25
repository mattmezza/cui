# Owned SVG resources in Rust

Use `App::create_icon_resource` to prepare SVG from a built-in name, a local
asset key, or raw SVG text. A path source looks up a copied key in the
resource’s asset map and embedded catalog. To load a file, read it in Rust
and pass the contents to `IconSource::svg` or `App::put_icon_asset`.

Run the [headless mask example](../bindings/rust/examples/icon_resource.rs)
with the [Rust builder prerequisites](RUST_BUILDERS.md).

```sh
cd bindings/rust
cargo run --offline --locked --example icon_resource
```

To use a compatible shared library, run the same example from `bindings/rust`
with its absolute directory.

```sh
CUI_BRIDGE_LIB_DIR=/absolute/path/to/libdir cargo run --offline --locked --example icon_resource
```

Both commands run headlessly. The shared path works with the alpha.4 C
bridge. Use `app.supports_icon_resources()` to check for the complete group.
The older named Icon and scalar Badge builders remain available on providers
that lack the resource group.

```rust
use cui_builders::{App, IconError, IconLimits, IconSource, IconTransform};

fn main() -> cui_builders::Result<()> {
    let mut app = App::headless(320., 240.)?;
    let icon = app.create_icon_resource(&IconSource::named("check"), IconLimits::default())?;
    let status = app.prepare_icon(icon, 24., 24., IconTransform::IDENTITY)?;
    if status == IconError::Ok {
        let bitmap = app.icon_bitmap(icon)?;
        println!("Prepared {}×{} RGBA8 pixels", bitmap.width, bitmap.height);
    }
    app.release_icon_resource(icon)?;
    Ok(())
}
```

- `prepare_icon` returns the artwork status inside the Rust `Result`. On an
  artwork failure, `icon_bitmap` can still copy the last successful raster.
  The resulting `Vec<u8>` owns its packed RGBA8 pixels and can outlive the app.
- Keep resource operations on the app’s owner thread. A copied handle refers
  to the same resource and becomes stale on explicit release. App destruction
  releases outstanding resources.
- `set_icon_source` changes the source, `put_icon_asset` replaces artwork
  behind a stable path, and `reload_icon` invalidates a cached preparation.
  Call `collect_icon` after consuming prior frames to reclaim inactive masks.
  The [bridge contract](BRIDGE_ICON_RESOURCES.md) records the limits.
- The Rust resource API copies SVG and local asset keys. Deferred loader
  callbacks remain a direct C feature.

See the [inbox example](RUST_ICON_BADGE.md) for retained Icon and Badge
configuration with one shared resource.
