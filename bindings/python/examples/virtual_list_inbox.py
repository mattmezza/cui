"""Retained inbox with callback-driven prepend/resize. --font FONT opens a window."""
import argparse
from cui_bridge import App, button, column, item, label, virtual_list


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font')
    args = parser.parse_args()
    with App(backend='sdl' if args.font else 'none', font_path=args.font,
             title='Inbox', width=360, height=320) as app:
        rows = [(item(f'message-{i}', f'Message {i}: project update'), 32) for i in range(20)]
        inbox = virtual_list('inbox').visual_size(320, 160).children(*(i for i, _ in rows))
        summary = label('Scroll the inbox, then prepend or expand a message.')
        sequence = 0
        def prepend(_):
            nonlocal sequence
            if len(rows) == 64:
                summary.label('Inbox capacity reached (64 messages).')
                return
            sequence += 1
            message = item(f'new-{sequence}', f'New message {sequence}')
            inbox.append_virtual_list_item(message)
            rows.insert(0, (message, 48))
            inbox.set_virtual_list_rows(rows)
        def resize(_):
            message, extent = rows[0]
            rows[0] = (message, 80 if extent != 80 else 32)
            inbox.set_virtual_list_rows(rows)
        supported = app.supports_virtual_list_rows()
        add = button('prepend').label('Prepend message').disabled(not supported).on_click(prepend)
        expand = button('resize').label('Expand first message').disabled(not supported).on_click(resize)
        app.mount(column('root').gap(8).children(add, expand, inbox, summary))
        if supported: inbox.preserve_virtual_list_anchor().set_virtual_list_rows(rows)
        if args.font:
            app.run()
        else:
            app.step()
            if supported:
                inbox.scroll_to(96.5); app.step()
                anchor = rows[3][0]
                y = inbox.item_bounds(anchor)[1]
                for control in (add, expand):
                    x, top, w, h = app.bounds(control)
                    app.queue_pointer('down', x+w/2, top+h/2)
                    app.queue_pointer('up', x+w/2, top+h/2)
                    app.step(); app.dispatch(); app.step()
                    assert inbox.item_bounds(anchor)[1] == y
                assert len(rows) == 21 and rows[0][1] == 80


if __name__ == '__main__': main()
