"""Focused typed shadow values through the real portable bridge."""
import ctypes as C
import math
import unittest
from unittest.mock import patch
from cui_bridge import App, BoxShadow, ShadowStyle, ShadowKind, ShadowPreset, CornerRadii, button, column, checkbox, _API
from cui_bridge.shadows import _Layer, _Style

class Shadows(unittest.TestCase):
    def test_owned_live_callback_clear_reset(self):
        layer=BoxShadow.css(0,2,8,-1,rgba=0x10203040)
        self.assertEqual(layer.blur_radius,8)
        self.assertEqual(C.sizeof(_Style),224)
        self.assertEqual(C.sizeof(_Layer),24)
        self.assertEqual(_Style.layers.offset,32)
        self.assertEqual(_Layer.rgba.offset,20)
        layers=[layer,BoxShadow(kind=ShadowKind.INSET)]
        node=button('target').label('Go').box_shadows(layers,CornerRadii(3,4,5,6))
        layers.clear()
        self.assertEqual(len(node._shadow_style.layers),2)
        with App(width=360,height=240) as app:
            app.mount(column().children(node))
            app.step()
            self.assertEqual(node._shadow_style._wire(app._api).layers[0].sigma,4)
            before=app.bounds(node)
            calls=[]
            def clicked(event):
                calls.append(event)
                node.box_shadows([])
            node.on_click(clicked)
            x,y,w,h=before
            app.queue_pointer('down',x+w/2,y+h/2)
            app.queue_pointer('up',x+w/2,y+h/2)
            app.step()
            self.assertEqual(len(calls),0)
            self.assertEqual(app.dispatch(),1)
            self.assertEqual(len(calls),1)
            self.assertEqual(node._shadow_style.layers,())
            node.shadow_preset(ShadowPreset.MD,CornerRadii(4,4,4,4))
            old=node._shadow_style
            with self.assertRaises(ValueError):node.box_shadows([layer]*9)
            self.assertIs(node._shadow_style,old)
            app.step()
            self.assertEqual(app.bounds(node),before)
            node.reset_box_shadows()
            self.assertIsNone(node._shadow_style)
            app.step()
        with self.assertRaises(RuntimeError):node.box_shadows([])
        with self.assertRaises(ValueError):checkbox('no').box_shadows([])
        with self.assertRaises(ValueError):BoxShadow(sigma=math.inf)
        with self.assertRaises(ValueError):BoxShadow(offset_x=1e300)
        with self.assertRaises(ValueError):BoxShadow.css(blur_radius=513)

    def test_optional_symbols_preserve_default_old_load(self):
        real=_API(None)
        names={'box_shadows_v1_size','set_box_shadows_v1','reset_box_shadows','box_shadow_from_css_v1','shadow_preset_v1'}
        class Older:
            def __getattr__(self,name):
                if name.removeprefix('cui_bridge_') in names:raise AttributeError(name)
                return getattr(real.lib,name)
        with patch('cui_bridge.C.CDLL',return_value=Older()):
            old=_API('older-proxy')
            self.assertEqual(old.lib.cui_bridge_abi_version(),1)
            with self.assertRaisesRegex(RuntimeError,'newer CUI library'):old.shadow_css()
            pending=button('pending').shadow_preset(ShadowPreset.MD,CornerRadii(1,2,3,4))
            with App(library='older-proxy') as app:
                with self.assertRaisesRegex(RuntimeError,'newer CUI library'):
                    app.mount(column().children(pending))
                self.assertIsNone(pending._app)
                self.assertIsNone(pending._handle)
                plain=button('plain').label('Old defaults')
                app.mount(column().children(plain));app.step()
                with self.assertRaisesRegex(RuntimeError,'newer CUI library'):
                    plain.shadow_preset(ShadowPreset.MD)
                self.assertIsNone(plain._shadow_style)
                app.step()

    def test_wrong_kind_mount_rolls_back_and_app_provider_is_used(self):
        bad=button('bad').box_shadows([])
        bad.kind='checkbox'  # Exercise C validation/rollback despite mutated authoring data.
        with App() as app:
            with self.assertRaisesRegex(Exception,'invalid argument'):
                app.mount(column().children(bad))
            self.assertIsNone(bad._app)
            self.assertIsNone(bad._handle)
            pending=button('valid').box_shadows([BoxShadow.css(blur_radius=8)],CornerRadii(1,2,3,4))
            calls=[]
            original=app._api.shadow_css
            def owning_factory(*args):
                calls.append(app._api)
                return original(*args)
            app._api.shadow_css=owning_factory
            app.mount(column().children(pending));app.step()
            self.assertEqual(calls,[app._api])
            wire=pending._shadow_style._wire(app._api)
            self.assertEqual((wire.top_left,wire.top_right,wire.bottom_left,wire.bottom_right),(1,2,3,4))


if __name__=='__main__':unittest.main()
