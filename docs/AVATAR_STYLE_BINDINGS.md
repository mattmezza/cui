# Avatar style bindings

The retained bridge exposes opt-in Avatar styling in C, Python, Node/TypeScript,
Go and Rust. Shared `design_size` currently covers **Button and Avatar only**;
other component kinds return an error. This is an incremental binding surface,
not uniform styling coverage or full component parity.

The existing Button `size` method, convenience variants, defaults and callback
behavior remain unchanged. `design_size` on Button selects the same scale;
subsequent size calls replace the previous selection. The C
`cui_bridge_design_size` typedef aliases `cui_bridge_button_size`, retaining all
existing `CUI_BRIDGE_SIZE_*` values. Go `DesignSize` and Rust `DesignSize` likewise
alias their existing `Size` type; Python and TypeScript expose named size literals.

| Setter | Coverage | Effect |
| --- | --- | --- |
| `design_size(size)` | Button, Avatar | `inherit`, `xs`, `sm`, `base`, `md`, `lg`, `xl`, `2xl` through `9xl` |
| `identity_colors(enabled, hue)` | Avatar | Deterministic CUI identity palette; optional hue selects one of 12 hues, 0..11 |
| `anonymous_icon(enabled, name)` | Avatar | Named SVG fallback for an empty person name; empty icon name selects `user` |

C uses `cui_bridge_set_design_size`, `cui_bridge_set_avatar_identity` and
`cui_bridge_set_avatar_placeholder`. Boolean flags must be 0/1; hue must be
0..11. Placeholder names are validated UTF-8 and copied transactionally, with a
4096-byte limit. Invalid calls preserve the previous settings. Removed handles
and closed Apps return the existing handle/lifecycle errors. An App pointer is
invalid after `cui_bridge_destroy`, as with every bridge function.

Python, Node and Go support configuring an unmounted retained builder and live
updates after mounting. Rust supports the App methods and fluent borrowed
Builder methods. All wrappers validate component kinds and argument types;
invalid Go builder calls retain the existing sticky error convention.

```python
person = avatar("person", "Ada Lovelace").design_size("md").identity_colors(True, 4)
anonymous = avatar("anonymous").anonymous_icon(True, "user")
```

```typescript
const person = avatar('person', 'Ada Lovelace').design_size('md').identity_colors(true, 4);
const anonymous = avatar('anonymous').anonymous_icon(true, 'user');
```

```go
person := cui.Avatar("person", "Ada Lovelace").DesignSize(cui.MD).IdentityColors(true, 4)
anonymous := cui.Avatar("anonymous", "").AnonymousIcon(true, "user")
// IdentityColors(true) selects the deterministic hue; at most one hue is accepted.
```

```rust
let person = app.build_child(root, Kind::Avatar, "person")?
    .text("Ada Lovelace")?.design_size(DesignSize::Md)?
    .identity_colors(true, Some(4))?.finish();
app.anonymous_icon(person, true, "user")?;
// None selects the deterministic hue.
```

With every new option disabled and size inherited, Avatar retains its exact
legacy renderer: 40 logical pixels by default, or its existing `visual_size`
width. Explicit scale variants take precedence over pixel width and use the
native Avatar's 16px rem basis: XS=16, SM=24, BASE=MD=48, LG=80; larger variants
follow the shared scale with a 512px bound. Styled inherited pixel sizes are
also capped to 512. Clearing the scale, identity and anonymous-icon options
restores the legacy renderer, including legacy pixel widths over 512. Positive
Avatar dimensions stay fixed even in narrow flex layouts; this slice adds no
responsive shrink behavior. Avatar continues to ignore `visual_size` height.

Identity colors use the native CUI initials hash and current theme's light/dark
palette. They are not a guarantee of another toolkit's identity assignment.
Named people keep their initials even when an anonymous icon is enabled.
The anonymous icon is decorative within the Avatar's existing Image semantic;
it does not add a focus stop or activation/callback API. Icon names resolve via
the existing embedded catalog/name/path aliases and context cache, with runtime
SVG rasterization. An unavailable icon name paints no icon; it is not a raw SVG
string, arbitrary filesystem loader or a foreign icon-state object.

This slice omits borrowed `cui_icon_state *`, raw SVG input/custom loaders,
bitmap/image-state ownership, custom background/foreground/ring colors,
interactivity and AvatarStack. It does not add style setters to other components.
Use a matching bridge/library revision; generated SDK/export copies are updated
by the integration workflow.

Validation includes a portable real-bridge draw fixture covering legacy and
styled bounds, palette colors, SVG mask draw commands, accessible Image/name,
owned placeholder lifetime, transactional rejection, removal and closed Apps.
The fixture includes the portable bridge implementation to inspect its private
renderer without exporting a testing hook; compile it with the `cui` target
only, not `cui_bridge`. Existing callback/lifetime tests and new language cases
cover all five handwritten surfaces, including strict TypeScript negative checks.
All CTest runs use `python scripts/test_local.py --build-dir <build>` on Linux.
