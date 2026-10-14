"""Validate owned native models and exact copied event payloads through Python."""
import unittest
import cui_bridge as c

class Models(unittest.TestCase):
    def press(self,app,r):
        x,y,w,h=r
        self.assertGreater(w,0); self.assertGreater(h,0)
        app.queue_pointer('down',x+w/2,y+h/2); app.queue_pointer('up',x+w/2,y+h/2);app.step()

    def test_font_fallback_validation(self):
        with c.App() as app:
            with self.assertRaises(ValueError): app.font_fallbacks([''] )
            with self.assertRaises(ValueError): app.font_fallbacks(['font.ttf'] * 33)
            with self.assertRaises(c.BridgeError) as error: app.font_fallbacks([])
            self.assertEqual(error.exception.code,9)

    def test_owned_images_and_attachment(self):
        with c.App() as app:
            pixels=bytearray([255,0,0,255]*4)
            image=c.image('image').image_rgba(pixels,2,2).visual_size(80,80)
            pixels.clear()  # The builder must already own the copy.
            attachment=c.attachment('file','plan.pdf').detail('24 KB').attachment_status('uploading').value(.4).visual_size(250,60)
            app.mount(c.column().children(image,attachment));app.step()
            self.assertGreater(app.bounds(image)[2],0);self.assertEqual(attachment.current_value(),.4)
            with self.assertRaises(ValueError):image.image_rgba(b'bad',2,2)
            image.image_rgba(bytes([0,255,0,255]*4),2,2);app.step()

    def test_tree(self):
        with c.App() as app:
            events=[];parent=c.item('parent','Project').step_status(0).checked(True)
            child=c.item('child','Notes').step_status(1)
            tree=c.tree('tree').visual_size(240,180).children(parent,child).on_change(events.append)
            app.mount(c.column().children(tree));app.step();self.press(app,app.bounds(child));app.dispatch()
            self.assertIs(tree.selected(),child);self.assertEqual(events[-1].selected_handle,(child._handle.slot,child._handle.generation))
            child.disabled();app.step();events.clear();self.press(app,app.bounds(child));app.dispatch();self.assertEqual(events,[])

    def test_time_and_color(self):
        with c.App(height=700) as app:
            events=[];time=c.time_field('time').time_value(9,45,30).on_change(events.append)
            color=c.color_picker('color').color_rgba(0xff0000ff).visual_size(240,0).on_change(events.append)
            app.mount(c.column().children(time,color));app.step();self.press(app,app.bounds(time));app.queue_key('down','up');app.step();app.dispatch()
            self.assertNotEqual(time.current_time(),(9,45,30));self.assertEqual(events[-1].text,'%02d:%02d:%02d'%time.current_time())
            events.clear();self.press(app,app.color_part_bounds(color,1));app.dispatch()
            self.assertNotEqual(color.current_color(),0xff0000ff);self.assertEqual(events[-1].value,color.current_color())
            self.assertEqual(events[-1].text,'#%08X'%color.current_color())
            before=color.current_color();color.disabled();app.step();self.press(app,app.color_part_bounds(color,1));self.assertEqual(color.current_color(),before)

    def test_dates(self):
        with c.App(width=800,height=650) as app:
            events=[];calendar=c.calendar('calendar').date_value(2024,3,5).on_change(events.append)
            picker=c.date_picker('picker','Choose').date_value(1980,1,1).on_change(events.append)
            with self.assertRaises(ValueError):picker.date_value(2024,2,30)
            app.mount(c.column().children(calendar,picker));app.step();self.press(app,app.calendar_day_bounds(calendar,2024,3,10));app.dispatch()
            self.assertEqual(calendar.current_date(),(2024,3,10));self.assertEqual((events[-1].value,events[-1].text),(20240310,'2024-03-10'))
            picker.open();app.step();self.assertTrue(picker.is_open());picker.open(False);self.assertFalse(picker.is_open())

    def test_form_and_input_group(self):
        with c.App(width=700,height=500) as app:
            events=[];save=c.button('save').label('Save').on_click(events.append)
            form=c.form('form','Profile').children(c.item('field','Name').detail('Public name').children(c.input('name','Alex')),c.item('action','Action').children(save))
            app.mount(c.column().children(form));app.step();self.press(app,app.bounds(save));app.dispatch();self.assertEqual(len(events),1)
            form.disabled();app.step();events.clear();self.press(app,app.bounds(save));app.dispatch();self.assertEqual(events,[])
        with c.App() as app:
            actions=[];changes=[];addon=c.item('go','Go').step_status(1).on_click(actions.append)
            group=c.input_group('search').detail('Search').visual_size(320,0).children(addon).on_change(changes.append)
            app.mount(c.column().children(group));app.step();self.press(app,app.bounds(addon));app.dispatch();self.assertEqual(len(actions),1)
            self.press(app,app.bounds(group));app.queue_text('a');app.step();app.dispatch();self.assertEqual(group.current_text(),'a');self.assertEqual(changes[-1].text,'a')
            group.disabled();app.step();changes.clear();app.queue_text('b');app.step();app.dispatch();self.assertEqual(changes,[])

    def test_focus_trap(self):
        with c.App() as app:
            events=[];outside=c.button('outside').label('Outside').on_click(events.append)
            inside=c.button('inside').label('Inside').on_click(events.append)
            trap=c.focus_trap('trap').children(inside).open()
            app.mount(c.column().children(outside,trap));app.step();self.press(app,app.bounds(outside));app.dispatch();self.assertEqual(events,[])
            self.press(app,app.bounds(inside));app.dispatch();self.assertIs(events[-1].node,inside)
            trap.open(False);app.step();self.press(app,app.bounds(outside));app.dispatch();self.assertIs(events[-1].node,outside)

    def test_shared_notifications(self):
        with c.App(width=800,height=650) as app:
            clicked=[];closed=[]
            first=c.notification('first','Saved').detail('File saved').open().on_click(clicked.append).on_close(closed.append)
            second=c.notification('second','Synced').open().on_close(closed.append)
            app.mount(c.column().children(first,second));app.step(.4)
            self.assertNotEqual(app.bounds(first)[1],app.bounds(second)[1]);self.press(app,app.bounds(first));app.dispatch()
            self.assertEqual(len(clicked),1);self.assertFalse(first.is_open());self.assertTrue(second.is_open())
            app.step(.3);app.dispatch();self.assertEqual(len(closed),1)
            second.open(False);app.step(.3);app.dispatch();self.assertEqual(len(closed),2)

    def test_rejected_capabilities_do_not_silently_mount(self):
        with self.assertRaises(ValueError): c.description_list('description').select(c.item('outside'))
        with self.assertRaises(ValueError): c.status_bar('status').on_change(lambda e: None)
        with self.assertRaises(ValueError): c.image('image').image_rgba(b'1234', True, 1)
        with c.App() as app:
            invalid=c.item('bad','Item').open()
            with self.assertRaises(c.BridgeError): app.mount(c.column().children(c.tabs('tabs').children(invalid)))
            # Python rolls back the partial native mount, retaining a reusable App.
            app.mount(c.column().children(c.label('Corrected')));app.step()

    def test_otp(self):
        with c.App() as app:
            events=[];otp=c.otp_input('otp','12').total(4).on_change(events.append)
            app.mount(c.column().children(otp));app.step();self.assertEqual(otp.current_text(),'12');self.press(app,app.bounds(otp))
            app.queue_text('3');app.step();app.dispatch();self.assertEqual(otp.current_text(),'123');self.assertEqual(events[-1].text,'123')
            otp.disabled();app.step();events.clear();app.queue_text('4');app.step();app.dispatch();self.assertEqual(events,[]);self.assertEqual(otp.current_text(),'123')

if __name__=='__main__':unittest.main()
