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

    def test_message_scroller_and_carousel(self):
        with c.App() as app:
            messages=c.message_scroller('messages').visual_size(280,100).children(*[c.item('m'+str(i),'Message '+str(i)).detail('Alex').step_status(i%2) for i in range(20)])
            app.mount(c.column().children(messages))
            with self.assertRaises(c.BridgeError): messages.scroll_to(0)
            app.step();self.assertGreater(messages.scroll_offset(),0)
            messages.scroll_to(0);app.step();self.assertEqual(messages.scroll_offset(),0)
            with self.assertRaises(ValueError): messages.scroll_to(-1)
        with c.App() as app:
            events=[];slides=[c.item('s'+str(i),'Slide '+str(i)) for i in range(3)]
            carousel=c.carousel('slides').visual_size(240,100).children(*slides).on_change(events.append)
            app.mount(c.column().children(carousel));app.step();self.assertIs(carousel.selected(),slides[0])
            x,y,w,h=app.bounds(carousel);app.queue_scroll(x+30,y+30,-1,0);app.step();app.dispatch()
            self.assertIs(carousel.selected(),slides[1]);self.assertEqual(events[-1].selected_handle,(slides[1]._handle.slot,slides[1]._handle.generation))
            carousel.select(slides[2]).disabled();app.step();events.clear();app.queue_scroll(x+30,y+30,1,0);app.step();app.dispatch()
            self.assertIs(carousel.selected(),slides[2]);self.assertEqual(events,[])

    def test_resizable_native_drag(self):
        with c.App() as app:
            events=[];first=c.item('first','First').panel_size(240).on_change(events.append);second=c.item('second','Second')
            split=c.resizable('split').visual_size(420,160).children(first,second)
            app.mount(c.column().children(split));app.step();before=first.current_panel_size()
            x,y,w,h=app.resizable_handle_bounds(split,first);x+=w/2;y+=h/2
            for phase,px in [('down',x),('move',x+30),('up',x+30)]:app.queue_pointer(phase,px,y);app.step()
            app.dispatch();after=first.current_panel_size();self.assertGreater(after,before);self.assertGreater(events[-1].value,before)
            with self.assertRaises(c.BridgeError):app.resizable_handle_bounds(split,second)
            split.disabled();app.step();x,y,w,h=app.resizable_handle_bounds(split,first);x+=w/2;y+=h/2;events.clear()
            for phase,px in [('down',x),('move',x-30),('up',x-30)]:app.queue_pointer(phase,px,y);app.step()
            app.dispatch();self.assertEqual(first.current_panel_size(),after);self.assertEqual(events,[])

    def test_text_view_rendered_selection(self):
        with c.App() as app:
            events=[];view=c.text_view('view','**Hello** world').visual_size(320,120).on_change(events.append)
            app.mount(c.column().children(view));app.step();view.select_text(0,5);self.assertEqual(view.selection_text(),'Hello');self.assertEqual(view.current_text(),'**Hello** world')
            view.text_format('html').label('<b>HTML</b> text');app.step();view.select_text(0,4);self.assertEqual(view.selection_text(),'HTML')
            view.text_format('plain').label('é text');app.step()
            with self.assertRaises(c.BridgeError):view.select_text(0,1)
            view.select_text(0,2);self.assertEqual(view.selection_text(),'é')
            view.label('Plain text');app.step();view.select_text(0,0);x,y,w,h=app.bounds(view)
            for phase,px in [('down',x+10),('move',x+60),('up',x+60)]:app.queue_pointer(phase,px,y+14)
            app.step();app.dispatch();self.assertTrue(events[-1].text)

    def test_editor_independent_native_editing(self):
        with c.App() as app:
            events=[];editor=c.editor('editor','first\nsecond').detail('c').visual_size(300,120).on_change(events.append);other=c.editor('other').visual_size(300,120)
            app.mount(c.column().children(editor,other));app.step();self.press(app,app.bounds(editor));app.queue_text('✓');app.step();app.dispatch()
            self.assertIn('✓',editor.current_text());self.assertEqual(events[-1].text,editor.current_text());self.assertEqual(other.current_text(),'')
            editor.disabled();app.step();events.clear();app.queue_text('X');app.step();app.dispatch();self.assertEqual(events,[])

    def test_chart_plot_and_composed_table(self):
        with c.App(height=700) as app:
            data=[['Jan',5],['Feb',8]];points=[[20,30],[60,50]]
            chart=c.chart('chart').chart_data(data).chart_kind('bar').visual_size(240,120);plot=c.plot('plot').plot_points(points).visual_size(240,120)
            table=c.table('table').visual_size(300,0).children(c.item('header').children(c.item('name','Name'),c.item('score','Score')),c.item('row').children(c.item('alex','Alex'),c.item('number','95')))
            data[0][0]='Changed';points[0][0]=999
            app.mount(c.column().children(chart,plot,table));app.step();self.assertEqual(chart.current_datum(0),('Jan',5));self.assertEqual(plot.current_point(0),(20,30));self.assertGreater(app.bounds(table)[2],0)
            with self.assertRaises(ValueError): chart.chart_data([['bad',float('nan')]])
            with self.assertRaises(ValueError): plot.plot_points([[float('inf'),0]])
            self.assertEqual(chart.current_datum(1),('Feb',8));self.assertEqual(plot.current_point(1),(60,50))
            chart.chart_data([['negative',-2]])
            with self.assertRaises(ValueError):chart.chart_kind('pie')
            self.assertEqual(chart.current_datum(0),('negative',-2))
            chart.chart_data([['valid',2]]).chart_kind('pie');app.step()

    def test_data_table_exact_selection_and_sort_snapshots(self):
        import json
        with c.App() as app:
            events=[];rows=[['Alex','95','A'],['Bob','87','B']]
            table=c.data_table('results').table_data(['Name','Score','Team'],rows).selection_mode('cell').visual_size(400,180).on_change(events.append);rows[0][0]='Changed'
            app.mount(c.column().children(table));app.step();self.assertEqual(table.current_cell(0,0),'Alex')
            with self.assertRaises(ValueError):table.table_data(['Name'],[['extra','column']])
            self.assertEqual(table.current_cell(1,0),'Bob')
            self.press(app,table.table_cell_bounds(1,2));self.press(app,table.table_header_bounds(2));app.dispatch()
            snapshots=[json.loads(e.text) for e in events];self.assertEqual(snapshots[0]['reason'],'selection');self.assertEqual(snapshots[0]['row'],2);self.assertEqual(snapshots[0]['column'],3);self.assertEqual(snapshots[0]['sortColumn'],0)
            self.assertEqual(snapshots[1]['reason'],'sort');self.assertEqual(snapshots[1]['sortColumn'],3);self.assertGreater(snapshots[1]['sortDirection'],0)
            self.assertEqual(table.table_state()['sortColumn'],3)
            table.table_data(['Name','Score','Team'],[['Bob','87','B'],['Alex','95','A']]);app.step();self.assertEqual(table.current_cell(0,0),'Bob')
            table.disabled();app.step();events.clear();self.press(app,table.table_header_bounds(0));app.dispatch();self.assertEqual(events,[])

    def test_command_native_filter_and_confirmation(self):
        with c.App() as app:
            events=[];open_item=c.item('open','Open');close=c.item('close','Close').detail('exit').on_click(events.append)
            command=c.command('actions').visual_size(300,150).children(open_item,close)
            app.mount(c.column().children(command));app.step();self.assertIs(command.selected(),open_item)
            self.press(app,command.item_bounds(close));app.dispatch();self.assertIs(command.selected(),close);self.assertIs(events[-1].node,close)
            command.label('exit');app.step();self.assertEqual(command.current_text(),'exit');self.assertIs(command.selected(),close)
            close.disabled();app.step();self.assertIsNone(command.selected())
            with self.assertRaises(ValueError):command.select(close)

    def test_virtual_list_native_wheel_and_visible_rows(self):
        with c.App() as app:
            events=[];items=[c.item('i'+str(i),'Row '+str(i)) for i in range(20)]
            virtual=c.virtual_list('rows').visual_size(200,96).children(*items).on_change(events.append)
            app.mount(c.column().children(virtual));app.step();self.assertGreater(virtual.item_bounds(items[0])[2],0)
            with self.assertRaises(c.BridgeError):virtual.item_bounds(items[-1])
            x,y,w,h=app.bounds(virtual);app.queue_scroll(x+20,y+20,0,-1);app.step();app.dispatch();self.assertGreater(virtual.scroll_offset(),0);self.assertGreater(events[-1].value,0)
            virtual.scroll_to(500);app.step();self.assertGreater(virtual.item_bounds(items[-1])[2],0)
            virtual.disabled();app.step();before=virtual.scroll_offset();events.clear();app.queue_scroll(x+20,y+20,0,1);app.step();app.dispatch();self.assertEqual(virtual.scroll_offset(),before);self.assertEqual(events,[])

    def test_sidebar_native_navigation(self):
        with c.App() as app:
            changes=[];clicks=[];home=c.item('home','Home');settings=c.item('settings','Settings').on_click(clicks.append)
            sidebar=c.sidebar('navigation').visual_size(240,180).children(home,settings).on_change(changes.append)
            app.mount(c.column().children(sidebar));app.step();self.press(app,app.bounds(settings));app.dispatch();self.assertIs(sidebar.selected(),settings);self.assertIs(clicks[-1].node,settings);self.assertEqual(changes[-1].selected_handle,(settings._handle.slot,settings._handle.generation))
            sidebar.open(False);app.step();self.assertFalse(sidebar.is_open());sidebar.open();app.step();self.assertTrue(sidebar.is_open())
            home.disabled();app.step();changes.clear();self.press(app,app.bounds(home));app.dispatch();self.assertIs(sidebar.selected(),settings);self.assertEqual(changes,[])
        with c.App() as app:
            sidebar=c.sidebar('closed').open(False).children(c.item('home','Home'));app.mount(c.column().children(sidebar));app.step();self.assertFalse(sidebar.is_open())

    def test_native_theme_presets(self):
        with c.App() as app:
            self.assertIsNone(app.current_theme_preset());names=app.theme_presets();self.assertGreaterEqual(len(names),3);self.assertTrue(all(names))
            app.mount(c.column().children(c.button('action').label('Action')))
            for index in range(len(names)):app.theme_preset(index);app.step();self.assertEqual(app.current_theme_preset(),index)
            with self.assertRaises(c.BridgeError):app.theme_preset(len(names))
            self.assertEqual(app.current_theme_preset(),len(names)-1)

    def test_native_root_view_padding_and_child_callback(self):
        with c.App() as app:
            events=[];button=c.button('save').label('Save').on_click(events.append);root=c.root_view().padding(24).children(c.column().children(button))
            app.mount(root);app.step();self.assertGreaterEqual(app.bounds(button)[0],24);self.press(app,app.bounds(button));app.dispatch();self.assertIs(events[-1].node,button)
            root.padding(40);app.step();self.assertGreaterEqual(app.bounds(button)[0],40)
        with c.App() as app:
            with self.assertRaises(c.BridgeError):app.mount(c.column().children(c.root_view('nested')))
            app.mount(c.root_view().children(c.label('Recovered')));app.step()

    def test_clipboard_headless_and_bounds(self):
        with c.App() as app:
            for operation in (lambda:app.clipboard_set('Unicode ✓'),app.clipboard_get):
                with self.assertRaises(c.BridgeError) as error:operation()
                self.assertEqual(error.exception.code,9)
            for text in ('bad\x00text','x'*65537):
                with self.assertRaises(ValueError):app.clipboard_set(text)
            with self.assertRaises(ValueError):c.plot('bad').plot_points([[16385,0]])
            with self.assertRaises(ValueError):c.data_table('bad').table_data(['Name'],[['x'*129]])

    def test_speech_host_reports_and_copied_lifecycle(self):
        with c.App() as app:
            events=[];speech=c.speech('recognition').on_change(events.append)
            app.mount(c.column().children(speech));app.step()
            with self.assertRaises(c.BridgeError):speech.speech_report('early',True)
            speech.speech_start()
            with self.assertRaises(c.BridgeError):speech.speech_start()
            speech.speech_report('hel');app.step();speech.speech_report('hello ✓',True);app.step();app.dispatch()
            self.assertEqual([e.text for e in events],['','hel','hello ✓']);self.assertEqual([e.value for e in events],[0,1,1]);self.assertTrue(speech.speech_active());self.assertEqual(speech.speech_transcript(),'hello ✓')
            speech.speech_finish();app.step();app.dispatch();self.assertEqual(events[-1].value,2);self.assertFalse(events[-1].checked);self.assertFalse(speech.speech_active());self.assertEqual(speech.speech_transcript(),'hello ✓')
            speech.disabled()
            with self.assertRaises(c.BridgeError):speech.speech_start()

    def test_questionnaire_required_choices_and_queued_snapshots(self):
        import json
        with c.App() as app:
            events=[];red=c.item('red','Red');blue=c.item('blue','Blue');round_choice=c.item('round','Round')
            color=c.item('color','Choose a color').question(True).children(red,blue);shape=c.item('shape','Choose a shape').question(False).children(round_choice)
            survey=c.questionnaire('survey').children(color,shape).on_change(events.append)
            app.mount(c.column().children(survey));app.step();self.assertIs(survey.questionnaire_state()['current'],color);self.assertFalse(survey.questionnaire_state()['complete'])
            with self.assertRaises(c.BridgeError):survey.questionnaire_next()
            with self.assertRaises(c.BridgeError):red.label('Changed after freeze')
            self.press(app,app.bounds(red));self.press(app,app.bounds(blue));app.dispatch();self.assertEqual([json.loads(e.text)['selected'] for e in events],[[1],[2]])
            self.assertTrue(blue.choice_selected());self.assertFalse(red.choice_selected());survey.questionnaire_next();app.step();self.assertIs(survey.questionnaire_state()['current'],shape);self.assertGreater(app.bounds(round_choice)[2],0)
            survey.questionnaire_previous();app.step();self.assertIs(survey.questionnaire_state()['current'],color)
            survey.questionnaire_next();app.step()
            with self.assertRaises(c.BridgeError):survey.questionnaire_submit()
            self.press(app,app.bounds(round_choice));survey.questionnaire_submit();app.step();self.assertTrue(survey.questionnaire_state()['complete'])

    def test_dock_native_layout_activation_and_freeze(self):
        with c.App(width=700,height=500) as app:
            events=[];first=c.item('first','First').children(c.label('Panel body'));second=c.item('second','Second').children(c.input('editor','Notes'))
            dock=c.dock('workspace').visual_size(480,300).children(first,second).on_change(events.append)
            app.mount(c.column().children(dock));app.step();self.assertIs(dock.dock_active(),first);self.assertGreater(dock.dock_panel_bounds(second)[2],0)
            before=dock.dock_serialize();self.assertTrue(before.startswith("CUIDOCK 1 "))
            with self.assertRaises(ValueError):dock.dock_move_split(second,first,'diagonal')
            self.assertEqual(dock.dock_serialize(),before)
            with self.assertRaises(c.BridgeError):second.label('Frozen panel metadata')
            dock.dock_move_split(second,first,'horizontal',.4);app.step();after=dock.dock_serialize();self.assertTrue(after.startswith("CUIDOCK 1 "));self.assertNotEqual(before,after)
            self.assertNotEqual(dock.dock_panel_bounds(first)[0],dock.dock_panel_bounds(second)[0])
            dock.dock_activate(second);app.dispatch();self.assertIs(dock.dock_active(),second);self.assertEqual(events[-1].selected_handle,(second._handle.slot,second._handle.generation))
            dock.disabled()
            with self.assertRaises(c.BridgeError):dock.dock_activate(first)

    def test_settings_native_fields_page_snapshots_and_reset(self):
        with c.App(width=900, height=600) as app:
            fields=[]; pages=[]
            switch=c.switch('updates','Automatic updates').checked().on_change(fields.append)
            name=c.input('name','Alex').detail('Display name').on_change(fields.append)
            count=c.number_input('count').detail('Retry count').range(0,10).value(3).on_change(fields.append)
            general=c.item('general','General').children(c.item('account','Account').children(switch,name,count))
            advanced=c.item('advanced','Advanced').children(c.item('network','Network').children(c.checkbox('proxy','Use proxy')))
            settings=c.settings('preferences').visual_size(760,400).children(general,advanced).on_change(pages.append)
            app.mount(c.column().children(settings))
            with self.assertRaises(c.BridgeError):settings.settings_page()
            app.step(); self.assertIs(settings.settings_page(),general)
            self.press(app,settings.settings_field_bounds(switch)); app.dispatch()
            self.assertFalse(switch.is_checked()); self.assertFalse(fields[-1].checked)
            self.press(app,settings.settings_field_bounds(name)); app.queue_text('é'); app.step(); app.dispatch()
            self.assertIn('é',name.current_text()); self.assertEqual(fields[-1].text,name.current_text())
            count.value(7); settings.settings_select(advanced); settings.settings_select(general); app.dispatch()
            self.assertEqual([e.selected_handle for e in pages],[(advanced._handle.slot,advanced._handle.generation),(general._handle.slot,general._handle.generation)])
            fields.clear(); settings.settings_reset(general); app.dispatch()
            self.assertTrue(switch.is_checked()); self.assertEqual(name.current_text(),'Alex'); self.assertEqual(count.current_value(),3); self.assertEqual(len(fields),3)
            with self.assertRaises(c.BridgeError):general.label('Frozen title')
            with self.assertRaises(ValueError):settings.settings_query('é'*129)
            settings.settings_query('Advanced'); app.step()
            with self.assertRaises(c.BridgeError):settings.settings_field_bounds(switch)
            settings.settings_query(''); settings.settings_select(general); app.step(); switch.disabled(); fields.clear()
            self.press(app,settings.settings_field_bounds(switch)); app.dispatch(); self.assertEqual(fields,[])
            with self.assertRaises(ValueError):settings.disabled()
        with c.App() as foreign:
            field=c.input('foreign','value'); foreign.mount(c.column().children(field)); foreign.step()
            with self.assertRaises(ValueError):settings.settings_field_bounds(field)

if __name__=='__main__':unittest.main()
