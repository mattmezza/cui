"""Retained Attachment slot composition and optional provider coverage."""
import os
import unittest

from cui_bridge import App, attachment, attachment_slot, button, column, label, notification


def click(app, widget):
    x, y, w, h = app.bounds(widget)
    app.queue_pointer("down", x + w / 2, y + h / 2)
    app.queue_pointer("up", x + w / 2, y + h / 2)
    app.step()
    return app.dispatch()


class AttachmentSlots(unittest.TestCase):
    def test_roles_validate_before_mount(self):
        for width in (0, -1, 4097, float("nan"), float("inf"), 1e-300):
            with self.subTest(width=width), self.assertRaises(ValueError):
                attachment_slot("action", "actions", width)
        with self.assertRaises(TypeError):
            attachment_slot("action", "actions", True)
        with self.assertRaises(ValueError):
            attachment_slot("", "metadata")
        with self.assertRaises(ValueError):
            attachment_slot("content", "unknown")
        with self.assertRaises(ValueError):
            attachment_slot("preview", "media", 100)
        with self.assertRaises(ValueError):
            attachment("file").children(column("raw"))
        with self.assertRaises(ValueError):
            column().children(attachment_slot("orphan", "metadata"))
        card = attachment("file").children(attachment_slot("first", "metadata"))
        with self.assertRaises(ValueError):
            card.children(attachment_slot("second", "metadata"))
        self.assertEqual(len(card._children), 1)
        with App() as app, self.assertRaisesRegex(ValueError, "direct Attachment"):
            app.mount(attachment_slot("root", "media"))

    def test_preflight_leaves_app_reusable_without_factory(self):
        with App() as app:
            if os.getenv("CUI_TEST_LEGACY_ATTACHMENT_SLOTS") == "1":
                self.assertFalse(app.supports_attachment_slots())
            else:
                self.assertTrue(app.supports_attachment_slots())
                app._api.create_attachment_slot = None
            root = column()
            action = button("inspect").label("Inspect")
            card = attachment("report", "Report.pdf").children(
                attachment_slot("actions", "actions", 100).children(action))
            with self.assertRaisesRegex(RuntimeError, "slots unavailable"):
                app.mount(root.children(card))
            self.assertIsNone(root._handle)
            self.assertIsNone(action._handle)
            app.mount(column().children(attachment("old-report", "Report.pdf")))
            app.step()

    @unittest.skipIf(os.getenv("CUI_TEST_LEGACY_ATTACHMENT_SLOTS") == "1", "legacy provider")
    def test_child_dispatch_and_inert_slot(self):
        calls = []
        inspect = button("inspect").label("Inspect").on_click(lambda event: calls.append(event.node))
        media = attachment_slot("media", "media").children(label("PDF"))
        overlay = attachment_slot("overlay", "overlay").children(label("1"))
        metadata = attachment_slot("metadata", "metadata").gap(2).children(label("1.4 MB"))
        actions = attachment_slot("actions", "actions", 102).gap(4).disabled().children(inspect)
        card = (attachment("file", "Report.pdf").visual_size(340, 0)
                .attachment_status("complete").children(media, overlay, metadata, actions))
        with App() as app:
            app.mount(column().children(card))
            app.step()
            self.assertEqual(app.bounds(media), app.bounds(overlay))
            self.assertEqual(app.bounds(actions)[2], 102)
            self.assertEqual(click(app, inspect), 0)
            actions.disabled(False)
            app.step()
            self.assertEqual(click(app, inspect), 1)
            self.assertEqual(calls, [inspect])
            self.assertEqual(app.dispatch(), 0)
        with self.assertRaises(RuntimeError):
            actions.disabled()

    @unittest.skipIf(os.getenv("CUI_TEST_LEGACY_ATTACHMENT_SLOTS") == "1", "legacy provider")
    def test_app_wide_notification_stays_outside_slots(self):
        invalid = attachment("file").children(attachment_slot("meta", "metadata").children(
            column().children(notification("notice", "Global notice"))))
        with App() as app, self.assertRaisesRegex(ValueError, "app-wide stack"):
            app.mount(column().children(invalid))


if __name__ == "__main__":
    unittest.main()
