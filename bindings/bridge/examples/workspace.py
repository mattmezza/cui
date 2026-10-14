"""Native account/settings workspace, or deterministic window-free validation."""
import argparse
import os
from cui_bridge import App, button, checkbox, column, date_picker, dialog, input, item, label, tabs

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--headless', action='store_true')
args = parser.parse_args()
status = label('Ready', key='status')
name = input('name', 'Alex Morgan').on_change(lambda e: status.label('Name: ' + e.text))
notifications = checkbox('notifications', 'Email notifications').checked()
birthday = date_picker('birthday', 'Birthday').date_value(1980, 1, 1)
confirmation = dialog('confirmation', 'Save account changes').detail('Confirm the new account details.')
confirmation.children(label('Your settings stay in your application model.'))
confirmation.on_close(lambda e: status.label('Dialog closed'))
save = button('save').label('Save changes').primary().on_click(lambda e: confirmation.open())
account = item('account', 'Account').children(column().gap_units(2).children(name, birthday, save, status))
settings = item('settings', 'Preferences').children(notifications)
pages = tabs('pages').children(account, settings).select(account)
options = dict(width=720, height=480, title='CUI account workspace')
if not args.headless:
    options.update(backend='sdl', font_path=os.environ['CUI_TEST_FONT'])
with App(**options) as app:
    app.mount(column().gap_units(3).children(label('Workspace'), pages, confirmation))
    if args.headless:
        app.step()
        x,y,w,h=app.bounds(save)
        app.queue_pointer('down',x+w/2,y+h/2);app.queue_pointer('up',x+w/2,y+h/2)
        app.step();app.dispatch();app.step()
        assert confirmation.is_open()
        confirmation.open(False);app.step();app.dispatch()
        pages.select(settings);app.step()
        assert app.bounds(notifications)[2] > 0
        print('PASS account workspace: native button, dialog and page selection')
    else:
        app.run()
