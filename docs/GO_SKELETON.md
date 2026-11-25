# Responsive Skeleton in Go

Use `ResponsiveSkeleton` to build decorative loading placeholders in a
retained Go UI. Start with the runnable
[workspace profile example](../bindings/go/examples/skeleton_profile/main.go).
It draws a circular avatar placeholder and responsive text bars. A button
removes the placeholders and reveals the profile.

```go
style := cui.DefaultSkeletonConfig()
style.Width, style.Radius, style.Secondary = .65, 6, true
name := cui.ResponsiveSkeleton("profile-name", style)
err := app.Mount(cui.Column("profile").Children(name))
```

Use `SkeletonFixed` with width in logical pixels, `SkeletonFraction` with
width in (0, 1], or `SkeletonGrow` with width zero. Height, radius and the
optional fixed/grow max width use logical pixels. Set `CustomColor` with a
numeric `BackgroundRGBA` in `0xRRGGBBAA` to replace the theme fill. A zero
duration uses the two-second pulse. Set `ReducedMotion` for a static shape.

`ConfigureSkeleton` replaces an owned value. Call it on the app's owner
thread once the node is mounted. `SkeletonConfig` returns a copy.
`SkeletonPresentation` reads the latest sample after a successful step.
Its boolean is false before the first step and after reconfiguration,
omission, or a failed frame. Read `NextWakeSeconds` on the same clock as
`CoreTime`. The native runner keeps its normal frame cadence. Model loading
announcements and completion in the application.

From the repository root on Linux, run the windowless example with the
bundled-source SDK:

```sh
cd bindings/go
go run ./examples/skeleton_profile
```

For a native SDL3 window, install the dependencies in the
[Go setup guide](GO_BUILDERS.md). From the repository
root, run:

```sh
cd bindings/go
go run -tags cui_native ./examples/skeleton_profile \
  --font ../../third_party/inter/Inter-Regular.otf
```

To use the dynamic build, set `CUI_BRIDGE_LIBRARY` to the absolute library path
and use `-tags cui_dynamic` on Linux. `SupportsSkeletonPresentation` checks
the optional bridge group. The alpha.4 library supports
`Skeleton("profile-name").VisualSize(72, 12)`. Responsive presentation
requires a newer provider. The bridge exposes built-in easing, while full
box refinement and custom easing are available to frame-based C callers.
