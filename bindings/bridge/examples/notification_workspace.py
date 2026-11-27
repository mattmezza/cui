"""Show a workspace notification and restore the trigger after dismissal."""
import argparse

from cui_bridge import App, button, column, label, notification


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="TTF or OTF font for the interactive SDL window")
    args = parser.parse_args()
    status = label("Workspace is up to date")
    show = button("show-update").primary().label("Show workspace update")
    notice = notification("workspace-update", "Workspace report").detail(
        "The weekly workspace report is ready. Review the latest changes "
        "with your team before sharing it."
    )
    dismissals = 0

    def show_update(_event):
        notice.open()
        show.disabled(True)
        status.label("Workspace update is open")

    def dismissed(_event):
        nonlocal dismissals
        dismissals += 1
        show.disabled(False)
        status.label("Workspace update dismissed")

    show.on_click(show_update)
    notice.on_close(dismissed)
    options = {"width": 320, "height": 380, "title": "Workspace updates"}
    if args.font:
        options.update(backend="sdl", font_path=args.font)
    with App(**options) as app:
        if app.supports_responsive_notifications():
            app.set_responsive_notifications(True)
        app.mount(column("workspace").gap(12).children(status, show, notice))
        if args.font:
            app.run()
            return
        app.step()
        x, y, width, height = app.bounds(show)
        app.queue_pointer("down", x + width / 2, y + height / 2)
        app.queue_pointer("up", x + width / 2, y + height / 2)
        app.step()
        app.dispatch()
        assert notice.is_open()
        notice.open(False)
        app.step(.4)
        app.dispatch()
        assert not notice.is_open() and dismissals == 1


if __name__ == "__main__":
    main()
