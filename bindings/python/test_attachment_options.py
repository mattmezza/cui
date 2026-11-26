"""Headless copied Attachment presentation, preview ownership and fallback."""
import ctypes as C
import os
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch
from cui_bridge import App, AttachmentOptions, AttachmentTextStyle, BridgeError, attachment, column, label
from cui_bridge.attachment_options import bind_api, Config

class AttachmentTests(unittest.TestCase):
    def test_validation_and_copy(self):
        for kw in ({'vertical':1},{'media_aspect':float('nan')},{'media_aspect':.01},{'media_aspect':17},{'title_status':5},{'title_status':0},{'description_status':'done'},{'title_status':True},{'title':{}}):
            with self.subTest(kw=kw), self.assertRaises((TypeError,ValueError)):
                AttachmentOptions(**kw)
        for kw in ({'font_size':float('inf')},{'font_size':10**300},{'weight':True},{'weight':4},{'color_rgba':-1},{'italic':1},{'letter_spacing':float('nan')}):
            with self.subTest(kw=kw), self.assertRaises((TypeError,ValueError)):
                AttachmentTextStyle(**kw)
        node=attachment('copy','Document')
        pixels=bytearray([255,0,0,255]*4)
        node.attachment_preview_rgba(pixels,2,2)
        pixels[:]=b'\0'*16
        self.assertEqual(node._attachment_preview[0],bytes([255,0,0,255]*4))
        for args in ((b'123',1,1),(b'1234',True,1),(b'1234',1.0,1),(b'1234',1,1,3),(b'',2049,1),(object(),1,1)):
            with self.subTest(args=args), self.assertRaises((TypeError,ValueError)): node.attachment_preview_rgba(*args)
        node.clear_attachment_preview(); self.assertIsNone(node._attachment_preview)
        with self.assertRaises(ValueError): label('x').attachment_options(None)
        class MustNotCopy(bytearray):
            def __bytes__(self):
                raise AssertionError("rejected preview must not be copied")
        with self.assertRaises(ValueError):
            node.attachment_preview_rgba(MustNotCopy(8), 1, 1)
        self.assertEqual(C.sizeof(Config),80)
        with self.assertRaises(ValueError): attachment("bad", "two\nlines").attachment_options(AttachmentOptions())
        styled=attachment("styled", "Title").attachment_options(AttachmentOptions())
        with self.assertRaises(ValueError): styled.label("two\nlines")
        with self.assertRaises(ValueError): styled.detail("two\r lines")
        styled.attachment_options(None).label("two\nlines")

    def test_independent_optional_symbols(self):
        names=('attachment_configure_v1','attachment_set_preview_rgba_v1')
        for missing in names:
            api=SimpleNamespace(lib=SimpleNamespace(**{'cui_bridge_'+n:Mock() for n in names if n!=missing}))
            bind_api(api,lambda *a:Mock())
            self.assertIsNone(getattr(api,missing))
            self.assertIsNotNone(getattr(api,next(n for n in names if n!=missing)))
        with App() as app:
            if os.environ.get("CUI_TEST_LEGACY_ATTACHMENT") == "1":
                self.assertFalse(app.supports_attachment_options())
                self.assertFalse(app.supports_attachment_preview())
                configured=attachment("configured","Title").attachment_options(AttachmentOptions())
                with self.assertRaisesRegex(RuntimeError,"optional"):app.mount(column().children(configured))
                self.assertIsNone(configured._handle)
                preview=attachment("preview","Title").attachment_preview_rgba(bytes([255,0,0,255]),1,1)
                with self.assertRaisesRegex(RuntimeError,"optional"):app.mount(column().children(preview))
                self.assertIsNone(preview._handle)
                configured.attachment_options(None)
                app.mount(column().children(configured));app.step()
                self.assertGreater(app.bounds(configured)[2],0)
                with self.assertRaisesRegex(RuntimeError,"optional"):configured.attachment_options(AttachmentOptions())
                with self.assertRaisesRegex(RuntimeError,"optional"):configured.clear_attachment_preview()
                return
            with patch.object(app._api,'attachment_configure_v1',None):
                self.assertFalse(app.supports_attachment_options())
                node=attachment('unsupported','Title').attachment_options(AttachmentOptions())
                with self.assertRaisesRegex(RuntimeError,'optional'):app.mount(column().children(node))
                self.assertIsNone(node._handle)
                node.attachment_options(None)
                self.assertTrue(app.supports_attachment_preview())
                node.attachment_preview_rgba(bytes([255,0,0,255]),1,1)
                app.mount(column().children(node));app.step()
                with self.assertRaisesRegex(RuntimeError,'optional'):node.attachment_options(None)

    @unittest.skipIf(os.environ.get("CUI_TEST_LEGACY_ATTACHMENT") == "1", "legacy bridge lacks retained Attachment setters")
    def test_retained_geometry_reset_replacement(self):
        node=attachment('card','A very long document title').detail('Copied media').visual_size(240,0).design_size('lg')
        node.attachment_options(AttachmentOptions(vertical=True,media=True,media_aspect=2,title_status="complete",title=AttachmentTextStyle(font_size=20,color_rgba=0)))
        pixels=bytearray([255,0,0,255]*4)
        node.attachment_preview_rgba(pixels,2,2);pixels[:]=b'\0'*16
        with App() as app:
            self.assertTrue(app.supports_attachment_options());self.assertTrue(app.supports_attachment_preview())
            app.mount(column().children(node));app.step()
            self.assertIsNone(node._attachment_preview)
            vertical=app.bounds(node);self.assertAlmostEqual(vertical[2],240)
            node.attachment_options(AttachmentOptions(media=True)).attachment_preview_rgba(bytes([0,255,0,255]*4),2,2)
            self.assertIsNone(node._attachment_preview)
            app.step();horizontal=app.bounds(node);self.assertLess(horizontal[3],vertical[3])
            node.clear_attachment_preview().attachment_options(None);app.step()
            self.assertEqual(app.bounds(node)[2],240)
            saved_preview=node._attachment_preview
            with patch.object(app._api,"attachment_set_preview_rgba_v1",return_value=1):
                with self.assertRaises(BridgeError):node.attachment_preview_rgba(bytes([255,0,0,255]),1,1)
            self.assertIs(node._attachment_preview,saved_preview)
            before=node._attachment_options
            with patch.object(app._api,'attachment_configure_v1',return_value=1):
                with self.assertRaises(BridgeError):node.attachment_options(AttachmentOptions(vertical=True))
            self.assertIs(node._attachment_options,before)
            with self.assertRaises(BridgeError):node.attachment_options(AttachmentOptions(title=AttachmentTextStyle(letter_spacing=1)))
            self.assertIs(node._attachment_options,before)
        with self.assertRaises(RuntimeError):node.clear_attachment_preview()

    @unittest.skipIf(os.environ.get("CUI_TEST_LEGACY_ATTACHMENT") == "1", "legacy bridge lacks retained Attachment setters")
    def test_mount_rollback_retains_pending_pixels(self):
        pixels = bytes([255, 0, 0, 255])
        card = attachment("first", "Title").attachment_preview_rgba(pixels, 1, 1)
        later = attachment("later", "Title").attachment_options(AttachmentOptions(
            title=AttachmentTextStyle(letter_spacing=1)))
        tree = column().children(card, later)
        with App() as app:
            with self.assertRaises(BridgeError):
                app.mount(tree)
            self.assertIsNone(card._app)
            self.assertEqual(card._attachment_preview[0], pixels)
            later.attachment_options(None)
            app.mount(tree)
            self.assertIsNone(card._attachment_preview)
            replacement = bytearray([0, 255, 0, 255])
            card.attachment_preview_rgba(replacement, 1, 1)
            self.assertIsNone(card._attachment_preview)
            replacement[:] = b"\0" * 4
            app.step()

if __name__=='__main__':unittest.main()
