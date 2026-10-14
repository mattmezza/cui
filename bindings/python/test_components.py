"""Real bridge component state, input and copied-event regression tests."""
import unittest
from cui_bridge import dropdown_button, select, combobox, collapsible, alert, radio_group, tabs, accordion, item, label, App, column, checkbox, radio, switch, toggle, slider, progress, badge, input, textarea, number_input

class Components(unittest.TestCase):
    def press(self, app, node):
        x,y,w,h=app.bounds(node)
        app.queue_pointer('down',x+w/2,y+h/2)
        app.queue_pointer('up',x+w/2,y+h/2)
        app.step()

    def test_boolean_events_and_disabled(self):
        for factory in (checkbox,radio,switch,toggle):
            with self.subTest(factory=factory.__name__), App(width=800,height=800) as app:
                events=[]
                first=factory('first','First').on_change(events.append)
                second=factory('second','Disabled').disabled()
                app.mount(column().children(first,second));app.step()
                self.press(app,first);app.dispatch()
                self.assertTrue(first.is_checked());self.assertTrue(events[0].checked)
                self.press(app,second);self.assertEqual(app.dispatch(),0)
                self.assertFalse(second.is_checked())

    def test_numeric_ranges_and_static(self):
        with App(width=800,height=800) as app:
            s=slider('slider').range(10,100,5).value(30)
            n=number_input('number').range(-100,100,1).value(-20)
            p=progress('progress').value(.5)
            b=badge('badge','Ready')
            app.mount(column().children(s,n,p,b));app.step()
            self.assertEqual(s.current_value(),30)
            self.assertEqual(n.current_value(),-20)
            self.assertEqual(p.current_value(),.5)
            self.assertGreater(app.bounds(b)[2],0)
            with self.assertRaises(ValueError):s.value(float('nan'))
            self.assertEqual(s.current_value(),30)

    def test_copied_utf8_input(self):
        for factory in (input,textarea):
            with self.subTest(factory=factory.__name__), App(width=800,height=800) as app:
                events=[];field=factory('field').on_change(events.append)
                app.mount(column().children(field));app.step();self.press(app,field)
                app.queue_text('é');app.step()
                app.queue_text('✓');app.step()
                self.assertEqual(field.current_text(),'é✓')
                app.dispatch()
                self.assertEqual([e.text for e in events],['é','é✓'])
                with self.assertRaises(ValueError):app.queue_text('bad\0text')
                app.close()
                with self.assertRaises(RuntimeError):field.current_text()

    def test_group_selection_and_disclosures(self):
        with App(width=800,height=800) as app:
            a=item('a','First');b=item('b','Second');events=[]
            group=radio_group('group').children(a,b).select(a).on_change(events.append)
            content=label('Visible content');fold=item('fold','Details').children(content)
            disclosure=accordion('accordion').children(fold)
            app.mount(column().children(group,disclosure));app.step()
            self.assertIs(group.selected(),a)
            self.press(app,b);app.dispatch();self.assertIs(group.selected(),b)
            self.assertEqual(events[0].selected_handle,(b._handle.slot,b._handle.generation))
            with self.assertRaises(Exception):app.bounds(content)
            self.press(app,fold);self.assertGreater(app.bounds(content)[2],0)
            with self.assertRaises(ValueError):group.select(item('foreign'))
        for factory in (collapsible,tabs):
            with App(width=800,height=800) as app:
                content=label('Panel content')
                if factory is tabs:
                    panel=item('tab','Tab').children(content)
                    node= factory('node').children(panel).select(panel)
                else:
                    node=factory('node','Details').children(content).open()
                app.mount(column().children(node,alert('alert','Notice').detail('More detail')))
                app.step();self.assertGreater(app.bounds(content)[2],0)

    def test_select_and_combobox(self):
        with App(width=800,height=800) as app:
            events=[];a=item('a','Apple');b=item('b','Banana')
            field=select('select','Fruit').detail('Choose').children(a,b).on_change(events.append)
            combo=combobox('combo','Ch').detail('Search').children(item('cherry','Cherry'))
            app.mount(column().children(field,combo));app.step()
            self.press(app,field);self.press(app,b);app.dispatch()
            self.assertIs(field.selected(),b)
            self.assertEqual(events[0].selected_handle,(b._handle.slot,b._handle.generation))
            self.press(app,combo);app.queue_text('e');app.step()
            self.assertEqual(combo.current_text(),'Che')

    def test_dropdown_split_action_and_menu(self):
        with App() as app:
            events=[];option=item('export','Export CSV')
            node=dropdown_button('dropdown','Export').children(option).on_click(events.append).on_change(events.append)
            app.mount(column().children(node));app.step()
            for part in ('action','menu'):
                x,y,w,h=app.dropdown_bounds(node,part)
                app.queue_pointer('down',x+w/2,y+h/2);app.queue_pointer('up',x+w/2,y+h/2)
                app.step();app.dispatch()
            self.assertEqual(events[0].type,'click')
            self.press(app,option);app.dispatch()
            self.assertEqual(events[-1].selected_handle,(option._handle.slot,option._handle.generation))

if __name__=='__main__':unittest.main()
