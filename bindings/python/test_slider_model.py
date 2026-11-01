"""Real controlled authored UIs; run through scripts/test_local.py, never desktop."""
from dataclasses import FrozenInstanceError, replace
import gc
import importlib
import math
import os
from pathlib import Path
import subprocess
import sys
import unittest
import weakref

from cui_bridge import (App, BridgeError, column, row, label, slider, slider_model,
    SliderSingle, SliderRange, SliderConfig, SliderDomain, SliderScale,
    SliderOrientation, SliderLayout, SliderPart, SliderPhase, hover_card)

wire = importlib.import_module("cui_bridge.slider_model")


class ControlledSlider(unittest.TestCase):
    def test_domain_boundaries_match_c_before_authoring(self):
        for low, high, scale in ((-1e308, 1e308, SliderScale.LINEAR),
                                  (1, 1, SliderScale.LOGARITHMIC)):
            with self.subTest(low=low, high=high, scale=scale):
                with self.assertRaises(ValueError):
                    SliderDomain(low, high, scale=scale)
        self.assertEqual(SliderDomain(1, 1).maximum, 1)
        self.assertGreater(SliderDomain(1, math.nextafter(1, 2),
                           scale=SliderScale.LOGARITHMIC).maximum, 1)
        self.assertEqual(SliderDomain(-1e307, 1e307).maximum, 1e307)

    def create(self, value=None, axis=SliderOrientation.HORIZONTAL, callback=None, **kw):
        app = App(width=640, height=480, **kw)
        self.addCleanup(app.close)
        value = value or SliderRange(20, 60)
        config = SliderConfig(orientation=axis, reduced_motion=True,
                              source_generation=0xFEDCBA9876543210)
        node = slider_model("selection", config, value).on_request(callback)
        app.mount(column("root").children(label("Controlled interval", key="title"),
                  row("row").children(column("nested").children(node))))
        app.step(); app.step()
        return app, node

    def focus(self, app, node):
        x, y, w, h = node.part_bounds(SliderPart.END_THUMB)
        app.queue_pointer("down", x + w / 2, y + h / 2)
        app.queue_pointer("up", x + w / 2, y + h / 2)
        app.step()
        return list(app.poll_slider_requests())

    def test_full_pairs_both_axes_and_after_step_order(self):
        for axis in SliderOrientation:
            for initial in (SliderSingle(60), SliderRange(20, 60)):
                with self.subTest(axis=axis, initial=initial):
                    app, node = self.create(initial, axis)
                    requests = self.focus(app, node)
                    self.assertEqual(requests[0].phase, SliderPhase.BEGIN)
                    self.assertEqual(requests[-1].phase, SliderPhase.COMMIT)
                    self.assertEqual(node.get_value(), initial)  # No autoaccept.
                    alternative = SliderSingle(50) if isinstance(initial, SliderSingle) else SliderRange(22, 50)
                    app.accept_slider_request(requests[-1].receipt, alternative)
                    self.assertEqual(node.get_value(), alternative)
                    seen, owner = [], [alternative]
                    def changed(request):
                        self.assertIsNotNone(node.part_bounds(SliderPart.TRACK))  # C is no longer stepping.
                        self.assertEqual(request.current, alternative)
                        seen.append(request)
                        if request.phase == SliderPhase.UPDATE:
                            app.accept_slider_request(request.receipt, request.requested)
                            owner[0] = request.requested  # External owner mutation is allowed.
                    node.on_request(changed)
                    app.queue_key("down", "left")
                    app.queue_key("down", "left", modifiers=15, repeat=True)
                    app.step()
                    self.assertEqual(seen, [])
                    self.assertEqual(app.dispatch(), 6)
                    self.assertEqual([r.phase for r in seen], [SliderPhase.BEGIN, SliderPhase.UPDATE, SliderPhase.COMMIT] * 2)
                    self.assertEqual([seen[1].requested.end, seen[4].requested.end], [49, 48])
                    self.assertEqual(seen[4].input.modifiers, 15)
                    self.assertEqual(seen[4].input.repeat, 1)
                    self.assertEqual(seen[4].receipt.source_generation, 0xFEDCBA9876543210)
                    self.assertIs(type(seen[4].receipt.source_generation), int)
                    self.assertEqual(node.get_value(), owner[0])
                    self.assertEqual(list(app.poll_events()), [])
                    app.close()

    def test_rejection_expiry_immutable_copy_and_precision(self):
        app, node = self.create(SliderSingle(math.nextafter(60, math.inf)))
        self.assertEqual(node.get_value().end, math.nextafter(60, math.inf))
        self.focus(app, node)
        seen = []
        node.on_request(seen.append)
        before = node.get_value()
        app.queue_key("down", "left"); app.queue_key("down", "left")
        app.step(); self.assertEqual(app.dispatch_slider_requests(), 6)
        self.assertEqual(node.get_value(), before)
        self.assertNotEqual(seen[1].requested, before)
        saved = seen[1]
        with self.assertRaises(FrozenInstanceError):
            saved.requested.end = 5
        app.step()
        with self.assertRaises(BridgeError) as error:
            app.accept_slider_request(saved.receipt, saved.requested)
        self.assertEqual(error.exception.code, 8)
        app.close()
        self.assertEqual(saved.receipt.source_generation, 0xFEDCBA9876543210)
        self.assertEqual(saved.current, before)

    def test_exception_consumes_once_leaves_later_entries_and_resets_guard(self):
        app, node = self.create()
        self.focus(app, node)
        popped = []
        def fail(request):
            popped.append(request)
            raise LookupError("application failed")
        node.on_request(fail)
        app.queue_key("down", "right"); app.step()
        with self.assertRaisesRegex(LookupError, "application failed"):
            app.dispatch()
        self.assertEqual(len(popped), 1)
        later = list(app.poll_slider_requests())
        self.assertEqual([r.phase for r in later], [SliderPhase.UPDATE, SliderPhase.COMMIT])
        self.assertGreater(later[0].receipt.request_sequence, popped[0].receipt.request_sequence)
        app.step()  # Guard restored, failed callback not retried or accepted.
        self.assertEqual(node.get_value(), SliderRange(20, 60))

    def test_reentrancy_rejected_but_enqueue_and_accept_allowed(self):
        app, node = self.create()
        self.focus(app, node)
        seen = []
        def changed(request):
            for nested in (app.step, app.run, app.dispatch, app.dispatch_slider_requests):
                with self.assertRaises(RuntimeError):
                    nested()
            seen.append(request)
            if request.phase == SliderPhase.UPDATE:
                app.accept_slider_request(request.receipt, request.requested)
                app.queue_key("down", "right")  # Later work, not immediate input replay.
        node.on_request(changed)
        app.queue_key("down", "left"); app.step(); app.dispatch()
        self.assertEqual(len(seen), 3)
        app.step()
        self.assertEqual(len(list(app.poll_slider_requests())), 3)

    def test_removal_prunes_later_copies_and_releases_handler(self):
        app, node = self.create()
        self.focus(app, node)
        seen = []
        class Remove:
            def __call__(self, request):
                seen.append(request)
                node.remove()
        handler = Remove()
        reference = weakref.ref(handler)
        node.on_request(handler)
        del handler
        app.queue_key("down", "right"); app.step()
        self.assertEqual(app.dispatch_slider_requests(), 1)
        self.assertEqual(list(app.poll_slider_requests()), [])
        gc.collect(); self.assertIsNone(reference())
        self.assertEqual(seen[0].phase, SliderPhase.BEGIN)
        with self.assertRaises(RuntimeError):
            node.get_value()
        app.close()
        self.assertEqual(seen[0].current, SliderRange(20, 60))

    def test_callback_accept_then_close_keeps_copy_and_stops_all_later_dispatch(self):
        app, node = self.create()
        self.focus(app, node)
        saved, calls, poll_calls = [], [], []
        api = app._api.slider_model
        original_poll = api.poll_slider_request_v1
        def guarded_poll(*args):
            self.assertTrue(app._ptr, "dispatch must not call C after callback destroys App")
            poll_calls.append(1)
            return original_poll(*args)
        api.poll_slider_request_v1 = guarded_poll
        class Close:
            def __call__(self, request):
                calls.append(request.phase)
                if request.phase == SliderPhase.UPDATE:
                    app.accept_slider_request(request.receipt, request.requested)
                    saved.append(request)
                    app.close()
        handler = Close()
        reference = weakref.ref(handler)
        node.on_request(handler)
        del handler
        app.queue_key("down", "right"); app.step()
        self.assertEqual(app.dispatch(), 2)
        self.assertEqual(calls, [SliderPhase.BEGIN, SliderPhase.UPDATE])
        self.assertEqual(len(poll_calls), 2)
        self.assertFalse(app._ptr)
        self.assertFalse(app._slider_dispatching)
        self.assertIsNone(node._request_callback)
        self.assertEqual(len(saved), 1)
        self.assertEqual(saved[0].current, SliderRange(20, 60))
        self.assertEqual(saved[0].requested, SliderRange(20, 61))
        self.assertEqual(saved[0].receipt.source_generation, 0xFEDCBA9876543210)
        api.poll_slider_request_v1 = original_poll
        gc.collect(); self.assertIsNone(reference())
        with self.assertRaises(RuntimeError):
            app.accept_slider_request(saved[0].receipt, saved[0].requested)

    def test_config_disable_readonly_atomic_errors_and_namespace(self):
        app, node = self.create()
        self.focus(app, node)
        app.queue_key("down", "left"); app.step()
        request = list(app.poll_slider_requests())[1]
        with App(width=200, height=200) as other:
            other.enable_slider_requests()
            with self.assertRaises(ValueError):
                other.accept_slider_request(request.receipt, request.requested)
        with self.assertRaises(BridgeError):
            app.accept_slider_request(replace(request.receipt, request_sequence=(1 << 64) - 1), request.requested)
        self.assertEqual(node.get_value(), SliderRange(20, 60))
        readonly = replace(node.get_config(), readonly=True)
        node.configure(readonly, SliderRange(30, 70))
        with self.assertRaises(BridgeError):
            app.accept_slider_request(request.receipt, request.requested)
        app.step(); app.queue_key("down", "left"); app.step()
        self.assertEqual(list(app.poll_slider_requests()), [])
        node.disabled(); app.step()
        self.assertIsNone(node.part_bounds(SliderPart.END_THUMB))
        self.assertIsNotNone(node.part_bounds(SliderPart.TRACK))
        self.assertIsNotNone(node.part_bounds(SliderPart.FILL))
        self.assertEqual(node.get_value(), SliderRange(30, 70))

    def test_parent_validation_before_mount_and_capacity(self):
        with App() as app:
            node = slider_model("typed", SliderConfig(), SliderSingle(40))
            with self.assertRaises(ValueError):
                app.mount(column("root").children(hover_card("hover").children(node)))
            self.assertFalse(app._slider_enabled)
            self.assertIsNone(app._root)
        app = App(); self.addCleanup(app.close)
        app.enable_slider_requests(1)
        node = slider_model("typed", SliderConfig(reduced_motion=True), SliderSingle(40))
        app.mount(column("root").children(node)); app.step(); app.step()
        x, y, w, h = node.part_bounds(SliderPart.END_THUMB)
        app.queue_pointer("down", x+w/2, y+h/2); app.queue_pointer("up", x+w/2, y+h/2)
        with self.assertRaises(BridgeError) as error:
            app.step()
        self.assertEqual(error.exception.code, 5)
        self.assertEqual(list(app.poll_slider_requests()), [])
        self.assertEqual(node.get_value(), SliderSingle(40))

    def test_legacy_callbacks_keep_preexisting_phase_before_typed_fifo(self):
        with App(width=640, height=480) as app:
            order = []
            legacy = slider("volume").range(0, 100, 1).value(40).on_change(lambda e: order.append("legacy"))
            node = slider_model("typed", SliderConfig(reduced_motion=True), SliderRange(20, 60))
            node.on_request(lambda request: order.append(request.phase))
            app.mount(column("root").children(legacy, node)); app.step(); app.step()
            x, y, w, h = app.bounds(legacy)
            app.queue_pointer("down", x+w*.7, y+h/2); app.queue_pointer("up", x+w*.7, y+h/2)
            x, y, w, h = node.part_bounds(SliderPart.END_THUMB)
            app.queue_pointer("down", x+w/2, y+h/2); app.queue_pointer("up", x+w/2, y+h/2)
            app.step(); self.assertEqual(order, [])
            app.dispatch()
            self.assertEqual(order[0], "legacy")
            self.assertTrue(any(isinstance(item, SliderPhase) for item in order[1:]))
            self.assertEqual(legacy.current_value(), 70)
            self.assertEqual(node.get_value(), SliderRange(20, 60))

    def test_configure_in_callback_expires_later_authority_without_retagging_copy(self):
        app, node = self.create()
        self.focus(app, node)
        before = node.get_config()
        copied = []
        def changed(request):
            copied.append(request)
            if request.phase == SliderPhase.BEGIN:
                node.configure(replace(before, source_generation=7), SliderRange(30, 70))
            else:
                with self.assertRaises(BridgeError):
                    app.accept_slider_request(request.receipt, request.requested)
        node.on_request(changed)
        app.queue_key("down", "left"); app.step(); app.dispatch()
        self.assertEqual(len(copied), 3)
        self.assertTrue(all(r.receipt.source_generation == before.source_generation for r in copied))
        self.assertEqual(node.get_value(), SliderRange(30, 70))
        self.assertEqual(node.get_config().source_generation, 7)

    def test_log_reverse_vertical_config_roundtrip_and_continuous_owner(self):
        app, node = self.create()
        config = SliderConfig(domain=SliderDomain(1, 100, 0, SliderScale.LOGARITHMIC),
            orientation=SliderOrientation.VERTICAL, reverse_fill=True, reduced_motion=True)
        owner = SliderRange(math.nextafter(2, math.inf), math.nextafter(40, math.inf))
        node.configure(config, owner)
        app.step(); app.step()
        self.assertEqual(node.get_config(), config)
        self.assertEqual(node.get_value(), owner)
        self.assertIsNotNone(node.part_bounds(SliderPart.START_THUMB))
        requests = self.focus(app, node)
        self.assertTrue(requests)
        self.assertTrue(all(r.domain.scale == SliderScale.LOGARITHMIC for r in requests))
        self.assertEqual(node.get_value(), owner)  # Still controlled, no accept.

    def test_local_validation_and_active_fields(self):
        for value in (True, float("nan"), float("inf")):
            with self.assertRaises((ValueError, TypeError)):
                SliderSingle(value)
        with self.assertRaises(ValueError): SliderRange(60, 20)
        with self.assertRaises(ValueError): SliderDomain(0, 10, 1, SliderScale.LOGARITHMIC)
        with self.assertRaises(ValueError): SliderConfig(source_generation=1 << 64)
        with self.assertRaises(TypeError): SliderConfig(requests=1)
        self.assertEqual(SliderConfig(layout=SliderLayout.PARENT, length=float("nan")).length, 0)
        self.assertEqual(wire.C.sizeof(wire._Request), 296)
        self.assertEqual(wire.C.sizeof(wire._Receipt), 56)

    def test_complete_optional_group_and_copied_uint64_records(self):
        from types import SimpleNamespace
        names = ("slider_model_versions", "slider_config_v1_size", "slider_value_v1_size",
            "slider_receipt_v1_size", "slider_request_v1_size", "slider_presentation_v1_size",
            "slider_queue_enable_v1", "slider_model_create_v1", "slider_model_configure_v1",
            "slider_model_get_config_v1", "slider_model_get_value_v1", "slider_model_set_value_v1",
            "poll_slider_request_v1", "accept_slider_request_v1", "slider_model_get_presentation_v1",
            "slider_model_part_bounds_v1")
        for missing in names:
            class Partial:
                def __getattr__(self, name):
                    if name == "cui_bridge_" + missing:
                        raise AttributeError(name)
                    return object()
            api, calls = SimpleNamespace(lib=Partial()), []
            wire.bind_api(api, lambda *args: calls.append(args))
            self.assertIsNone(api.slider_model)
            self.assertEqual(calls, [])
        raw = wire._Request()
        raw.struct_size, raw.schema_version, raw.phase, raw.thumb = 296, 1, 1, 1
        raw.event_kind, raw.event_source, raw.event_flags = 1, 3, 7
        raw.receipt = wire._Receipt(56, 1, (1 << 63)+1, wire._Handle(2, 3),
                                   (1 << 64)-1, (1 << 63)+2, (1 << 63)+3, (1 << 63)+4)
        raw.event_target, raw.event_frame, raw.event_index = (1 << 64)-2, (1 << 63)+3, 1
        raw.current, raw.requested = wire._Value(1, 0, 20, 60), wire._Value(1, 0, 22, 59)
        raw.domain = wire._Domain(0, 100, 1, 0, 0)
        raw.gesture_domain = wire._Domain(0, 100, 1, 0, 0)
        raw.input.flags, raw.input.type, raw.input.key, raw.input.modifiers = 1, 3, 5, 15
        copied = wire._copy_request(raw, object())
        raw.receipt.source_generation = 0
        raw.current.start = 0
        raw.event_target = 0
        self.assertEqual(copied.receipt.source_generation, (1 << 64)-1)
        self.assertEqual(copied.event_target, (1 << 64)-2)
        self.assertEqual(copied.current, SliderRange(20, 60))
        self.assertEqual(copied.requested, SliderRange(22, 59))
        raw.input.flags = 2
        with self.assertRaises(ValueError): wire._copy_request(raw, object())

    def test_copied_wire_canonical_input_evidence(self):
        def record():
            raw = wire._Request()
            raw.struct_size, raw.schema_version, raw.phase = 296, 1, 1
            raw.event_kind, raw.event_index = 1, (1 << 64) - 1
            raw.receipt = wire._Receipt(56, 1, 1, wire._Handle(2, 3), 4, 5, 6, 7)
            raw.current, raw.requested = wire._Value(0, 0, 0, 40), wire._Value(0, 0, 0, 41)
            raw.domain = raw.gesture_domain = wire._Domain(0, 100, 1, 0, 0)
            return raw
        namespace = object()
        saved = wire._copy_request(record(), namespace)
        for field, value in (("type", 3), ("key", 5), ("modifiers", 1),
                             ("repeat", 1), ("x", 1), ("y", 1),
                             ("pointer_type", 1), ("pointer_button", 1),
                             ("accessibility_action", 2), ("accessibility_value", .25),
                             ("pointer_id", 1), ("pointer_device", 1),
                             ("accessibility_target", 1)):
            with self.subTest(absent_input=field):
                raw = record(); setattr(raw.input, field, value)
                with self.assertRaises(ValueError): wire._copy_request(raw, namespace)
        for field, value in (("repeat", 2), ("x", math.nan), ("y", math.inf),
                             ("accessibility_value", math.nan), ("accessibility_value", .25)):
            with self.subTest(active_input=field, value=value):
                raw = record(); raw.input.flags, raw.input.type = 1, 3
                setattr(raw.input, field, value)
                with self.assertRaises(ValueError): wire._copy_request(raw, namespace)
        raw = record(); raw.event_source = 1
        with self.assertRaises(ValueError): wire._copy_request(raw, namespace)
        with self.assertRaises(ValueError): wire._copy_request(record(), None)
        raw = record(); raw.input.flags, raw.input.type = 1, 13
        raw.input.accessibility_action = 2  # INCREMENT has no numeric evidence.
        self.assertEqual(wire._copy_request(raw, namespace).input.accessibility_value, 0)
        raw.input.accessibility_value = .25
        with self.assertRaises(ValueError): wire._copy_request(raw, namespace)
        raw.input.flags, raw.input.accessibility_action = 3, 4
        self.assertEqual(wire._copy_request(raw, namespace).input.accessibility_value, .25)
        self.assertEqual(saved.current, SliderSingle(40))
        self.assertIs(saved.receipt._namespace, namespace)
        # A malformed provider record is consumed, never exposed as a request.
        from types import SimpleNamespace
        from unittest.mock import patch
        invalid = record(); invalid.input.x = 1
        def invalid_poll(pointer, size, output):
            wire.C.memmove(output, wire.C.byref(invalid), wire.C.sizeof(invalid))
            return 0
        app = SimpleNamespace(_ptr=1, _slider_namespace=namespace,
                              _api=SimpleNamespace(check=lambda status: None))
        with patch.object(wire, "_api", return_value=SimpleNamespace(
                poll_slider_request_v1=invalid_poll)):
            with self.assertRaises(BridgeError): next(wire.poll(app))
        self.assertEqual(saved.current, SliderSingle(40))

    @unittest.skipUnless(os.environ.get("CUI_BRIDGE_OLD_LIBRARY"), "actual pre-typed library required")
    def test_actual_old_library_defaults_still_work_and_new_use_fails_before_mount(self):
        program = '''
from cui_bridge import *
with App() as app:
    assert not app.supports_slider_model()
    typed = slider_model("typed", SliderConfig(), SliderRange(20, 60))
    try: app.mount(column("root").children(typed))
    except RuntimeError: pass
    else: raise AssertionError("old library unexpectedly enabled typed model")
    assert app._root is None and not app._by_handle
    legacy = slider("legacy").range(0, 100, 1).value(40)
    app.mount(column("legacy-root").children(legacy)); app.step()
    assert legacy.current_value() == 40
'''
        env = dict(os.environ, CUI_BRIDGE_LIBRARY=os.environ["CUI_BRIDGE_OLD_LIBRARY"])
        subprocess.run([sys.executable, "-c", program], env=env, check=True)


if __name__ == "__main__":
    unittest.main()
