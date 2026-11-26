"""Compose a file card and handle Inspect like an ordinary Button.

Set PYTHONPATH and CUI_BRIDGE_LIBRARY as shown in docs/BRIDGE_COMPONENTS.md.
Pass --font to open an SDL window with a native-enabled bridge.
"""
from argparse import ArgumentParser

from cui_bridge import App, attachment, attachment_slot, button, column, label


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="TTF font for the optional native window")
    font = parser.parse_args().font

    with App(title="Project files", width=500, height=240,
             backend="sdl" if font else "none", font_path=font) as app:
        if not app.supports_attachment_slots():
            raise RuntimeError("This example requires a bridge with retained Attachment slots")
        summary = label("Select Inspect for file details.")

        def show_file(_event):
            summary.label("Quarterly report.pdf · Finance team · 1.4 MB")
            print("Inspected Quarterly report.pdf")

        inspect = button("inspect").label("Inspect").on_click(show_file)
        card = (attachment("report", "Quarterly report.pdf").visual_size(380, 0)
                .detail("1.4 MB").attachment_status("complete").children(
                    attachment_slot("preview", "media").children(label("PDF")),
                    attachment_slot("metadata", "metadata").children(label("Finance team")),
                    attachment_slot("actions", "actions", 100).children(inspect)))
        app.mount(column().gap(12).children(label("Project files"), card, summary))
        if font:
            app.run()
        else:
            app.step()
            x, y, w, h = app.bounds(inspect)
            app.queue_pointer("down", x + w / 2, y + h / 2)
            app.queue_pointer("up", x + w / 2, y + h / 2)
            app.step()
            if app.dispatch() != 1:
                raise RuntimeError("Expected one Inspect activation")


if __name__ == "__main__":
    main()
