"""Real queued owning values; synthetic protocol tests are labelled separately."""
import ctypes as C
from dataclasses import FrozenInstanceError
import os
from types import SimpleNamespace
import unittest
import cui_bridge as c


def click(app, node):
    x, y, w, h = app.bounds(node)
    app.queue_pointer('down', x + w / 2, y + h / 2)
    app.queue_pointer('up', x + w / 2, y + h / 2)
    app.step()


class ValueNotifications(unittest.TestCase):
    def test_real_scalar_text_selection_shared_queue(self):
        with c.App(width=800, height=800) as app:
            number = c.number_input('number').range(0, 100000000, .25).value(16777216.25)
            callbacks = []
            number.on_change(callbacks.append)
            app.mount(c.column().children(number)); app.step()
            click(app, number); app.queue_key('down', 'up'); app.step()
            scalar = next(app.poll_value_notifications_v1())
            self.assertIsInstance(scalar, c.ScalarChangeNotificationV1)
            self.assertEqual(scalar.value, 16777216.5)
            self.assertEqual(scalar.value, scalar.legacy.value)
            self.assertEqual(callbacks, [])
            with self.assertRaises(FrozenInstanceError): scalar.value = 0
        with c.App() as app:
            text = c.input('text', '')
            app.mount(c.column().children(text)); app.step()
            click(app, text); app.queue_text('héllo ✓'); app.step()
            # Old manual poll still exposes a complete legacy snapshot.
            old = next(app.poll_notifications_v1())
            self.assertIsInstance(old, c.LegacyNotificationV1)
            self.assertEqual(old.legacy.text, 'héllo ✓')
            app.queue_text('!'); app.step()
            typed_text = next(app.poll_value_notifications_v1())
            self.assertIsInstance(typed_text, c.TextChangeNotificationV1)
            self.assertEqual(typed_text.text, 'héllo ✓!')
            self.assertEqual(list(app.poll_value_notifications_v1()), [])
        self.assertEqual(typed_text.text, 'héllo ✓!')
        self.assertEqual(scalar.value, 16777216.5)

    def test_adjacent_number_and_text_fields(self):
        with c.App(width=800, height=800) as app:
            number = c.number_input('number').range(0, 100000000, .25).value(16777216.25)
            field = c.input('text', '')
            app.mount(c.column().children(number, field))
            app.step()
            click(app, number)
            app.queue_key('down', 'up'); app.step()
            value = next(app.poll_value_notifications_v1())
            self.assertIsInstance(value, c.ScalarChangeNotificationV1)
            self.assertEqual(value.value, 16777216.5)
            click(app, field)
            app.queue_text('ready'); app.step()
            text = next(app.poll_value_notifications_v1())
            self.assertIsInstance(text, c.TextChangeNotificationV1)
            self.assertEqual(text.text, 'ready')

    def test_real_selected_generation_reuse(self):
        with c.App(width=800, height=800) as app:
            first, second = c.item('first', 'First'), c.item('second', 'Second')
            group = c.radio_group('group').children(first, second).select(first)
            app.mount(c.column().children(group)); app.step(); click(app, second)
            live = next(app.poll_value_notifications_v1())
            self.assertIsInstance(live, c.SelectionChangeNotificationV1)
            self.assertIs(live.selected_node, second)
            with self.assertRaises(FrozenInstanceError): live.selected_handle = (0, 0)
            click(app, first)
            self.assertEqual(next(app.poll_events()).selected_handle, (first._handle.slot, first._handle.generation))
            click(app, second)
            historical = (second._handle.slot, second._handle.generation)
            # C fixture-style removal: Python has no public per-node remove API.
            app._api.check(app._api.node_remove(app._ptr, second._handle))
            app._by_handle.pop(historical)
            replacement = c._Handle()
            app._api.check(app._api.node_create(app._ptr, group._handle, 20, b'replacement', C.byref(replacement)))
            self.assertEqual(replacement.slot, historical[0])
            self.assertNotEqual(replacement.generation, historical[1])
            event = next(app.poll_value_notifications_v1())
            self.assertEqual(event.selected_handle, historical)
            self.assertIsNone(event.selected_node)

    def test_existing_boolean_and_table_views_on_value_poll(self):
        with c.App(width=1200, height=800) as app:
            check = c.checkbox('check', 'Check')
            table = c.data_table('matrix').table_data(['A', 'B', 'C'], [['x', 'y', 'z']]).selection_mode('cell')
            app.mount(c.column().children(check, table)); app.step(); click(app, check)
            self.assertTrue(next(app.poll_value_notifications_v1()).value)
            x, y, w, h = table.table_cell_bounds(0, 2)
            app.queue_pointer('down', x+w/2, y+h/2); app.queue_pointer('up', x+w/2, y+h/2); app.step()
            event = next(app.poll_value_notifications_v1())
            self.assertIsInstance(event, c.TableChangeNotificationV1)
            self.assertEqual((event.row, event.column), (1, 3))

    @unittest.skipUnless(os.environ.get('CUI_BRIDGE_OLD_LIBRARY'), 'requires explicit real old library')
    def test_actual_old_library_unavailable_without_drain(self):
        with c.App(library=os.environ['CUI_BRIDGE_OLD_LIBRARY']) as app:
            field = c.input('text', '')
            app.mount(c.column().children(field)); app.step(); click(app, field)
            app.queue_text('queued in old library'); app.step()
            self.assertFalse(hasattr(app._api.lib, 'cui_bridge_poll_value_notification_v1'))
            with self.assertRaisesRegex(c.BridgeError, 'unavailable'):
                next(app.poll_value_notifications_v1())
            self.assertEqual(next(app.poll_notifications_v1()).legacy.text, 'queued in old library')

    def test_synthetic_protocol_negotiation_and_limits(self):
        with c.App() as app:
            real = app._api.lib
            def size(): return C.sizeof(c._NotificationV1)
            def versions(): return 1
            def categories(): return (1 << 2) | (1 << 31)
            calls = []
            def poll(_a, _s, mask, out, buffer, capacity, required):
                calls.append((mask, capacity))
                if len(calls) > 1: return 1
                value = C.cast(out, C.POINTER(c._NotificationV1)).contents
                value.struct_size = C.sizeof(value); value.schema_version = 1
                value.kind = value.legacy.type = 2; value.payload_kind = 2
                C.cast(required, C.POINTER(C.c_uint32))[0] = 1
                value.flags = 1
                value.frame_sequence = value.legacy.frame_sequence = (1 << 64) - 1
                value.node.slot = value.legacy.node.slot = (1 << 32) - 1
                value.node.generation = value.legacy.node.generation = (1 << 32) - 1
                value.payload.scalar_value = value.legacy.value = 16777216.5
                return 0
            symbols = SimpleNamespace(cui_bridge_notification_v1_size=size,
                cui_bridge_notification_versions=versions,
                cui_bridge_notification_value_payloads_v1=categories,
                cui_bridge_poll_value_notification_v1=poll)
            app._api.lib = symbols
            try:
                event = next(app.poll_value_notifications_v1())
                self.assertEqual(calls, [(1 << 2, 4097)])
                self.assertEqual(event.frame_sequence, (1 << 64) - 1)
                self.assertEqual(event.handle, ((1 << 32) - 1, (1 << 32) - 1))
                symbols.cui_bridge_notification_value_payloads_v1 = lambda: 1 << 31
                with self.assertRaisesRegex(c.BridgeError, 'unavailable'): next(app.poll_value_notifications_v1())
                self.assertEqual(len(calls), 1)
                symbols.cui_bridge_notification_value_payloads_v1 = categories
                symbols.cui_bridge_notification_v1_size = lambda: 0
                with self.assertRaisesRegex(c.BridgeError, 'layout'): next(app.poll_value_notifications_v1())
                symbols.cui_bridge_notification_v1_size = size
                for required_size in (0, 4097, 1048577):
                    def limited(_a, _s, _m, _o, _b, _c, required):
                        C.cast(required, C.POINTER(C.c_uint32))[0] = required_size
                        return 5
                    symbols.cui_bridge_poll_value_notification_v1 = limited
                    with self.assertRaisesRegex(c.BridgeError, 'limit|retry'): next(app.poll_value_notifications_v1())
                capacities = []
                def retry(_a, _s, _m, out, buffer, capacity, required):
                    capacities.append(capacity)
                    C.cast(required, C.POINTER(C.c_uint32))[0] = 6000
                    if capacity < 6000: return 5
                    value = C.cast(out, C.POINTER(c._NotificationV1)).contents
                    value.struct_size = C.sizeof(value); value.schema_version = 1
                    value.kind = value.legacy.type = 2; value.payload_kind = 7
                    value.text_length = value.legacy.text_length = 5999
                    C.memset(buffer, ord('x'), 5999); return 0
                symbols.cui_bridge_poll_value_notification_v1 = retry
                iterator = app.poll_value_notifications_v1(); event = next(iterator); iterator.close()
                self.assertEqual(capacities, [4097, 6000]); self.assertEqual(len(event.legacy.text), 5999)
                def repeated(_a, _s, _m, _o, _b, capacity, required):
                    C.cast(required, C.POINTER(C.c_uint32))[0] = capacity + 1; return 5
                symbols.cui_bridge_poll_value_notification_v1 = repeated
                with self.assertRaisesRegex(c.BridgeError, 'retry'): next(app.poll_value_notifications_v1())
            finally: app._api.lib = real

    def test_synthetic_malformed_tags_and_values(self):
        with c.App() as app:
            real = app._api.lib
            try:
                for tag, kind, bad in [(99, 2, ''), (2, 1, ''), (2, 2, 'nan'), (4, 2, 'handle'),
                                       (1, 2, 'boolean'), (5, 2, 'table'), (6, 3, 'surface'),
                                       (3, 2, 'utf8'), (3, 2, 'nul'), (7, 2, 'flags')]:
                    def poll(_a, _s, _m, out, buffer, _capacity, _required):
                        value = C.cast(out, C.POINTER(c._NotificationV1)).contents
                        value.struct_size = C.sizeof(value); value.schema_version = 1
                        value.kind = value.legacy.type = kind; value.payload_kind = tag
                        if bad == 'nan': value.payload.scalar_value = value.legacy.value = float('nan')
                        if bad == 'handle': value.payload.selected.slot = 1
                        if bad == 'boolean': value.payload.boolean_value = 2
                        if bad == 'table': value.payload.table.reason = 3
                        if bad == 'surface': value.payload.surface_result = 0
                        if bad == 'flags': value.flags = 2
                        C.cast(_required, C.POINTER(C.c_uint32))[0] = 1
                        if bad in ('utf8', 'nul'):
                            C.cast(_required, C.POINTER(C.c_uint32))[0] = 2
                            value.text_length = value.legacy.text_length = 1
                            C.memmove(buffer, b'\xff\x00' if bad == 'utf8' else b'xx', 2)
                        return 0
                    app._api.lib = SimpleNamespace(cui_bridge_notification_v1_size=lambda: C.sizeof(c._NotificationV1),
                        cui_bridge_notification_versions=lambda: 1,
                        cui_bridge_notification_value_payloads_v1=lambda: 28,
                        cui_bridge_poll_value_notification_v1=poll)
                    with self.subTest(tag=tag, kind=kind, bad=bad):
                        with self.assertRaises(c.BridgeError): next(app.poll_value_notifications_v1())
            finally: app._api.lib = real


if __name__ == '__main__': unittest.main()
