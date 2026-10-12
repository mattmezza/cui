# Named theme presets

CUI includes 38 named palette adaptations from the pinned GPUI Kit theme sources: 12 light and 26 dark. The source colors, author metadata, license and revision are retained in `third_party/gpui_themes/`.

```c
cui_theme theme;
size_t index = 0;
if (cui_theme_preset_get(index, &theme)) {
    cui_str name = cui_theme_preset_name(index);
    bool dark = cui_theme_preset_is_dark(index);
    /* Use theme when rendering components. */
    (void)name;
    (void)dark;
}
```

Enumerate indices below `cui_theme_preset_count()`. Invalid indices return an empty name and `false`; a failed `get` leaves the output untouched. Returned names have static lifetime. Presets initialize a complete theme, including default metrics and motion settings. Apply application overrides, including an SVG icon loader, after loading a preset.

System appearance is a host preference: select the light or dark default when it changes. Named presets are explicit choices and should remain selected across system appearance changes. Persist names rather than indices if preferences must survive catalog reordering.

These are adaptations to CUI's semantic color tokens, not a claim of complete GPUI theme rendering parity. Missing source tokens use derived colors; button interaction colors are derived where necessary. Font and syntax-highlighting themes are not included. Aurora's unsupported gradient brushes use their first color stop; the generated JSON records affected tokens in `adaptations`.

`python scripts/theme_presets.py` regenerates both the portable C table and website JSON from the same pinned palette data. `make lint` checks that these outputs remain synchronized.
