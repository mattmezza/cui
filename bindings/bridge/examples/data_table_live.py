"""A Python inbox with a stable view when a batch of messages arrives."""

import argparse
import os

from cui_bridge import App, button, column, data_table, label, root_view


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="existing TTF path for an interactive SDL window")
    parser.add_argument("--smoke", action="store_true", help="verify on a private test display")
    args = parser.parse_args()
    if args.smoke and (not args.font or os.environ.get("CUI_TEST_VIRTUAL_DISPLAY") != "1"):
        parser.error("--smoke requires --font and the private display from scripts/test_local.py")
    options = {"width": 480, "height": 320, "title": "Live inbox"}
    if args.font:
        options.update(backend="sdl", font_path=args.font, initially_hidden=args.smoke)

    headers = ["Subject", "Sender"]
    keys = list(range(100, 112))
    rows = [[f"Project update {key}", "Workspace"] for key in keys]
    table = data_table("inbox").table_data(headers, rows, row_keys=keys).visual_size(360, 184)
    status = label("12 messages", key="status")

    def load_latest(_event):
        # Apply the incoming data on the UI thread after the click dispatch.
        table.table_data(
            headers, [["Release notes", "Build team"], ["Design review", "Morgan"], *rows],
            row_keys=[203, 204, *keys],
        )
        load.disabled()
        status.label("14 messages · Updated just now")

    load = button("load").label("Load latest").on_click(load_latest)
    with App(**options) as app:
        content = column("content").gap(8).children(
            label("Inbox", key="heading"), load, table, status,
        )
        app.mount(root_view("root").padding(16).children(content))
        if args.font and not args.smoke:
            app.run()
            return

        app.step()
        x, y, _, _ = table.table_cell_bounds(1, 0)
        app.queue_scroll(x + 10, y + 10, 0, -1)
        app.step()
        offset = table.table_cell_bounds(1, 0)[1]
        assert offset < y

        x, y, width, height = app.bounds(load)
        app.queue_pointer("down", x + width / 2, y + height / 2)
        app.queue_pointer("up", x + width / 2, y + height / 2)
        app.step()
        app.dispatch()
        app.step()
        assert table.current_cell(0, 0) == "Release notes"
        assert table.current_cell(3, 0) == "Project update 101"
        assert table.table_cell_bounds(3, 0)[1] == offset
        print("Live inbox updated and kept the visible row in place.")


if __name__ == "__main__":
    main()
