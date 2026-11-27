"""Build a retained account menu and exercise right-click or primary activation and Escape."""
from cui_bridge import App, button, column, popover


def main():
    with App() as app:
        trigger = button("account").label("Account")
        close = button("close").label("Close account menu")
        menu = popover("account-menu").anchor(trigger).children(close)
        right_click = app.supports_popover_secondary_trigger()
        if app.supports_popover_options():
            menu.popover_options(anchor=0, width=220, height=96, margin=8,
                                 hide_close=True, focus_content=True, secondary_trigger=right_click)
        trigger.on_click(lambda event: menu.open())
        close.on_click(lambda event: menu.open(False))
        app.mount(column().children(trigger, menu)).step()

        x, y, width, height = app.bounds(trigger)
        mouse_button = "secondary" if right_click else "primary"
        app.queue_pointer("down", x + width / 2, y + height / 2, button=mouse_button)
        app.queue_pointer("up", x + width / 2, y + height / 2, button=mouse_button)
        app.step()
        app.dispatch()
        app.step()
        assert menu.is_open()

        app.queue_key("down", "escape")
        app.queue_key("up", "escape")
        app.step()
        assert not menu.is_open()


if __name__ == "__main__":
    main()
