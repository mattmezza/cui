"""Headless opt-in Spinner V1 runtime and real-provider compatibility."""
import os
import unittest
from dataclasses import FrozenInstanceError, replace
from unittest.mock import patch
from cui_bridge import (App, SpinnerConfig, SpinnerSize, SpinnerEasing, column,
                        rotating_spinner, spinner, collapsible, BridgeError, IconSource)

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

    def test_shared_svg_resource(self):
        node = rotating_spinner('svg')
        shared = rotating_spinner('shared')
        with self.assertRaises(RuntimeError): node.set_icon_resource(None)
        with App() as app, App() as other:
            app.mount(column('root').children(node, shared)); app.step(.1)
            if os.environ.get('CUI_EXPECT_SPINNER_ICON_UNSUPPORTED') == '1':
                self.assertFalse(app.supports_spinner_icon_resource())
                with self.assertRaisesRegex(RuntimeError, 'compatible bridge symbol group'):
                    node.set_icon_resource(None)
                self.assertIsNotNone(node.presentation())
                return
            self.assertTrue(app.supports_spinner_icon_resource())
            source = IconSource.svg('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><path d="M0 0h16v16H0z"/></svg>')
            resource = app.icon_resource(source)
            foreign = other.icon_resource(source)
            before = node.presentation()
            with self.assertRaises(ValueError): node.set_icon_resource(foreign)
            with self.assertRaises(TypeError): node.set_icon_resource(object())
            self.assertEqual(node.presentation(), before)
            self.assertIs(node.set_icon_resource(resource), node)
            shared.set_icon_resource(resource)
            self.assertIsNone(node.presentation())
            app.step(.1)
            self.assertIsNotNone(node.presentation())
            self.assertGreater(resource.bitmap().width, 0)
            with self.assertRaises(BridgeError): resource.close()
            app.step(.1)
            self.assertIsNotNone(node.presentation())
            self.assertIs(node.set_icon_resource(None), node)
            with self.assertRaises(BridgeError): resource.close()
            shared.set_icon_resource(None)
            resource.close()
            with self.assertRaises(RuntimeError): node.set_icon_resource(resource)
            self.assertIsNone(node.presentation())
            app.step(.1)
            self.assertIsNotNone(node.presentation())

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

    def test_icon_resource_wrong_version(self):
        with App() as app:
            if app._api.spinner_icon_resource is None:
                self.skipTest('optional group absent in provider')
            node = rotating_spinner('version')
            app.mount(column('root').children(node))
            with patch.object(app._api.spinner_icon_resource, 'icon_resource_versions', return_value=0):
                self.assertFalse(app.supports_spinner_icon_resource())
                with self.assertRaisesRegex(RuntimeError, 'compatible bridge symbol group'):
                    node.set_icon_resource(None)
            app.step()
            self.assertIsNotNone(node.presentation())

    def test_wrong_abi(self):
        for name,value in (('versions',0),('config_size_v1',40),('presentation_size_v1',32)):
            with App() as app, patch.object(app._api.spinner_presentation,name,return_value=value):
                self.check_unsupported(app)

if __name__=='__main__': unittest.main()
