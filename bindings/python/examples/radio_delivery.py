"""Choose a delivery service. Pass --font FONT to open the native app."""
import argparse

from cui_bridge import App, button, column, item, label, radio_group, tag


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="Font file for the native window")
    args = parser.parse_args()

    arrival = "Friday"
    summary = label(f"Estimated arrival: {arrival}")
    standard = item("standard", "Standard")
    priority = item("priority", "Priority").detail("Tracked delivery. Recommended.")
    recommendation = tag("recommended", "Recommended")
    priority.children(recommendation)
    group = radio_group("delivery").children(standard, priority).select(standard)

    def update_estimate(_event):
        nonlocal arrival
        arrival = "Wednesday" if group.selected() is priority else "Friday"
        summary.label(f"Estimated arrival: {arrival}")

    def reset_selection(event):
        group.select(standard)
        update_estimate(event)

    group.on_change(update_estimate)
    reset = button("reset").label("Reset selection").on_click(reset_selection)
    with App(backend="sdl" if args.font else "none", font_path=args.font,
             title="Delivery options", width=320, height=200) as app:
        group.animate_selection(app.supports_radio_motion())
        decorated = app.supports_radio_item_content()
        priority.decorate_content(decorated)
        if app.supports_radio_tooltip():
            priority.radio_tooltip("Track your parcel online")
        ordered = app.supports_radio_tab_index()
        if ordered:
            group.tab_index(-2)
        app.mount(column("checkout").gap(12).children(
            label("Delivery service"), reset, group, summary))
        if args.font:
            app.run()
        else:
            app.step()
            x, y, width, height = app.bounds(recommendation if decorated else priority)
            app.queue_pointer("down", x + width / 2, y + height / 2)
            app.queue_pointer("up", x + width / 2, y + height / 2)
            app.step()
            assert app.dispatch() == 1
            assert group.selected() is priority
            assert arrival == "Wednesday"
            if ordered:
                for key in ("tab", "enter"):
                    app.queue_key("down", key)
                    app.queue_key("up", key)
                    app.step()
                assert app.dispatch() == 1
                assert group.selected() is standard
                assert arrival == "Friday"


if __name__ == "__main__":
    main()
