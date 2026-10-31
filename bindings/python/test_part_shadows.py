"""Semantic part authoring against a real portable bridge; no native windows."""
import unittest
from unittest.mock import patch
import ctypes as C
from cui_bridge import _API
from cui_bridge import App, BoxShadow, ShadowPart as P, ShadowPreset, CornerRadii, checkbox, radio, switch as switch_control, slider, column, radio_group, item, tabs, button

class PartShadows(unittest.TestCase):
    def test_owned_aliases_geometry_and_capability(self):
        layers=[BoxShadow.css(blur_radius=8)]
        check=checkbox('check').label('Check').part_shadows(P.MAIN,layers)
        layers.clear()
        switch=switch_control('switch').label('Switch').part_shadows(P.TRACK,[BoxShadow(sigma=1)]).part_shadows(P.THUMB,[])
        slide=slider('slider').part_shadows(P.MAIN,[])
        with App() as app:
            app.mount(column().children(check,switch,slide));app.step()
            self.assertEqual(check.shadow_target(P.MAIN).canonical_part,P.INDICATOR)
            self.assertFalse(slide.shadow_target(P.THUMB).supported)
            self.assertEqual(len(check._part_shadow_styles[P.INDICATOR].layers),1)
            before=app.bounds(check)
            calls=[]
            check.on_change(lambda event:(calls.append(event),check.part_shadows(P.INDICATOR,[])))
            x,y,w,h=before
            app.queue_pointer('down',x+w/2,y+h/2);app.queue_pointer('up',x+w/2,y+h/2)
            app.step();self.assertEqual(len(calls),0);app.dispatch();self.assertEqual(len(calls),1)
            check.part_shadow_preset(P.ROOT,ShadowPreset.SM,CornerRadii(2,3,4,5))
            check.part_shadows(P.INDICATOR,[])
            self.assertEqual(check._part_shadow_styles[P.INDICATOR].layers,())
            old=dict(check._part_shadow_styles)
            with self.assertRaises(ValueError):check.part_shadows(P.THUMB,[])
            self.assertEqual(check._part_shadow_styles,old)
            check.reset_part_shadows(P.MAIN);app.step()
            self.assertNotIn(P.INDICATOR,check._part_shadow_styles)
            self.assertEqual(app.bounds(check),before)
            plain=button('not-mounted')
            with self.assertRaises(RuntimeError):plain.shadow_target(P.ROOT)
        with self.assertRaises(RuntimeError):check.part_shadows(P.MAIN,[])

    def test_original_root_and_new_main_share_one_override(self):
        node=button('same').box_shadows([BoxShadow(sigma=1)])
        node.part_shadows(P.MAIN,[])
        self.assertIsNone(node._shadow_style)
        self.assertIn(P.ROOT,node._part_shadow_styles)
        with App() as app:
            app.mount(column().children(node));app.step()
            self.assertEqual(node.shadow_target(P.MAIN).canonical_part,P.ROOT)
            node.box_shadows([BoxShadow(sigma=2)])
            self.assertNotIn(P.ROOT,node._part_shadow_styles)
            node.reset_part_shadows(P.MAIN)
            self.assertIsNone(node._shadow_style)
            app.step()

    def test_group_items_and_wrong_owner_mount_rollback(self):
        a=item('a').label('A').part_shadows(P.MAIN,[])
        b=item('b').label('B').part_shadows(P.ROOT,[BoxShadow(sigma=1)])
        with App() as app:
            app.mount(column().children(radio_group('g').children(a,b)));app.step()
            self.assertTrue(a.shadow_target(P.INDICATOR).supported)
            a.reset_part_shadows(P.MAIN);app.step()
        wrong=item('wrong').part_shadows(P.INDICATOR,[])
        with App() as app:
            with self.assertRaisesRegex(Exception,'invalid argument'):
                app.mount(column().children(tabs('tabs').children(wrong)))
            self.assertIsNone(wrong._app)
            self.assertIsNone(wrong._handle)

    def test_missing_symbols_preserve_default_load_and_local_state(self):
        real=_API(None)
        names={'cui_bridge_set_target_box_shadows_v1','cui_bridge_reset_target_box_shadows_v1','cui_bridge_shadow_target_query_v1'}
        class OldParts:
            def __getattr__(self,name):
                if name in names:raise AttributeError(name)
                return getattr(real.lib,name)
        with patch('cui_bridge.C.CDLL',return_value=OldParts()):
            with App(library='old-part-proxy') as app:
                check=checkbox('plain').label('Plain')
                app.mount(column().children(check));app.step()
                with self.assertRaisesRegex(RuntimeError,'matching newer CUI library'):
                    check.part_shadows(P.MAIN,[])
                self.assertEqual(check._part_shadow_styles,{})
                with self.assertRaisesRegex(RuntimeError,'matching newer CUI library'):
                    check.shadow_target(P.MAIN)
                app.step()
            pending=checkbox('pending').part_shadows(P.MAIN,[])
            with App(library='old-part-proxy') as app:
                with self.assertRaisesRegex(RuntimeError,'matching newer CUI library'):
                    app.mount(column().children(pending))
                self.assertIsNone(pending._app)
                self.assertIsNone(pending._handle)

if __name__=='__main__':unittest.main()
