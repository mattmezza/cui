"""Build an inbox with shared SVG artwork and a count Badge in Python."""
import argparse
import os
from dataclasses import replace

from cui_bridge import (App, BadgeConfig, BadgeVariant, IconError, IconOptions, IconSource,
                        IconStyle, button, column, configured_badge, configured_icon, label, row)


INBOX = """<svg width="24" height="24" viewBox="0 0 24 24">
  <path d="M4 4h16l3 12v4H1v-4zM3 16h6l1 2h4l1-2h6L18 6H6z" fill-rule="evenodd"/>
</svg>"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="TTF or OTF font for the interactive SDL window")
    parser.add_argument("--smoke", action="store_true",
                        help="exercise the example in the private test display")
    args = parser.parse_args()
    if args.smoke and (not args.font or os.environ.get("CUI_TEST_VIRTUAL_DISPLAY") != "1"):
        parser.error("--smoke requires --font and the private display from scripts/test_local.py")

    options = dict(width=520, height=260, title="Inbox")
    if args.font:
        options.update(backend="sdl", font_path=args.font, initially_hidden=args.smoke)

    with App(**options) as app:
        artwork = app.icon_resource(IconSource.svg(INBOX))
        header_icon = configured_icon(
            "inbox-artwork", artwork,
            IconOptions(width=36, height=36, color_rgba=0x17645aff,
                        style=IconStyle(background_rgba=0xdef6efff, radius=9)),
        )
        sync = configured_badge("sync", BadgeConfig(
            variant=BadgeVariant.ICON, color_rgba=0x17645aff, name="Synced",
            artwork=artwork, artwork_options=IconOptions(width=11, height=11)))
        unread = configured_badge("unread", BadgeConfig(count=8, name="Eight unread messages"))
        summary = label("8 unread messages", key="summary")
        mark_read = button("read").primary().label("Mark all as read")

        def read_all(_event):
            unread.configure(replace(unread.get_config(), count=0, name=""))
            summary.label("You're all caught up.")

        mark_read.on_click(read_all)
        app.mount(column("root").gap(16).children(
            row("header").gap(12).children(header_icon, label("Inbox", key="title")),
            sync.children(label("Workspace mail", key="workspace")),
            summary,
            unread.children(mark_read),
        ))
        if args.font and not args.smoke:
            app.run()
            return

        app.step()
        assert header_icon.result().error == IconError.OK
        assert sync.result().icon_error == IconError.OK
        assert artwork.info().references == 2
        assert unread.indicator_bounds() is not None
        before = app.bounds(mark_read)
        x, y, width, height = before
        app.queue_pointer("down", x + width / 2, y + height / 2)
        app.queue_pointer("up", x + width / 2, y + height / 2)
        app.step()
        assert app.dispatch() == 1
        app.step()
        assert app.bounds(mark_read) == before
        assert unread.indicator_bounds() is None
        print("Inbox: shared SVG painted twice, and marking messages read cleared the count")


if __name__ == "__main__":
    main()
