"""Show a workspace notification with a separate Review action."""
import argparse

from cui_bridge import App, button, column, label, notification


def tap(app, node):
    x, y, width, height = app.bounds(node)
    assert width > 0 and height > 0
    app.queue_pointer("down", x + width / 2, y + height / 2)
    app.queue_pointer("up", x + width / 2, y + height / 2)
    app.step()
    app.dispatch()


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
    review = button("review-update").secondary().label("Review report")
    reviews = dismissals = 0

    def show_update(_event):
        notice.open()
        show.disabled(True)
        status.label("Workspace update is open")

    def review_report(_event):
        nonlocal reviews
        reviews += 1
        status.label("Review opened · three changes to verify")

    def dismissed(_event):
        nonlocal dismissals
        dismissals += 1
        show.disabled(False)
        status.label("Workspace update dismissed")

    show.on_click(show_update)
    review.on_click(review_report)
    notice.on_close(dismissed)
    options = {"width": 320, "height": 380, "title": "Workspace updates"}
    if args.font:
        options.update(backend="sdl", font_path=args.font)
    with App(**options) as app:
        if app.supports_responsive_notifications():
            app.set_responsive_notifications(True)
        has_action = app.supports_notification_action()
        if has_action:
            notice.action(review)
        app.mount(column("workspace").gap(12).children(status, show, notice))
        if args.font:
            app.run()
            return
        app.step()
        tap(app, show)
        assert notice.is_open()
        if has_action:
            app.step(.4)
            app.step()
            tap(app, review)
            assert notice.is_open() and reviews == 1
        notice.open(False)
        app.step(.4)
        app.dispatch()
        assert not notice.is_open() and dismissals == 1


if __name__ == "__main__":
    main()
