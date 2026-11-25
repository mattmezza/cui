# Owned SVG resources in Go

Use `App.NewIconResource` for raw SVG, an embedded catalog name, or a key in
the resource’s local asset map. The bridge copies SVG and keys. A local asset
key is resolved from the resource map or built-in catalog. To use a local file,
read it in Go and pass its contents to `IconSVG` or `PutAsset`.

Run the [coverage mask example](../bindings/go/examples/icon_resource/main.go)
from `bindings/go` on Linux with cgo and a C11 compiler:

```sh
go run ./examples/icon_resource
```

To exercise the corresponding shared-library path:

```sh
CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so \
go run -tags cui_dynamic ./examples/icon_resource
```

Both commands work headlessly. The dynamic path supports the published
alpha.4 library. Probe `app.SupportsIconResources()` when selecting a provider.
The optional group checks its version, all symbols, and exact record sizes.
Older libraries can still build named `Icon` nodes and text `Badge` nodes.
See [Go setup and ownership](GO_BUILDERS.md) for prerequisites and the app thread.

```go
mark, err := app.NewIconResource(cui.IconName("check"), cui.IconLimits{})
if err != nil {
    return err
}
defer mark.Close()

status, err := mark.Prepare(24, 24)
if err != nil {
    return err
}
if status != cui.IconOK {
    return fmt.Errorf("Icon preparation failed with status %d", status)
}
mask, err := mark.Bitmap()
if err != nil {
    return err
}
fmt.Printf("Rasterized %dx%d\n", mask.Width, mask.Height)
```

- Use `IconPath` and `PutAsset` to replace artwork behind a stable key.
  `SetSource` changes the source, and `Reload` invalidates preparation
  with the same source. Call `Collect` after consuming prior frames to
  reclaim inactive masks.
- `Prepare` returns an artwork status independently of the Go error.
  After a failed prepare, `Bitmap` can still return the previous active mask.
  Successful copies own their RGBA8 bytes and can outlive the app.
- `DefaultIconLimits` and `Limits` report normalized limits. See the
  [C resource contract](BRIDGE_ICON_RESOURCES.md) for ceilings, app-wide budgets,
  copy semantics, and caller responsibilities.
- Keep resources on the app’s owner goroutine. Copies of the Go resource
  share one handle. Call `Close` for early release. App shutdown releases
  outstanding resources. Deferred foreign SVG loader callbacks are unavailable.

To attach the same resource to multiple widgets and handle child events, see
the [retained Icon and Badge inbox](GO_ICON_BADGE.md).
