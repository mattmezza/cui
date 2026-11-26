"""Choose a delivery service. Pass --font FONT to open the native app."""
import argparse

from cui_bridge import App, column, item, label, radio_group


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="Font file for the native window")
    args = parser.parse_args()

    arrival = "Friday"
    summary = label(f"Estimated arrival: {arrival}")
    standard = item("standard", "Standard")
    priority = item("priority", "Priority").detail("Tracked delivery")
    group = radio_group("delivery").children(standard, priority).select(standard)

    def update_estimate(_event):
        nonlocal arrival
        arrival = "Wednesday" if group.selected() is priority else "Friday"
        summary.label(f"Estimated arrival: {arrival}")

    group.on_change(update_estimate)
    with App(backend="sdl" if args.font else "none", font_path=args.font,
             title="Delivery options", width=320, height=200) as app:
        group.animate_selection(app.supports_radio_motion())
        app.mount(column("checkout").gap(12).children(label("Delivery service"), group, summary))
        if args.font:
            app.run()
        else:
            app.step()
            x, y, width, height = app.bounds(priority)
            app.queue_pointer("down", x + width / 2, y + height / 2)
            app.queue_pointer("up", x + width / 2, y + height / 2)
            app.step()
            assert app.dispatch() == 1
            assert group.selected() is priority
            assert arrival == "Wednesday"


if __name__ == "__main__":
    main()
