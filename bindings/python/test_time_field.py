"""Localized authored UIs; source checkpoint, runtime validation not yet executed."""
import ctypes as C
from dataclasses import FrozenInstanceError, replace
import gc
import importlib
import os
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import weakref

from cui_bridge import (App, BridgeError, column, row, label, time_field,
    localized_time_field, TimeValue, TimeLocale, TimeFieldConfig, TimeCycle,
    TimePrecision, TimeTextStatus, slider_model, SliderConfig, SliderSingle)

wire = importlib.import_module('cui_bridge.time_field')


class LocalizedTime(unittest.TestCase):
    def create(self, config=None, capacity=16):
        app = App(width=760, height=640)
        self.addCleanup(app.close)
        app.enable_time_changes(capacity)
        node = localized_time_field('time', config or TimeFieldConfig(), TimeValue(9, 45, 30), 'Appointment')
        app.mount(column('root').children(label('Schedule', key='title'), row('row').children(node)))
        app.step(); app.step()
        return app, node

    def queue_paste(self, app, node, text):
        x, y, width, height = app.bounds(node)
        app.queue_pointer('down', x + width / 2, y + height / 2)
        app.queue_pointer('up', x + width / 2, y + height / 2)
        app.queue_text(text)

    def test_config_validation_and_frozen_records(self):
        for make in (lambda: TimeValue(True, 0), lambda: TimeValue(24, 0),
                     lambda: TimeLocale(tuple('0012345678')),
                     lambda: TimeLocale(tuple('0123456789'), am='P', pm='PM'),
                     lambda: TimeLocale(tuple('0123456789'), am='\ud800'),
                     lambda: TimeFieldConfig(edit_generation=0),
                     lambda: TimeFieldConfig(edit_generation=1 << 64),
                     lambda: TimeFieldConfig(cycle=1),
                     lambda: TimeFieldConfig(minimum=TimeValue(12, 0), maximum=TimeValue(11, 0)),
                     lambda: TimeFieldConfig(precision=TimePrecision.MINUTE, minimum=TimeValue(9, 0, 1)),
                     lambda: TimeFieldConfig(segment_names=('x' * 65, '', '', ''))):
            with self.subTest(make=make), self.assertRaises((TypeError, ValueError)):
                make()
        config = TimeFieldConfig(edit_generation=0xFEDCBA9876543210)
        self.assertEqual(wire._copy_config(wire._config(config)), config)
        with self.assertRaises(FrozenInstanceError): config.edit_generation = 2
        self.assertEqual([C.sizeof(t) for t in (wire._Config, wire._Result, wire._Change)], [616, 80, 64])

    def test_actual_value_copy_poststep_and_no_generic_replay(self):
        app, node = self.create()
        copied = []
        node.on_time_change(copied.append)
        with self.assertRaises(TypeError): node.on_change(copied.append)
        self.queue_paste(app, node, '12:34:56')
        app.step()
        self.assertEqual(copied, [])
        self.assertEqual(node.get_value(), TimeValue(12, 34, 56))
        self.assertEqual(list(app.poll_events()), [])
        result = node.get_result()
        self.assertTrue(result.changed)
        self.assertEqual(result.queued_change_count, 1)
        self.assertEqual(app.dispatch(), 1)
        self.assertEqual(app.dispatch(), 0)
        self.assertEqual(copied[0].previous, TimeValue(9, 45, 30))
        self.assertEqual(copied[0].value, TimeValue(12, 34, 56))
        self.assertEqual(copied[0].frame_status, 0)
        with self.assertRaises(FrozenInstanceError): copied[0].frame_sequence = 0
        app.close()
        self.assertEqual(copied[0].value.second, 56)

    def test_locale_large_paste_bounds_clear_and_owner_replacement(self):
        locale = TimeLocale(tuple('α' + 'a' * 13 + str(i) for i in range(10)))
        config = TimeFieldConfig(locale, maximum=TimeValue(18, 0), edit_generation=0xFEDCBA9876543210)
        app, node = self.create(config)
        self.assertEqual(node.get_config(), config)
        text = ':'.join(''.join(locale.digits[int(d)] for d in part) for part in ('12', '34', '56'))
        self.assertEqual(len(text.encode()), 98)
        self.queue_paste(app, node, 'α'); app.step()
        self.queue_paste(app, node, text); app.step()
        records = list(app.poll_time_changes())
        self.assertEqual(len(records), 1)
        self.assertEqual(records[0].value, TimeValue(12, 34, 56))
        rejected = ':'.join(''.join(locale.digits[int(d)] for d in part) for part in ('23', '00', '00'))
        self.queue_paste(app, node, rejected); app.step()
        result = node.get_result()
        self.assertEqual(result.validation, TimeTextStatus.OUT_OF_RANGE)
        self.assertEqual(result.rejected_count, 1)
        self.assertFalse(result.changed)
        self.assertEqual(list(app.poll_time_changes()), [])
        node.clear_validation(); app.step()
        self.assertEqual(node.get_result().validation, TimeTextStatus.OK)
        # Legacy setter remains legal and never clamps to configured bounds.
        node.time_value(23, 0); app.step()
        self.assertEqual(node.get_value(), TimeValue(23, 0))
        self.assertEqual(list(app.poll_time_changes()), [])
        node.configure(replace(config, edit_generation=config.edit_generation + 1))
        self.assertEqual(node.get_config().edit_generation, config.edit_generation + 1)

    def test_readonly_disabled_and_atomic_failed_setter(self):
        app, node = self.create()
        before = node.get_config()
        node.configure(replace(before, readonly=True))
        self.queue_paste(app, node, '12:00:00'); app.step()
        self.assertEqual(node.get_value(), TimeValue(9, 45, 30))
        self.assertEqual(list(app.poll_time_changes()), [])
        node.configure(before).disabled(True)
        self.queue_paste(app, node, '13:00:00'); app.step()
        self.assertEqual(node.get_value(), TimeValue(9, 45, 30))
        with self.assertRaises(ValueError): node.time_value(24, 0)
        self.assertEqual(node.get_config(), before)
        node.disabled(False)
        # Native failure must not commit the Python configuration cache.
        with patch.object(app._api.time_field, 'time_field_configure_v1', return_value=5):
            with self.assertRaises(BridgeError): node.configure(replace(before, readonly=True))
        self.assertEqual(node._time_config, before)

    def test_queue_preflight_retries_input_once(self):
        app, node = self.create(capacity=1)
        self.queue_paste(app, node, '12:00:00'); app.step()
        first_frame = node.get_result().frame_sequence
        self.queue_paste(app, node, '13:00:00')
        with self.assertRaises(BridgeError) as error: app.step()
        self.assertEqual(error.exception.code, 5)
        self.assertEqual(node.get_value(), TimeValue(12, 0))
        self.assertEqual(node.get_result().frame_sequence, first_frame)
        self.assertEqual(len(list(app.poll_time_changes())), 1)
        app.step()
        event, = app.poll_time_changes()
        self.assertEqual(event.value, TimeValue(13, 0))
        self.assertEqual(event.frame_sequence, first_frame + 1)
        self.assertEqual(list(app.poll_time_changes()), [])

    def test_failed_frame_delivers_applied_copy(self):
        app = App(width=760, height=640); self.addCleanup(app.close)
        app.enable_time_changes(4); app.enable_slider_requests(1)
        copies = []
        node = localized_time_field('time', value=TimeValue(9, 45)).on_time_change(copies.append)
        later = slider_model('later', SliderConfig(reduced_motion=True), SliderSingle(40))
        app.mount(column('root').children(row('nested').children(node), later))
        app.step(); app.step()
        self.queue_paste(app, node, '12:34:56')
        with self.assertRaises(BridgeError) as error: app.step()
        self.assertEqual(error.exception.code, 5)
        with self.assertRaises(BridgeError): node.get_result()
        self.assertEqual(copies, [])
        app.dispatch()
        self.assertEqual(copies[0].value, TimeValue(12, 34, 56))
        self.assertEqual(copies[0].frame_status, 5)
        later.remove(); app.step()
        self.assertEqual(node.get_result().frame_status, 0)

    def test_callback_exception_guard_and_retained_next_record(self):
        app, node = self.create()
        def fail(event):
            with self.assertRaises(RuntimeError): app.step()
            with self.assertRaises(RuntimeError): app.dispatch()
            with self.assertRaises(RuntimeError): app.run()
            with self.assertRaises(RuntimeError): list(app.poll_time_changes())
            raise LookupError('application failure')
        node.on_time_change(fail)
        self.queue_paste(app, node, '12:00:00'); app.step()
        self.queue_paste(app, node, '13:00:00'); app.step()
        with self.assertRaisesRegex(LookupError, 'application failure'): app.dispatch()
        self.assertFalse(app._time_field_dispatching)
        records = []; node.on_time_change(records.append)
        self.assertEqual(app.dispatch(), 1)
        self.assertEqual(records[0].value.hour, 13)

    def test_callback_close_stops_second_and_releases_handler(self):
        app, node = self.create()
        saved = []
        class Handler:
            def __call__(self, event): saved.append(event); app.close()
        handler = Handler(); reference = weakref.ref(handler)
        node.on_time_change(handler); del handler
        self.queue_paste(app, node, '12:00:00'); app.step()
        self.queue_paste(app, node, '13:00:00'); app.step()
        self.assertEqual(app.dispatch(), 1)
        self.assertEqual(len(saved), 1)
        self.assertEqual(saved[0].value.hour, 12)
        self.assertFalse(app._time_field_dispatching)
        self.assertIsNone(node._time_callback)
        gc.collect(); self.assertIsNone(reference())
        self.assertIsNone(app._ptr)

    def test_remove_prunes_queue_without_replaying_result(self):
        app, node = self.create()
        node.on_time_change(lambda event: self.fail('removed callback'))
        self.queue_paste(app, node, '12:00:00'); app.step()
        result = node.get_result()
        node.remove()
        self.assertEqual(app.dispatch(), 0)
        with self.assertRaises(RuntimeError): node.clear_validation()
        self.assertEqual(result.value.hour, 12)
        self.assertTrue(node._disposed)

    def test_complete_optional_group_and_output_protocol(self):
        names = ['time_field_versions', 'time_field_config_size_v1', 'time_field_result_size_v1',
                 'time_field_change_size_v1', 'time_field_enable_v1', 'time_field_configure_v1',
                 'time_field_get_config_v1', 'time_field_get_result_v1',
                 'time_field_clear_validation_v1', 'time_field_poll_change_v1']
        for missing in names:
            class Partial:
                def __getattr__(self, name):
                    if name == 'cui_bridge_' + missing: raise AttributeError(name)
                    return object()
            api, bound = SimpleNamespace(lib=Partial()), []
            wire.bind_api(api, lambda *args: bound.append(args))
            self.assertIsNone(api.time_field); self.assertEqual(bound, [])
        valid = wire._Change(struct_size=64, version=1, node=wire._Handle(7, 9),
            frame_sequence=0xFEDCBA9876543210, previous=wire._Time(9, 0, 0), value=wire._Time(10, 0, 0))
        copied = wire._copy_change(valid)
        valid.value.hour = 11
        self.assertEqual(copied.value.hour, 10)
        self.assertEqual(copied.frame_sequence, 0xFEDCBA9876543210)
        for field, value in [('struct_size', 63), ('version', 2), ('reserved', 1), ('frame_sequence', 0),
                             ('frame_status', 1), ('frame_status', 99), ('validation', 2)]:
            raw = wire._Change.from_buffer_copy(bytes(valid)); setattr(raw, field, value)
            with self.subTest(field=field, value=value), self.assertRaises(BridgeError):
                wire._decode(wire._copy_change, raw)
        config = wire._config(TimeFieldConfig())
        for mutate in (lambda c: setattr(c, 'flags', 8), lambda c: setattr(c, 'cycle', 4),
                       lambda c: setattr(c, 'edit_generation', 0), lambda c: setattr(c.minimum, 'hour', 1),
                       lambda c: setattr(c.tokens[0], 'length', 17), lambda c: c.names[0].bytes.__setitem__(63, 1)):
            raw = wire._Config.from_buffer_copy(bytes(config)); mutate(raw)
            with self.assertRaises(BridgeError): wire._decode(wire._copy_config, raw)

    def test_malformed_provider_poll_consumed_without_callback(self):
        raw = wire._Change(struct_size=64, version=1, reserved=1, node=wire._Handle(7, 9),
            frame_sequence=1, previous=wire._Time(9, 0, 0), value=wire._Time(10, 0, 0))
        calls = []
        def provider(ptr, out):
            calls.append(ptr)
            if len(calls) > 1: return 1
            C.memmove(out, C.byref(raw), C.sizeof(raw))
            return 0
        node = localized_time_field('provider')
        app = SimpleNamespace(_ptr=1, _time_field_dispatching=False,
            _by_handle={(7, 9): node}, _api=SimpleNamespace(check=lambda status: None))
        with patch.object(wire, '_api', return_value=SimpleNamespace(time_field_poll_change_v1=provider)):
            with self.assertRaises(BridgeError): list(wire.poll(app))
            self.assertEqual(list(wire.poll(app)), [])
        self.assertEqual(len(calls), 2)

    def test_owner_thread_and_app_local_queue(self):
        import threading
        app, node = self.create()
        before = node.get_config()
        errors = []
        def foreign():
            for action in (lambda: node.configure(replace(before, readonly=True)), app.step, app.close):
                try: action()
                except (BridgeError, RuntimeError) as error: errors.append(error)
        thread = threading.Thread(target=foreign); thread.start(); thread.join()
        self.assertEqual(len(errors), 3)
        self.assertEqual(node.get_config(), before)
        with App(width=760, height=640) as other:
            second = localized_time_field('other', value=TimeValue(10, 0))
            other.mount(column('root').children(second)); other.step(); other.step()
            self.queue_paste(app, node, '12:00:00'); app.step()
            self.assertEqual(list(other.poll_time_changes()), [])
            event, = app.poll_time_changes()
            self.assertEqual(event.value.hour, 12)
            self.assertEqual(second.get_value().hour, 10)

    @unittest.skipUnless(os.environ.get('CUI_TIME_FIELD_OLD_LIBRARY'), 'actual older module path required')
    def test_actual_old_library_in_separate_process(self):
        program = """
from cui_bridge import *
with App(library=OLD) as app:
    assert not app.supports_time_field()
    try: app.mount(column('root').children(localized_time_field('localized')))
    except RuntimeError: pass
    else: raise AssertionError('old module advertised new API')
    assert app._root is None and not app._by_handle
    field=time_field('legacy').time_value(9,45,30)
    app.mount(column('root').children(field)); app.step()
    assert field.current_time()==(9,45,30)
"""
        old = str(Path(os.environ['CUI_TIME_FIELD_OLD_LIBRARY']).resolve())
        subprocess.run([sys.executable, '-c', 'OLD=' + repr(old) + '\n' + program], check=True)


if __name__ == '__main__':
    unittest.main()
