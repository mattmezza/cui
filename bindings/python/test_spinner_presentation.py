"""Headless opt-in Spinner V1 runtime and real-provider compatibility."""
import os
import unittest
from dataclasses import FrozenInstanceError, replace
from unittest.mock import patch
from cui_bridge import (App, SpinnerConfig, SpinnerSize, SpinnerEasing, column,
                        rotating_spinner, spinner, collapsible, BridgeError)

class SpinnerTests(unittest.TestCase):
    def test_runtime(self):
        node = rotating_spinner('loading')
        with App() as app:
            self.assertTrue(app.supports_spinner_presentation())
            app.mount(column('root').children(node))
            self.assertEqual(node.get_config(), SpinnerConfig())
            self.assertIsNone(node.presentation())
            app.step(.1)
            first = node.presentation()
            self.assertEqual(first.phase, 0)
            self.assertEqual(app.bounds(node)[2:], (16, 16))
            self.assertEqual(first, node.presentation())
            with self.assertRaises(FrozenInstanceError): first.phase = 1
            node.configure(replace(node.get_config(), size=SpinnerSize.CUSTOM, pixels=24,
                                   rgba=0, easing=SpinnerEasing.LINEAR))
            self.assertIsNone(node.presentation())
            app.step(.2)
            self.assertGreater(node.presentation().phase, 0)
            self.assertEqual(app.bounds(node)[2:], (24, 24))
            node.configure(SpinnerConfig(reduced_motion=True))
            app.step(.1)
            self.assertEqual(node.presentation().next_wake_seconds, 0)
            self.assertEqual(node.presentation().phase, 0)
            node.remove()
            with self.assertRaises(RuntimeError): node.get_config()

    def test_validation_atomicity(self):
        for fields in ({'pixels':1}, {'size':SpinnerSize.CUSTOM}, {'rem_size':float('nan')},
                       {'duration':1e-100}, {'duration':61}, {'rgba':True}, {'icon':'x'*64},
                       {'icon':'x\0y'}, {'size':SpinnerSize.LARGE,'rem_size':4096}):
            with self.subTest(fields=fields), self.assertRaises((ValueError, TypeError)):
                SpinnerConfig(**fields)
        for fields in ({'size':3}, {'easing':0}, {'reduced_motion':1}):
            with self.assertRaises(TypeError): SpinnerConfig(**fields)
        node=rotating_spinner('loading')
        with App() as app:
            app.mount(column('root').children(node)); app.step()
            before=node.presentation()
            with self.assertRaises(BridgeError): node.configure(SpinnerConfig(icon='not-an-embedded-icon'))
            self.assertEqual(node.get_config(), SpinnerConfig())
            self.assertEqual(node.presentation(), before)
            with self.assertRaises(TypeError): node.visual_size(10,10)

    def test_omission_and_mount_rollback(self):
        node = rotating_spinner('loading')
        panel = collapsible('panel', 'Sync').open().children(node)
        with App() as app:
            with patch.object(app._api.spinner_presentation, 'configure_v1', return_value=6):
                with self.assertRaises(BridgeError): app.mount(column('failed').children(node))
            self.assertIsNone(node._handle)
            app.mount(column('root').children(panel)); app.step(.1)
            self.assertIsNotNone(node.presentation())
            panel.open(False); app.step(.1)
            self.assertIsNone(node.presentation())
            panel.open(True); app.step(.1)
            self.assertEqual(node.presentation().phase, 0)

    def check_unsupported(self, app):
        node=rotating_spinner('modern'); root=column('root').children(node)
        self.assertFalse(app.supports_spinner_presentation())
        with self.assertRaisesRegex(RuntimeError, 'compatible bridge symbol group'): app.mount(root)
        self.assertIsNone(root._handle)
        legacy=spinner('legacy').visual_size(22,22)
        app.mount(column('old').children(legacy)); app.step()
        self.assertEqual(app.bounds(legacy)[2:], (22,22))

    def test_real_providers(self):
        for name in ('CUI_SPINNER_LEGACY_LIBRARY','CUI_SPINNER_PARTIAL_LIBRARY'):
            library=os.environ.get(name)
            if not library: self.skipTest(name+' not configured')
            with self.subTest(provider=name), App(library=library) as app: self.check_unsupported(app)

    def test_wrong_abi(self):
        for name,value in (('versions',0),('config_size_v1',40),('presentation_size_v1',32)):
            with App() as app, patch.object(app._api.spinner_presentation,name,return_value=value):
                self.check_unsupported(app)

if __name__=='__main__': unittest.main()
