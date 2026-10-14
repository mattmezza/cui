"""Native sidebar workspace; --headless checks real navigation and editing."""
import argparse
import os
from cui_bridge import App, button, checkbox, column, input, item, label, root_view, row, sidebar, tabs

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--headless', action='store_true')
args = parser.parse_args()
status = label('Ready', key='status')
name = input('name', 'Alex Morgan')
email = checkbox('email', 'Email notifications').checked()
save = button('save').label('Save changes').primary().rounded_md().on_click(
    lambda event: status.label('Saved account: ' + name.current_text()))
account = item('account-page', 'Account').children(
    column().gap_units(3).children(label('Account details'), name, save, status))
preferences = item('preferences-page', 'Preferences').children(
    column().gap_units(3).children(label('Notifications'), email))
pages = tabs('pages').children(account, preferences).select(account)
account_action = item('account-action', 'Account').on_click(lambda event: pages.select(account))
preferences_action = item('preferences-action', 'Preferences').on_click(lambda event: pages.select(preferences))
navigation = sidebar('navigation').visual_size(180, 280).children(account_action, preferences_action)
root = root_view().padding(24).children(column().gap_units(4).children(
    label('CUI workspace'), row().gap_units(6).children(navigation, pages)))
options = dict(width=820, height=460, title='CUI sidebar workspace')
if not args.headless:
    options.update(backend='sdl', font_path=os.environ['CUI_TEST_FONT'])
with App(**options) as app:
    app.mount(root)
    if args.headless:
        def press(node):
            x, y, width, height = app.bounds(node)
            assert width > 0 and height > 0
            app.queue_pointer('down', x + width/2, y + height/2)
            app.queue_pointer('up', x + width/2, y + height/2)
            app.step(); app.dispatch(); app.step()
        app.step(); press(preferences_action)
        assert pages.selected() is preferences and app.bounds(email)[2] > 0
        press(account_action); press(name); app.queue_text('!'); app.step(); app.dispatch()
        press(save)
        assert status._text == 'Saved account: ' + name.current_text() and name.current_text().endswith('!')
        print('PASS sidebar workspace: navigation, native input and save callback')
    else:
        app.run()
