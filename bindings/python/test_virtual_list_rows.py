"""Headless retained VirtualList projection and live membership checks."""
import math
import os
import unittest
from unittest.mock import patch
import cui_bridge as c


class VirtualRows(unittest.TestCase):
    def test_partial_symbol_groups_keep_ordinary_list(self):
        import ctypes
        original = ctypes.CDLL
        for missing in ('cui_bridge_virtual_list_set_rows_v1', 'cui_bridge_virtual_list_preserve_anchor_v1'):
            class Partial:
                def __init__(self, *args, **kwargs): self.lib = original(*args, **kwargs)
                def __getattr__(self, name):
                    if name == missing: raise AttributeError(name)
                    return getattr(self.lib, name)
            with patch.object(ctypes, 'CDLL', Partial), c.App() as app:
                child = c.item('child')
                view = c.virtual_list('ordinary').children(child)
                app.mount(c.column('root').children(view)); app.step()
                self.assertFalse(app.supports_virtual_list_rows())
                with self.assertRaises(RuntimeError): view.set_virtual_list_rows([(child, 32)])

    def test_composed_items_and_bounded_projection(self):
        from itertools import repeat
        with c.App() as app:
            if not app.supports_virtual_list_rows(): return
            child = c.label('Composed content', 'content')
            entry = c.item('entry').children(child)
            view = c.virtual_list('view').children(entry)
            app.mount(c.column('root').children(view))
            view.set_virtual_list_rows([(entry, 0)])
            with self.assertRaises(ValueError): view.set_virtual_list_rows(repeat((entry, 0)))
            view.remove_virtual_list_item(entry)
            self.assertTrue(child._disposed)
            self.assertNotIn((child._handle.slot, child._handle.generation), app._by_handle)
            with self.assertRaises(RuntimeError): child.label('retired')
            view.set_virtual_list_rows([]); app.step()

    def test_projection_and_callbacks(self):
        with c.App(max_nodes=90) as app:
            items = [c.item(str(i), 'Message '+str(i)) for i in range(12)]
            view = c.virtual_list('inbox').visual_size(240, 96).children(*items)
            prepend = c.button('prepend').label('New message')
            resize = c.button('resize').label('Expand message')
            app.mount(c.column('root').children(prepend, resize, view))
            if os.environ.get('CUI_EXPECT_VIRTUAL_LEGACY') == '1':
                self.assertFalse(app.supports_virtual_list_rows())
                app.step()
                with self.assertRaises(RuntimeError): view.preserve_virtual_list_anchor()
                with self.assertRaises(RuntimeError): view.set_virtual_list_rows([(i, 0) for i in items])
                return
            self.assertTrue(app.supports_virtual_list_rows())
            view.preserve_virtual_list_anchor().set_virtual_list_rows([(i, 0) for i in items])
            app.step()
            view.scroll_to(65); app.step()
            before = view.item_bounds(items[2])
            handle = (items[2]._handle.slot, items[2]._handle.generation)
            added = c.item('new', 'New message')
            rows = [(i, 32) for i in items]
            def on_prepend(event):
                view.append_virtual_list_item(added)
                rows.insert(0, (added, 48))
                view.set_virtual_list_rows(rows)
            prepend.on_click(on_prepend)
            def on_resize(event):
                rows[0] = (added, 80)
                view.set_virtual_list_rows(rows)
            resize.on_click(on_resize)
            def click(node):
                x,y,w,h=app.bounds(node)
                app.queue_pointer('down',x+w/2,y+h/2)
                app.queue_pointer('up',x+w/2,y+h/2)
                app.step(); app.dispatch(); app.step()
            click(prepend)
            self.assertEqual(handle, (items[2]._handle.slot, items[2]._handle.generation))
            self.assertAlmostEqual(view.item_bounds(items[2])[1], before[1], delta=.1)
            click(resize)
            self.assertAlmostEqual(view.item_bounds(items[2])[1], before[1], delta=.1)
            self.assertAlmostEqual(view.scroll_offset(), 145, delta=.1)
            old_order=list(view._children)
            for bad in [-1, .5, 4097, math.nan, math.inf, True]:
                with self.assertRaises(ValueError): view.set_virtual_list_rows([(added,bad)]+rows[1:])
                self.assertEqual(view._children,old_order)
            with self.assertRaises(ValueError): view.set_virtual_list_rows(rows[:-1])
            with self.assertRaises(ValueError): view.set_virtual_list_rows([rows[0]]*len(rows))
            with self.assertRaises(ValueError): view.append_virtual_list_item(items[0])
            with self.assertRaises(ValueError): view.append_virtual_list_item(c.item('new'))
            view.remove_virtual_list_item(added)
            with self.assertRaises(ValueError): view.set_virtual_list_rows(rows)
            with self.assertRaises(RuntimeError): added.label('stale')
            view.set_virtual_list_rows(rows[1:]); app.step()
            view.preserve_virtual_list_anchor(False)
            with c.App() as foreign:
                other=c.item('foreign'); foreign.mount(c.column('other').children(c.virtual_list('foreign-list').children(other)))
                with self.assertRaises(ValueError): view.set_virtual_list_rows([(other,0)]+rows[2:])
            for i in list(items): view.remove_virtual_list_item(i)
            view.set_virtual_list_rows([]); app.step()
            full = [c.item(f'capacity-{i}') for i in range(64)]
            for child in full: view.append_virtual_list_item(child)
            view.set_virtual_list_rows([(child, 1 if i % 2 else 4096) for i, child in enumerate(full)])
            with self.assertRaises(ValueError): view.append_virtual_list_item(c.item('overflow'))
            app.step()
        with self.assertRaises((ValueError,RuntimeError)): view.set_virtual_list_rows([])


if __name__ == '__main__': unittest.main()
