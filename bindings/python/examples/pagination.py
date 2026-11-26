"""An interactive results pager. Use --font FONT.ttf to open an SDL window."""
import argparse

from cui_bridge import App, PaginationConfig, column, label, pagination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="Use this TrueType font in the native window")
    args = parser.parse_args()

    summary = label("Page 1 of 24")
    results = [label("", key=f"result-{slot}") for slot in range(3)]
    current = 1

    def render_results(page):
        nonlocal current
        current = page
        summary.label(f"Page {page} of 24")
        for slot, row in enumerate(results, 1):
            row.label(f"Document {(page - 1) * len(results) + slot:02d}")

    render_results(1)
    pager = pagination("results-pages").configure(PaginationConfig(
        chevrons=True, jump_menu=True, visible_pages=5, available_width=320,
        group="Results pages",
    )).design_size("base").custom_size(52.5)
    pager.total(24).on_change(lambda event: render_results(int(event.value)))

    with App(backend="sdl" if args.font else "none", font_path=args.font,
             title="Document results", width=440, height=320) as app:
        app.mount(column("results").children(label("Documents"), summary, *results, pager))
        if args.font:
            app.run()
        else:
            app.step()
            x, y, width, height = app.numeric_part_bounds(pager, 3)
            app.queue_pointer("down", x + width / 2, y + height / 2)
            app.queue_pointer("up", x + width / 2, y + height / 2)
            app.step()
            assert app.dispatch() == 1
            assert current == 2


if __name__ == "__main__":
    main()
