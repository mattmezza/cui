# Retained Icons and Badges in Go

Use `ConfiguredIcon` to display an app-owned SVG in a Go layout and
`ConfiguredBadge` to put a count, dot, or icon around ordinary widgets.
Share an `IconResource` across uses and give each use its own size, color,
transform, box style, and layout constraints. The Badge indicator takes
no layout space or keyboard focus.

## Run the inbox example

From the repository root on Linux with Go 1.23 or newer, cgo and a C11
compiler, run the headless example using the bundled C source:

```sh
cd bindings/go
go run ./examples/icon_badge_inbox
```

For a native window, install the SDL and font dependencies in the
[Go builder setup](GO_BUILDERS.md). From `bindings/go`, run:

```sh
go run -tags cui_native ./examples/icon_badge_inbox \
  --font ../../third_party/inter/Inter-Regular.otf
```

The example shares an SVG between the header and an Outbox action. Click
**Mark all as read** to hide the count while the button stays in place.
[Inbox source](../bindings/go/examples/icon_badge_inbox/main.go).

To use a shared bridge, run with `-tags cui_dynamic` and set
`CUI_BRIDGE_LIBRARY` to its absolute path. The published alpha.4 bridge
supports resources and legacy named Icon and text Badge. Configured
components need a newer bridge and matching Go source.

```go
check, err := app.NewIconResource(cui.IconName("check"), cui.IconLimits{})
if err != nil {
    return err
}
status := cui.ConfiguredIcon("status", cui.IconConfig{
    Resource: check, Name: "Synced",
    Options: cui.IconOptions{Size: cui.IconLarge, ColorRGBA: 0x34a170ff, HasColor: true},
})
badge := cui.ConfiguredBadge("outbox-state", cui.BadgeConfig{
    Variant: cui.BadgeIcon, Artwork: check, Name: "Synced",
})
outbox := cui.Button("outbox").Text("Outbox")
return app.Mount(cui.Column("inbox").Children(status, badge.Children(outbox)))
```

## Configuration and interaction

- Probe `SupportsIconResources`, `SupportsIconPresentation` and
  `SupportsConfiguredBadge` separately. Each optional group checks its
  advertised version, required symbols, and exact record sizes.
- `ConfigureIcon` and `ConfigureBadge` replace the options. Failed changes
  preserve the current configuration. `IconConfig` and `BadgeConfig`
  copy the options and keep a reference to the same resource owner. Set the
  corresponding `Has` flag for explicit zero width, height, opacity,
  constraints, or color. Colors use `0xRRGGBBAA`.
- To preserve a fallback, start with `Icon("status", "check")` and call
  `ConfigureIcon`. `ResetIcon` returns to the original named Icon path.
  Set text and legacy visual size before configuring, or after reset.
- `DefaultBadgeConfig` reads the provider’s defaults. `Count: 0` hides
  the count. An explicit `Maximum: 0` formats a positive count as `0+`.
  BadgeIcon accepts `NamedIcon` or `Artwork`. Owned artwork also accepts
  `ArtworkOptions`. Set the indicator's semantic label on `BadgeConfig.Name`.
- After `Step`, `IconResult` and `BadgeResult` report whether a widget ran
  in the latest admitted frame. Use `FrameStatus` for the bridge’s frame
  outcome and `IconError` for the artwork outcome. For a Badge, inspect
  `IconError` only when `HasIconError` is true. `BadgeIndicatorBounds`
  reports committed indicator geometry when present.
- Child controls dispatch normal events through `App.Dispatch` after
  `Step`. Reading a result leaves the event queue untouched. Disable the
  Badge to make its children inert. Removing it removes its children.
- Reset, remove, or reconfigure consumers before closing an actively
  referenced resource. Closing the app releases nodes and resources on
  the owner goroutine. Arbitrary SVG loader callbacks and configured Spinner
  remain C APIs.

[SVG preparation and cache limits](GO_ICON_RESOURCES.md) ·
[C Icon contract](BRIDGE_ICON_PRESENTATION.md) · [C Badge contract](BRIDGE_BADGE.md).
