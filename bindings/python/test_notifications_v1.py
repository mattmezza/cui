"""Real queued V1 snapshots plus optional ABI/protocol failure fixtures."""
import ctypes as C
from dataclasses import FrozenInstanceError
from types import SimpleNamespace
import unittest
import cui_bridge as c


def click(app, node):
    x, y, w, h = app.bounds(node)
    app.queue_pointer('down', x + w / 2, y + h / 2)
    app.queue_pointer('up', x + w / 2, y + h / 2)
    app.step()


class NotificationsV1(unittest.TestCase):
    def test_real_queue_interleave_and_owning_views(self):
        with c.App() as app:
            checked = c.checkbox('check', 'Check')
            action = c.button('action').label('Action')
            callbacks = []
            checked.on_change(callbacks.append)
            app.mount(c.column().children(checked, action))
            app.step()
            click(app, checked)
            event = next(app.poll_notifications_v1())
            self.assertIsInstance(event, c.BooleanChangeNotificationV1)
            self.assertTrue(event.value)
            self.assertEqual(event.legacy.checked_raw, 1)
            self.assertEqual(callbacks, [])
            self.assertTrue(event.has_frame)
            with self.assertRaises(FrozenInstanceError):
                event.value = False
            click(app, checked)
            old = list(app.poll_events())
            self.assertFalse(old[0].checked)
            self.assertEqual(list(app.poll_notifications_v1()), [])
            click(app, action)
            activation = next(app.poll_notifications_v1())
            self.assertIsInstance(activation, c.ActivationNotificationV1)
            self.assertTrue(event.value)
            self.assertEqual(activation.handle, (action._handle.slot, action._handle.generation))
        self.assertTrue(event.value)

    def test_legacy_text_owns_utf8_after_close(self):
        with c.App() as app:
            field = c.input('text', '')
            app.mount(c.column().children(field))
            app.step()
            click(app, field)
            list(app.poll_notifications_v1())
            app.queue_text('héllo ✓')
            app.step()
            event = next(app.poll_notifications_v1())
            self.assertIsInstance(event, c.LegacyNotificationV1)
            self.assertEqual(event.legacy.text, 'héllo ✓')
            app.queue_text('later')
            app.step()
        self.assertEqual(event.legacy.text, 'héllo ✓')

    def test_table_columns_preserved(self):
        with c.App(width=1200) as app:
            table = c.data_table('matrix').table_data(['A', 'B', 'C'], [['x', 'y', 'z']]).selection_mode('cell')
            app.mount(c.column().children(table))
            app.step()
            x, y, w, h = table.table_cell_bounds(0, 2)
            app.queue_pointer('down', x+w/2, y+h/2)
            app.queue_pointer('up', x+w/2, y+h/2)
            app.step()
            event = next(app.poll_notifications_v1())
            self.assertIsInstance(event, c.TableChangeNotificationV1)
            self.assertEqual((event.reason, event.row, event.column), ('selection', 1, 3))
            self.assertEqual(event.legacy.checked_raw, 3)
            self.assertIn('"column":3', event.legacy.text)

    def test_optional_symbols_size_and_capacity(self):
        with c.App() as app:
            real = app._api.lib
            app._api.lib = SimpleNamespace()
            with self.assertRaisesRegex(c.BridgeError, 'unavailable'):
                list(app.poll_notifications_v1())
            # Existing legacy poll does not resolve V1 symbols.
            self.assertEqual(list(app.poll_events()), [])
            def size(): return C.sizeof(c._NotificationV1)
            def versions(): return 1
            calls = []
            def poll(_app, _size, out, buffer, capacity, required):
                calls.append(capacity)
                C.cast(required, C.POINTER(C.c_uint32))[0] = 6000
                if capacity < 6000:
                    return 5
                value = C.cast(out, C.POINTER(c._NotificationV1)).contents
                value.struct_size = C.sizeof(value); value.schema_version = 1
                value.payload_kind = 7; value.text_length = 5999
                C.memset(buffer, ord('x'), 5999)
                return 0 if len(calls) == 2 else 1
            symbols = SimpleNamespace(cui_bridge_notification_v1_size=size,
                cui_bridge_notification_versions=versions, cui_bridge_poll_notification_v1=poll)
            app._api.lib = symbols
            iterator = app.poll_notifications_v1()
            event = next(iterator)
            iterator.close()
            self.assertEqual(calls, [4097, 6000])
            self.assertEqual(len(event.legacy.text), 5999)
            symbols.cui_bridge_notification_v1_size = lambda: 1
            with self.assertRaisesRegex(c.BridgeError, 'layout'):
                next(app.poll_notifications_v1())
            symbols.cui_bridge_notification_v1_size = size
            for required_size in (0, 4097, 1048577):
                def limited(_a, _s, _o, _b, _c, required):
                    C.cast(required, C.POINTER(C.c_uint32))[0] = required_size
                    return 5
                symbols.cui_bridge_poll_notification_v1 = limited
                with self.assertRaisesRegex(c.BridgeError, 'limit|retry'):
                    next(app.poll_notifications_v1())
            symbols.cui_bridge_notification_versions = lambda: 0
            with self.assertRaisesRegex(c.BridgeError, 'capability'):
                next(app.poll_notifications_v1())
            symbols.cui_bridge_notification_versions = versions
            retry_calls = []
            def repeated(_a, _s, _o, _b, capacity, required):
                retry_calls.append(capacity)
                C.cast(required, C.POINTER(C.c_uint32))[0] = capacity + 1
                return 5
            symbols.cui_bridge_poll_notification_v1 = repeated
            with self.assertRaisesRegex(c.BridgeError, 'retry'):
                next(app.poll_notifications_v1())
            self.assertEqual(retry_calls, [4097, 4098])
            app._api.lib = real


if __name__ == '__main__':
    unittest.main()
