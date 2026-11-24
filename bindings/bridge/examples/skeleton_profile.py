"""Build a profile loading UI in Python and reveal its content with a button."""
import argparse
import math
import os

from cui_bridge import App, SkeletonConfig, SkeletonWidth, button, column, label, responsive_skeleton


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="TTF or OTF font for the interactive SDL window")
    parser.add_argument("--smoke", action="store_true", help="exercise the UI on a private test display")
    args = parser.parse_args()
    if args.smoke and (not args.font or os.environ.get("CUI_TEST_VIRTUAL_DISPLAY") != "1"):
        parser.error("--smoke requires --font and the private display from scripts/test_local.py")

    avatar = responsive_skeleton("profile-avatar", SkeletonConfig(
        width_mode=SkeletonWidth.FIXED, width=40, height=40, radius=20))
    name = responsive_skeleton("profile-name", SkeletonConfig(width=.65, radius=6))
    role = responsive_skeleton("profile-role", SkeletonConfig(width=.4, height=12, radius=6,
                                                              secondary=True))
    description = responsive_skeleton("profile-description", SkeletonConfig(height=36, radius=8))
    placeholders = (avatar, name, role, description)
    title = label("Workspace profile", key="title")
    status = label("Loading profile…", key="status")
    finish = button("finish").primary().label("Finish loading")
    loaded = False

    def reveal(_event):
        nonlocal loaded
        for placeholder in placeholders:
            placeholder.remove()
        title.label("Alex Morgan")
        status.label("Workspace admin · alex@example.com")
        finish.disabled(True).label("Profile loaded")
        loaded = True

    finish.on_click(reveal)
    tree = column("root").gap(10).children(title, *placeholders, status, finish)
    options = dict(width=480, height=320, title="Python Skeleton profile")
    if args.font:
        options.update(backend="sdl", font_path=args.font, initially_hidden=args.smoke)
    with App(**options) as app:
        app.mount(tree)
        if args.font and not args.smoke:
            app.run()
            return

        app.step(.25)
        assert app.bounds(avatar)[2:] == (40, 40)
        assert math.isclose(app.bounds(name)[2], (480 - 32) * .65, abs_tol=.001)
        assert app.bounds(description)[2:] == (448, 36)
        sample = name.presentation()
        assert sample and sample.next_wake_seconds > sample.core_time
        before = app.bounds(status)[1]
        x, y, width, height = app.bounds(finish)
        app.queue_pointer("down", x + width / 2, y + height / 2)
        app.queue_pointer("up", x + width / 2, y + height / 2)
        app.step()
        app.dispatch()
        app.step()
        assert loaded and app.bounds(status)[1] < before
        print("Responsive Skeleton profile: loaded and placeholders released")


if __name__ == "__main__":
    main()
