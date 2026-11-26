"""Retained Attachment actions: real keyboard input and optional wire boundaries."""
import os
import ctypes as C
from dataclasses import FrozenInstanceError
from types import SimpleNamespace
import unittest
import cui_bridge as c


def activate(app):
    app.queue_key('down', 'enter'); app.queue_key('up', 'enter'); app.step()


class AttachmentActions(unittest.TestCase):
    @unittest.skipIf(os.getenv("CUI_TEST_LEGACY_ATTACHMENT_ACTIONS") == "1", "legacy runtime")
    def test_real_input_shared_queue_and_reset(self):
        with c.App() as app:
            card = c.attachment('file', 'Plan.pdf').detail('Failed').attachment_status('failed')
            card.attachment_actions(open=True, remove=True, retry=True)
            app.mount(c.column().children(card)); app.step(); app.step()
            app.queue_key('down', 'tab'); app.queue_key('up', 'tab'); app.step()
            activate(app)
            event = next(app.poll_component_notifications_v1())
            self.assertEqual((event.kind, event.action, event.part, event.source),
                             ('attachment-action', 'open', 'card', 'keyboard'))
            self.assertEqual(event.event_index, 0)
            self.assertEqual(event.node, card)
            self.assertEqual(event.handle, (card._handle.slot, card._handle.generation))
            with self.assertRaises(FrozenInstanceError): event.action = 'remove'
            activate(app)
            old = next(app.poll_value_notifications_v1())
            self.assertIsInstance(old, c.LegacyNotificationV1)
            self.assertEqual(old.legacy.value, 1)
            activate(app); card.attachment_actions()
            self.assertEqual(next(app.poll_component_notifications_v1()).action, 'open')
            activate(app); self.assertEqual(list(app.poll_component_notifications_v1()), [])
        self.assertEqual(event.action, 'open')

    def test_runtime_capability(self):
        if os.getenv("CUI_TEST_LEGACY_ATTACHMENT_ACTIONS") != "1": return
        with c.App() as app:
            self.assertFalse(app.supports_attachment_actions())
            card = c.attachment('legacy', 'Legacy file')
            card.attachment_actions(open=True).attachment_actions()
            app.mount(c.column().children(card)); app.step()
            with self.assertRaises(RuntimeError): card.attachment_actions(open=True)
            with self.assertRaises(c.BridgeError): next(app.poll_component_notifications_v1())
            self.assertEqual(list(app.poll_value_notifications_v1()), [])

    def test_no_drain_and_mount_preflight(self):
        with c.App() as app:
            original = app._api.lib
            calls = []
            def poll(*args): calls.append(args); return 1
            def size(): return C.sizeof(c._NotificationV1)
            def versions(): return 1
            def categories(): return 256
            symbols = dict(cui_bridge_notification_v1_size=size,
                           cui_bridge_notification_versions=versions,
                           cui_bridge_notification_value_payloads_v1=categories,
                           cui_bridge_poll_value_notification_v1=poll,
                           cui_bridge_attachment_set_actions_v1=lambda *args: 0)
            try:
                for missing in symbols:
                    app._api.lib = SimpleNamespace(**{k:v for k,v in symbols.items() if k != missing})
                    self.assertFalse(app.supports_attachment_actions())
                    with self.assertRaises(c.BridgeError): next(app.poll_component_notifications_v1())
                    card = c.attachment('new', 'New').attachment_actions(open=True)
                    with self.assertRaises(RuntimeError): app.mount(c.column().children(card))
                    self.assertIsNone(card._handle)
                for name, result in (("notification_v1_size", 0), ("notification_versions", 0),
                                     ("notification_value_payloads_v1", 28 | (1 << 31))):
                    invalid = dict(symbols)
                    invalid["cui_bridge_" + name] = lambda result=result: result
                    app._api.lib = SimpleNamespace(**invalid)
                    self.assertFalse(app.supports_attachment_actions())
                    with self.assertRaises(c.BridgeError): next(app.poll_component_notifications_v1())
                self.assertEqual(calls, [])
            finally: app._api.lib = original

    def test_owned_protocol_and_malformed_evidence(self):
        with c.App() as app:
            original = app._api.lib
            for malformed in (None, 'index-only', 'source-only', 'flags', 'action', 'part', 'source', 'index', 'node', 'frame', 'text', 'legacy', 'checked', 'selected'):
                calls = []
                def poll(_a, _s, mask, out, buffer, capacity, required):
                    calls.append(mask)
                    value = C.cast(out, C.POINTER(c._NotificationV1)).contents
                    value.struct_size = C.sizeof(value); value.schema_version = 1
                    value.kind = value.legacy.type = 1; value.payload_kind = 8; value.node_kind = 52
                    value.node.slot = value.legacy.node.slot = 0xffffffff
                    value.node.generation = value.legacy.node.generation = 0xfffffffe
                    value.flags = 1; value.frame_sequence = value.legacy.frame_sequence = 0xffffffffffffffff
                    value.legacy.value = 4
                    a = value.payload.attachment_action
                    a.action = 4; a.part = 3; a.source = 2; a.flags = 6; a.event_index = 0xffffffff
                    if malformed == 'index-only': a.flags = 2; a.source = 0
                    if malformed == 'source-only': a.flags = 4; a.event_index = 0
                    if malformed == 'flags': a.flags = 8
                    if malformed == 'action': a.action = 3
                    if malformed == 'part': a.part = 2
                    if malformed == 'source': a.source = 1
                    if malformed == 'index': a.flags = 4
                    if malformed == 'node': value.node_kind = 4
                    if malformed == 'frame': value.flags = 0
                    if malformed == 'text': value.text_length = value.legacy.text_length = 1
                    if malformed == 'legacy': value.legacy.value = 1
                    if malformed == 'checked': value.legacy.checked = 1
                    if malformed == 'selected': value.legacy.selected.slot = 1
                    C.cast(required, C.POINTER(C.c_uint32))[0] = value.text_length + 1
                    return 0
                app._api.lib = SimpleNamespace(cui_bridge_notification_v1_size=lambda:C.sizeof(c._NotificationV1),
                    cui_bridge_notification_versions=lambda:1,
                    cui_bridge_notification_value_payloads_v1=lambda:256 | 28 | (1 << 31),
                    cui_bridge_attachment_set_actions_v1=lambda *args:0,
                    cui_bridge_poll_value_notification_v1=poll)
                try:
                    with self.subTest(malformed=malformed):
                        if malformed not in (None, 'index-only', 'source-only'):
                            with self.assertRaises(c.BridgeError): next(app.poll_component_notifications_v1())
                        else:
                            event = next(app.poll_component_notifications_v1())
                            self.assertEqual(event.frame_sequence, 0xffffffffffffffff)
                            self.assertEqual(event.event_index, None if malformed == "source-only" else 0xffffffff)
                            self.assertEqual(event.handle, (0xffffffff,0xfffffffe))
                        self.assertEqual(calls, [284])
                finally: app._api.lib = original

if __name__ == '__main__': unittest.main()
