# Authoring a custom design system

Call base behavior inside your own Clay-backed box, then draw with your tokens. Base functions do not import colors, fonts or styled components.

```c
cui_id id = cui_id_from(ui, "notify");
cui_begin_box(ui, id, (cui_box){.width=240, .height=40, .padding=8});
bool changed = cui_base_switch(ui, id, cui_string("Notifications"),
                               &settings.notifications, false);
cui_text(ui, cui_string(settings.notifications ? "Enabled" : "Disabled"),
         16, my_foreground);
cui_end_box(ui);
```

The box and semantic behavior use the same ID. Use a different scoped key for each child. Domain values belong to the caller; base functions request changes synchronously through controlled values and return change status. Draw from updated state and inspect `cui_interaction` for focus, hover and press. Current styled controls deliberately reuse this seam. `examples/unstyled` compiles independently of theme/style APIs, and the terminal theme uses the same behavior with square corners and amber/ink tokens.

Compound components must use explicit behavioral models and focus scopes. Do not duplicate keyboard rules in their painters. Add behavioral tests, semantic assertions, a real story and limitation docs before raising manifest status. Full verified status requires every acceptance criterion in initial-prompt.md.
