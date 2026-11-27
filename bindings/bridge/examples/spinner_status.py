"""Loading indicator beside application status, completed by a button interaction."""
import argparse

from cui_bridge import App, button, column, row, label, rotating_spinner


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="TTF or OTF font for the interactive SDL window")
    args = parser.parse_args()
    loading = rotating_spinner('loading')
    status = label('Syncing workspace…')
    finish = button('finish').primary().label('Finish sync')
    completed = False

    def complete(_event):
        nonlocal completed
        loading.remove()
        status.label('Workspace is up to date')
        finish.disabled(True).label('Sync complete')
        completed = True

    finish.on_click(complete)
    options = dict(width=480, height=180)
    if args.font:
        options.update(backend="sdl", font_path=args.font)
    with App(**options) as app:
        app.mount(column('root').gap(12).children(
            row('status').gap(8).children(loading, status), finish))
        if args.font:
            app.run()
            return
        app.step(.1)
        assert loading.presentation().next_wake_seconds > 0
        x, y, width, height = app.bounds(finish)
        app.queue_pointer('down', x+width/2, y+height/2)
        app.queue_pointer('up', x+width/2, y+height/2)
        app.step()
        app.dispatch()
        app.step()
        assert completed
        print('Spinner status: sync completed through pointer interaction')


if __name__ == '__main__':
    main()
