"""Open an account confirmation. Pass --font to open an SDL window."""
from argparse import ArgumentParser

from cui_bridge import App, alert_dialog, button, column, dialog_slot, label


def click(app, control):
    x, y, width, height = app.bounds(control)
    for phase in ('down', 'up'):
        app.queue_pointer(phase, x + width / 2, y + height / 2)
    app.step()
    app.dispatch()


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--font', help='TTF font for the optional SDL window')
    font = parser.parse_args().font
    with App(title='Account settings', width=560, height=360,
             backend='sdl' if font else 'none', font_path=font) as app:
        if not app.supports_dialog_composition():
            raise RuntimeError('This example requires retained dialog composition')
        status = label('Local account preferences are active.')
        confirm = button('Remove account').dialog_action('confirm')
        cancel = button('Keep account').outline().dialog_action('cancel')
        alert = (alert_dialog('remove', 'Remove this account?')
                 .detail('Only this device is affected.')
                 .dialog_options(centered=True).children(
                     dialog_slot('details', 'body').children(
                         label('Remove saved preferences and account information.')),
                     dialog_slot('actions', 'footer').children(cancel, confirm)))
        removed = []

        def update_status(event):
            if event.value == 1:
                status.label('Local account preferences removed.')
                removed.append(True)

        alert.on_close(update_status)
        trigger = button('Manage account').on_click(lambda _: alert.open())
        app.mount(column().gap(12).children(label('Account settings'), status, trigger, alert))
        if font:
            app.run()
            return
        app.step()
        click(app, trigger)
        app.step()
        click(app, confirm)
        assert removed == [True] and not alert.is_open()
        print('Local account preferences removed.')


if __name__ == '__main__':
    main()
