"""Workspace preferences. Pass --font FONT.ttf to open a native SDL window."""
import argparse
from cui_bridge import (App, DisclosureConfig, DisclosureTriggerStyle, DisclosureHeading,
                        DisclosureTitleState, DisclosureTitleStyle, DisclosureHeadingBox, DisclosureTypography, DisclosureHeadingLayer, DisclosureDivider)
from cui_bridge import accordion, collapsible, column, item, label


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font")
    args = parser.parse_args()
    status = label("Open a section to edit your preferences.")
    updates = []

    def on_editor_change(event):
        if event.checked:
            text = "Editing workspace preferences."
            status.label(text)
            updates.append(text)

    editor = item("editor", "Editor").children(label("Auto-save is enabled.")).on_change(on_editor_change)
    account = item("account", "Account").children(label("Personal details"))
    sections = accordion("sections").children(account, editor)
    advanced = collapsible("advanced", "Advanced").children(label("Keep local cache"))
    with App(backend="sdl" if args.font else "none", font_path=args.font,
             width=480, height=320, title="Workspace preferences") as app:
        if app.supports_disclosure_presentation():
            sections.disclosure_configure(DisclosureConfig(
                width=360, design_size="lg", joined=True, bordered=True,
                single=True, allow_collapse=True, collapsed_icon="plus", expanded_icon="minus",
            ))
            editor.set_trigger_style(DisclosureTriggerStyle(hover_rgba=0x4879e02b, cursor="pointer"))
            advanced.disclosure_configure(DisclosureConfig(width=360, borderless=True))
        if app.supports_disclosure_heading():
            sections.set_disclosure_heading(DisclosureHeadingLayer.NORMAL, DisclosureHeading(
                box=DisclosureHeadingBox(width=-1, height=44, padding=12, gap=8),
                typography=DisclosureTypography(font_size=16, weight=2), title_rgba=0x23354aff))
            editor.set_disclosure_heading(DisclosureHeadingLayer.EXPANDED, DisclosureHeading(title_rgba=0x275dccff))
        if app.supports_disclosure_title_state():
            sections.set_disclosure_title_state(DisclosureTitleState.HOVERED,
                DisclosureTitleStyle(title_rgba=0x275dccff))
            advanced.set_disclosure_title_state(DisclosureTitleState.FOCUS_VISIBLE,
                DisclosureTitleStyle(typography=DisclosureTypography(font_size=17, weight=2)))
        if app.supports_disclosure_divider():
            sections.set_disclosure_divider(DisclosureDivider(thickness=1, inset_left=12, inset_right=12, color_rgba=0xc8d0daff))
        app.mount(column("workspace").gap(12).children(label("Workspace preferences"), sections, advanced, status))
        if args.font:
            app.run()
            return

        app.step()
        x, y, _, _ = app.bounds(editor)
        app.queue_pointer("down", x + 20, y + 12)
        app.queue_pointer("up", x + 20, y + 12)
        app.step()
        app.dispatch()
        assert editor.is_open() and updates == ["Editing workspace preferences."]
        print(updates[0])


if __name__ == "__main__":
    main()
