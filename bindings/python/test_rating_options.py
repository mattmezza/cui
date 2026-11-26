"""Headless public retained Rating configuration and compatibility."""
import ctypes as C
import os
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch
from cui_bridge import App, BridgeError, IconSource, RatingConfig, column, rating
from cui_bridge.rating_options import bind_api

class RatingTests(unittest.TestCase):
    def test_validation(self):
        for options in ({'half_steps':1},{'width':float('nan')},{'width':.5},{'width':4097}, {'fill_rgba':-1},{'outline_icon':object()},{'design_size':'huge'}):
            with self.subTest(options=options), self.assertRaises((TypeError,ValueError)):
                RatingConfig(**options)
    def test_incomplete_probe(self):
        for missing in ('config_size_v1','configure_v1','get_config_v1'):
            lib=SimpleNamespace(**{'cui_bridge_rating_'+n:Mock() for n in ('config_size_v1','configure_v1','get_config_v1') if n!=missing})
            api=SimpleNamespace(lib=lib)
            bind_api(api,Mock())
            self.assertIsNone(api.rating_options)
    def test_legacy_or_current(self):
        with App() as app:
            if os.environ.get("CUI_RATING_UNSUPPORTED") == "1":
                self.assertFalse(app.supports_rating_options())
            else:
                self.assertTrue(app.supports_rating_options())
            if not app.supports_rating_options():
                node=rating('opt').configure(RatingConfig())
                with self.assertRaisesRegex(RuntimeError,'compatible bridge'):
                    app.mount(column().children(node))
                self.assertIsNone(node._handle)
                node=rating('legacy').total(5).value(2)
                app.mount(column().children(node));app.step()
                self.assertIsNone(node.get_config())
                self.assertEqual(node.current_value(),2)
                return
            blank=rating("blank").configure(RatingConfig())
            self.assertEqual(blank.get_config(),RatingConfig())
            changes=[]
            config=RatingConfig(half_steps=True,hover_preview=True,step_down_filled=True,clearable=True,design_size='lg',width=300,fill_rgba=0)
            node=rating('rating').total(5).value(1.5).configure(config).on_change(changes.append)
            app.mount(column().children(node));app.step()
            self.assertEqual(node.get_config(),config)
            self.assertEqual(node.current_value(),1.5)
            x,y,w,h=app.numeric_part_bounds(node,13)
            app.queue_pointer('move',x+w/4,y+h/2);app.step()
            self.assertEqual(node.current_value(),1.5)
            self.assertEqual(app.dispatch(),0)
            app.queue_pointer('down',x+w/4,y+h/2);app.queue_pointer('up',x+w/4,y+h/2);app.step()
            self.assertEqual(app.dispatch(),1)
            self.assertEqual([e.value for e in changes],[3.5])
            with patch.object(app._api.rating_options,'configure_v1',return_value=6):
                with self.assertRaises(BridgeError):node.configure(RatingConfig(readonly=True))
            self.assertEqual(node.get_config(),config)
            node.reset_config()
            self.assertIsNone(node.get_config())
    def test_bad_size_preflight(self):
        with App() as app:
            if app._api.rating_options is None:return
            with patch.object(app._api.rating_options,'config_size_v1',return_value=47):
                self.assertFalse(app.supports_rating_options())
                with self.assertRaisesRegex(RuntimeError,'compatible bridge'):
                    app.mount(column().children(rating('x').configure(RatingConfig())))
            self.assertIsNone(app._root)
    def test_mask_affinity_pin_release(self):
        with App() as app, App() as other:
            if not app.supports_rating_options():return
            icon=app.icon_resource(IconSource.named('heart'))
            foreign=other.icon_resource(IconSource.named('heart'))
            node=rating('mask').configure(RatingConfig(fill_icon=icon,outline_icon=icon))
            app.mount(column().children(node));app.step()
            self.assertIs(node.get_config().fill_icon,icon)
            with self.assertRaises((RuntimeError,ValueError,BridgeError)):icon.close()
            with self.assertRaises((RuntimeError,ValueError,BridgeError)):node.configure(RatingConfig(fill_icon=foreign))
            self.assertIs(node.get_config().fill_icon,icon)
            node.reset_config();icon.close();foreign.close()
            with self.assertRaises((RuntimeError,ValueError,BridgeError)):node.configure(RatingConfig(fill_icon=icon))

if __name__=='__main__':unittest.main()
