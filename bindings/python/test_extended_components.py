"""Headless interaction checks for surfaces, visuals, navigation and scrolling."""
import unittest
import cui_bridge as c

class Extended(unittest.TestCase):
    def press_rect(self,app,rect):
        x,y,w,h=rect
        self.assertGreater(w,0);self.assertGreater(h,0)
        app.queue_pointer('down',x+w/2,y+h/2);app.queue_pointer('up',x+w/2,y+h/2)
        app.step()

    def test_surfaces(self):
        for name in ('dialog','alert_dialog','sheet','popover'):
            with self.subTest(name=name),c.App() as app:
                trigger=c.button('trigger').label('Open');body=c.label('Body');events=[]
                node=getattr(c,name)('surface','Title').detail('Description').on_close(events.append)
                if name!='alert_dialog':node.children(body)
                if name=='popover':node.anchor(trigger)
                app.mount(c.column().children(trigger,node));app.step();node.open();app.step()
                self.assertTrue(node.is_open())
                if name!='alert_dialog':self.assertGreater(app.bounds(body)[2],0)
                node.open(False);app.step();app.dispatch()
                self.assertEqual([e.type for e in events],['close'])
                self.assertEqual(events[0].value,3)

    def test_hover_surfaces(self):
        for name in ('tooltip','hover_card'):
            with self.subTest(name=name),c.App() as app:
                trigger=c.button('trigger').label('Hover');body=c.label('Context')
                node=getattr(c,name)('hover','Help').anchor(trigger)
                if name=='hover_card':node.children(body)
                app.mount(c.column().children(trigger,node));app.step()
                x,y,w,h=app.bounds(trigger);app.queue_pointer('move',x+w/2,y+h/2);app.step(.5)
                self.assertTrue(node.is_open())
                if name=='hover_card':self.assertGreater(app.bounds(body)[2],0)
                app.queue_pointer('move',600,400);app.step(.5);self.assertFalse(node.is_open())

    def test_visuals(self):
        for name in ('avatar','bubble','empty_view','group_box','kbd','skeleton','spinner','tag','marker','shimmer','icon','message'):
            with self.subTest(name=name),c.App() as app:
                node=getattr(c,name)('visual','check' if name=='icon' else 'Example')
                if name=='group_box':node.children(c.label('Body'))
                if name in ('avatar','empty_view','group_box','skeleton','spinner','icon','message'):node.visual_size(80,40)
                if name in ('empty_view','message'):node.detail('Description')
                app.mount(c.column().children(node));app.step();self.assertGreater(app.bounds(node)[2],0)

    def test_navigation(self):
        for name in ('list_view','menu','stepper'):
            with self.subTest(name=name),c.App(width=800,height=800) as app:
                a=c.item('a','First');b=c.item('b','Second')
                node=getattr(c,name)('navigation','Choose').children(a,b);events=[];node.on_change(events.append)
                if name=='stepper':a.step_status(2);b.detail('Next step')
                app.mount(c.column().children(node));app.step()
                if name=='menu':self.press_rect(app,app.bounds(node))
                self.press_rect(app,app.numeric_part_bounds(node,2) if name=='stepper' else app.bounds(b))
                app.dispatch();self.assertIs(node.selected(),b);self.assertGreater(len(events),0)
        for name,part,total,start,expected in [('pagination',3,20,10,11),('rating',14,5,2,5)]:
            with c.App() as app:
                node=getattr(c,name)('numeric').total(total).value(start)
                app.mount(c.column().children(node));app.step()
                self.press_rect(app,app.numeric_part_bounds(node,part));self.assertEqual(node.current_value(),expected)

    def test_scroll_description_and_chrome(self):
        with c.App() as app:
            scroller=c.scrollable('scroll').visual_size(200,100).scroll_extent(200,1000).children(c.label('Content'))
            app.mount(c.column().children(scroller));app.step();x,y,w,h=app.bounds(scroller)
            app.queue_scroll(x+5,y+5,0,-80);app.step();self.assertGreater(scroller.scroll_offset(),0)
        for name in ('description_list','status_bar','toolbar','title_bar'):
            with self.subTest(name=name),c.App() as app:
                entry=c.item('entry','Title')
                if name=='description_list':entry.detail('Value')
                if name=='status_bar':entry.step_status(1)
                if name=='toolbar':entry.children(c.button('save').label('Save'))
                node=getattr(c,name)('chrome','Workspace').children(entry)
                app.mount(c.column().children(node));app.step();self.assertGreater(app.bounds(node)[2],0)

if __name__=='__main__':unittest.main()
