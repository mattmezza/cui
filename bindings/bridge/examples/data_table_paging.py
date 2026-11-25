"""Build an inbox that requests another page when its viewport reaches the end."""

import argparse
import os
from time import monotonic

from cui_bridge import App, column, data_table, label, root_view


class Inbox:
    """A UI-thread owner with a deterministic local data source."""

    headers = ["Subject", "Sender"]
    initial_count = 8

    def __init__(self):
        self.rows = [[f"Project update {i}", "Workspace"] for i in range(1, 13)]
        self.keys = list(range(101, 109)) + list(range(201, 205))
        self.generation = 1
        self.pending = None
        self.table = (data_table("inbox").table_data(
            self.headers, self.rows[:self.initial_count], row_keys=self.keys[:self.initial_count])
            .visual_size(360, 160).configure_load_more())
        self.status = label("8 messages", key="status")

    def view(self):
        return root_view().padding(16).children(column("content").gap(8).children(
            label("Inbox", key="heading"), self.table, self.status,
        ))

    def start_page(self):
        if self.pending is not None:
            return None
        demand = self.table.poll_load_more()
        if demand is None:
            return None
        self.table.configure_load_more(enabled=False)
        self.pending = self.generation, demand.frame_sequence
        self.status.label("Loading the next page…")
        return self.pending

    def retry(self):
        # Cancel our logical request. A transport may still deliver its old response.
        self.generation += 1
        self.pending = None
        self.table.configure_load_more()

    def complete_page(self, ticket):
        if self.pending is None or ticket != self.pending:
            return False
        # This local source returns the final 4 messages. Merge and apply on the UI thread.
        self.table.table_data(self.headers, self.rows, row_keys=self.keys)
        self.pending = None
        self.status.label("12 messages · End of inbox")
        return True


def click_cell(app, cell):
    x, y, width, height = cell
    app.queue_pointer("down", x + width / 2, y + height / 2)
    app.queue_pointer("up", x + width / 2, y + height / 2)


def verify_inbox(app, owner, smoke):
    changes = []
    table = owner.table.on_change(lambda event: changes.append(event.value))
    app.step()
    assert owner.start_page() is None

    x, y, _, _ = table.table_cell_bounds(1, 0)
    app.queue_scroll(x + 10, y + 10, 0, -20)
    app.step()
    cancelled = owner.start_page()
    assert cancelled is not None and owner.start_page() is None
    selected = table.table_cell_bounds(6, 0)
    click_cell(app, selected)
    app.step()
    app.dispatch()
    assert changes == [owner.keys[6]]

    owner.retry()
    app.step()
    fresh = owner.start_page()
    assert fresh is not None and fresh != cancelled
    assert not owner.complete_page(cancelled)
    assert owner.complete_page(fresh)
    app.step()
    assert table.table_cell_bounds(6, 0) == selected
    assert table.table_state()["row"] == owner.keys[6]
    assert table.current_cell(11, 0) == owner.rows[11][0]
    assert app.dispatch() == 0

    app.queue_scroll(x + 10, y + 10, 0, -20)
    app.step()
    assert owner.start_page() is None
    if smoke:
        click_cell(app, table.table_cell_bounds(10, 0))
        ran = False

        def verify_runner(active_app):
            nonlocal ran
            assert changes == [owner.keys[6], owner.keys[10]]
            assert active_app is app and owner.start_page() is None
            ran = True
            app.request_close()

        app.run(after_frame=verify_runner)
        assert ran
    print("Appended the final page, kept the selected row, and discarded a late response.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="existing font path for the native SDL UI")
    parser.add_argument("--smoke", action="store_true", help="check the hidden SDL runner")
    args = parser.parse_args()
    if args.smoke and (not args.font or os.environ.get("CUI_TEST_VIRTUAL_DISPLAY") != "1"):
        parser.error("--smoke requires --font and the private display from scripts/test_local.py")
    options = {"width": 480, "height": 320, "title": "Paged inbox"}
    if args.font:
        options.update(backend="sdl", font_path=args.font, initially_hidden=args.smoke)

    owner = Inbox()
    with App(**options) as app:
        if not app.supports_data_table_paging() or not app.supports_keyed_data_table():
            parser.error("the paged inbox requires a bridge with paging and keyed replacement")
        app.mount(owner.view())
        if not args.font or args.smoke:
            verify_inbox(app, owner, args.smoke)
            return

        # Let a local page take half a second. Each frame returns immediately.
        ticket = None
        ready_at = 0.0

        def after_frame(_app):
            nonlocal ticket, ready_at
            if ticket is None:
                ticket = owner.start_page()
                if ticket is not None:
                    ready_at = monotonic() + 0.5
            elif monotonic() >= ready_at:
                owner.complete_page(ticket)
                ticket = None

        app.run(after_frame=after_frame)


if __name__ == "__main__":
    main()
