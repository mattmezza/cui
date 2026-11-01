"""Owned controlled Slider records; all user callbacks execute after C returns."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass, field
from enum import IntEnum
import math
import threading
from types import SimpleNamespace
from typing import Callable

from . import BridgeError, Widget, _Handle, _Rect, _utf8


class SliderScale(IntEnum):
    LINEAR = 0
    LOGARITHMIC = 1


class SliderOrientation(IntEnum):
    HORIZONTAL = 0
    VERTICAL = 1


class SliderLayout(IntEnum):
    FIXED = 0
    PARENT = 1


class SliderPhase(IntEnum):
    BEGIN = 0
    UPDATE = 1
    COMMIT = 2
    CANCEL = 3


class SliderReason(IntEnum):
    INPUT = 0
    DISABLED = 1
    READONLY = 2
    ZERO_SPAN = 3
    SOURCE_REPLACED = 4
    DOMAIN_CHANGED = 5
    MODE_CHANGED = 6
    AXIS_CHANGED = 7
    OMITTED = 8
    CAPTURE_LOST = 9


class SliderPart(IntEnum):
    ROOT = 0
    TRACK = 1
    FILL = 2
    START_THUMB = 3
    END_THUMB = 4


def _double(value):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TypeError("Slider numbers must be int/float, not bool")
    value = float(value)
    if not math.isfinite(value):
        raise ValueError("Slider numbers must be finite")
    return value


def _uint(value, bits=64):
    if type(value) is not int or not 0 <= value < 1 << bits:
        raise ValueError(f"expected uint{bits}")
    return value


@dataclass(frozen=True)
class SliderSingle:
    end: float

    def __post_init__(self):
        object.__setattr__(self, "end", _double(self.end))


@dataclass(frozen=True)
class SliderRange:
    start: float
    end: float

    def __post_init__(self):
        object.__setattr__(self, "start", _double(self.start))
        object.__setattr__(self, "end", _double(self.end))
        if self.start > self.end:
            raise ValueError("range start must not exceed end")


@dataclass(frozen=True)
class SliderDomain:
    minimum: float = 0
    maximum: float = 100
    step: float = 1
    scale: SliderScale = SliderScale.LINEAR

    def __post_init__(self):
        for name in ("minimum", "maximum", "step"):
            object.__setattr__(self, name, _double(getattr(self, name)))
        if not isinstance(self.scale, SliderScale):
            raise TypeError("expected SliderScale")
        if (self.minimum > self.maximum or not math.isfinite(self.maximum - self.minimum) or
                self.step < 0 or (self.scale == SliderScale.LOGARITHMIC and
                                  (self.minimum <= 0 or self.maximum <= self.minimum))):
            raise ValueError("invalid Slider domain")


@dataclass(frozen=True)
class SliderConfig:
    domain: SliderDomain = SliderDomain()
    orientation: SliderOrientation = SliderOrientation.HORIZONTAL
    layout: SliderLayout = SliderLayout.FIXED
    length: float = 200
    rem: float = 16
    source_generation: int = 0
    requests: bool = True
    readonly: bool = False
    reverse_fill: bool = False
    reduced_motion: bool = False
    hide_focus: bool = False

    def __post_init__(self):
        if not isinstance(self.domain, SliderDomain) or not isinstance(self.orientation, SliderOrientation) or not isinstance(self.layout, SliderLayout):
            raise TypeError("typed Slider domain/orientation/layout required")
        _uint(self.source_generation)
        for name in ("requests", "readonly", "reverse_fill", "reduced_motion", "hide_focus"):
            if type(getattr(self, name)) is not bool:
                raise TypeError(f"{name} must be bool")
        rem = C.c_float(_double(self.rem)).value
        if not math.isfinite(rem) or rem <= 0 or not math.isfinite(C.c_float(rem * 1.5).value):
            raise ValueError("invalid float32 rem")
        object.__setattr__(self, "rem", rem)
        length = C.c_float(_double(self.length)).value if self.layout == SliderLayout.FIXED else 0.0
        if self.layout == SliderLayout.FIXED and (not math.isfinite(length) or length <= 0):
            raise ValueError("invalid fixed float32 length")
        object.__setattr__(self, "length", length)


@dataclass(frozen=True)
class SliderReceipt:
    app_epoch: int
    node: tuple[int, int]
    source_generation: int
    configuration_revision: int
    submission_frame: int
    request_sequence: int
    _namespace: object = field(default=None, repr=False, compare=False)

    def __post_init__(self):
        if not isinstance(self.node, (tuple, list)) or len(self.node) != 2:
            raise ValueError("receipt node must be a full slot/generation pair")
        object.__setattr__(self, "node", tuple(_uint(v, 32) for v in self.node))
        for name in ("app_epoch", "source_generation", "configuration_revision", "submission_frame", "request_sequence"):
            _uint(getattr(self, name))


@dataclass(frozen=True)
class SliderInput:
    type: int
    flags: int
    key: int
    modifiers: int
    x: float
    y: float
    repeat: int
    pointer_type: int
    pointer_button: int
    accessibility_action: int
    pointer_id: int
    pointer_device: int
    accessibility_target: int
    accessibility_value: float


@dataclass(frozen=True)
class SliderRequest:
    phase: SliderPhase
    reason: SliderReason
    thumb: int
    event_kind: int
    event_source: int
    event_flags: int
    receipt: SliderReceipt
    event_target: int
    event_frame: int
    event_index: int
    current: SliderSingle | SliderRange
    requested: SliderSingle | SliderRange
    domain: SliderDomain
    gesture_domain: SliderDomain
    input: SliderInput


@dataclass(frozen=True)
class SliderPresentation:
    render_status: int
    core_time: float
    next_wake_seconds: float


class _Value(C.Structure):
    _fields_ = [("kind", C.c_uint32), ("reserved", C.c_uint32), ("start", C.c_double), ("end", C.c_double)]


class _Domain(C.Structure):
    _fields_ = [(n, C.c_double) for n in ("minimum", "maximum", "step")] + [("scale", C.c_uint32), ("reserved", C.c_uint32)]


class _Config(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "schema_version", "flags", "orientation")] + [("source_generation", C.c_uint64), ("domain", _Domain), ("rem", C.c_float), ("length", C.c_float), ("layout", C.c_uint32), ("reserved", C.c_uint32)]


class _Queue(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "schema_version", "capacity", "reserved")]


class _Receipt(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("schema_version", C.c_uint32), ("app_epoch", C.c_uint64), ("node", _Handle)] + [(n, C.c_uint64) for n in ("source_generation", "configuration_revision", "submission_frame", "request_sequence")]


class _Input(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("type", "flags", "key", "modifiers")] + [("x", C.c_float), ("y", C.c_float)] + [(n, C.c_uint32) for n in ("repeat", "pointer_type", "pointer_button", "accessibility_action")] + [(n, C.c_uint64) for n in ("pointer_id", "pointer_device", "accessibility_target")] + [("accessibility_value", C.c_double)]


class _Request(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "schema_version", "phase", "reason", "thumb", "event_kind", "event_source", "event_flags")] + [("receipt", _Receipt)] + [(n, C.c_uint64) for n in ("event_target", "event_frame", "event_index")] + [("current", _Value), ("requested", _Value), ("domain", _Domain), ("gesture_domain", _Domain), ("input", _Input)]


class _Presentation(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "schema_version", "render_status", "reserved")] + [("core_time", C.c_double), ("next_wake_seconds", C.c_double)]


def bind_api(api, bind):
    """Resolve the COMPLETE optional group without breaking old library loads."""
    specs = {
        "slider_model_versions": (C.c_uint32,),
        **{f"slider_{name}_v1_size": (C.c_uint32,) for name in ("config", "value", "receipt", "request", "presentation")},
        "slider_queue_enable_v1": (C.c_int, C.c_void_p, C.c_uint32, C.POINTER(_Queue)),
        "slider_model_create_v1": (C.c_int, C.c_void_p, _Handle, C.c_char_p, C.c_uint32, C.POINTER(_Config), C.c_uint32, C.POINTER(_Value), C.POINTER(_Handle)),
        "slider_model_configure_v1": (C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Config), C.c_uint32, C.POINTER(_Value)),
        "slider_model_get_config_v1": (C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Config)),
        "slider_model_get_value_v1": (C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Value)),
        "slider_model_set_value_v1": (C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Value)),
        "poll_slider_request_v1": (C.c_int, C.c_void_p, C.c_uint32, C.POINTER(_Request)),
        "accept_slider_request_v1": (C.c_int, C.c_void_p, C.c_uint32, C.POINTER(_Receipt), C.c_uint32, C.POINTER(_Value)),
        "slider_model_get_presentation_v1": (C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Presentation)),
        "slider_model_part_bounds_v1": (C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Rect)),
    }
    api.slider_model = None
    if all(hasattr(api.lib, "cui_bridge_" + name) for name in specs):
        api.slider_model = SimpleNamespace(**{name: bind(name, *spec) for name, spec in specs.items()})


def supported(app):
    app._ensure_open()
    api = app._api.slider_model
    if api is None or not api.slider_model_versions() & 1:
        return False
    return all(C.sizeof(wire) == size and getattr(api, f"slider_{name}_v1_size")() == size
               for name, wire, size in (("config", _Config, 72), ("value", _Value, 24),
                    ("receipt", _Receipt, 56), ("request", _Request, 296), ("presentation", _Presentation, 32)))


def _api(app):
    if not supported(app):
        raise RuntimeError("controlled Slider V1 API unavailable or incompatible")
    return app._api.slider_model


def enable(app, capacity=4096):
    if type(capacity) is not int or not 0 <= capacity <= 4096:
        raise ValueError("request capacity must be 0..4096")
    api = _api(app)
    wire = _Queue(16, 1, capacity, 0)
    app._api.check(api.slider_queue_enable_v1(app._ptr, 16, C.byref(wire)))
    if not getattr(app, "_slider_enabled", False):
        app._slider_namespace = object()  # No App/C pointer retained by copied receipts.
        app._slider_owner_thread = threading.get_ident()
    app._slider_enabled = True
    return app


def _value(value, domain=None):
    if not isinstance(value, (SliderSingle, SliderRange)):
        raise TypeError("expected SliderSingle or SliderRange")
    start = value.start if isinstance(value, SliderRange) else 0.0
    if domain is not None and (value.end < domain.minimum or value.end > domain.maximum or
                              (isinstance(value, SliderRange) and start < domain.minimum)):
        raise ValueError("confirmed pair is outside the domain")
    return _Value(int(isinstance(value, SliderRange)), 0, start, value.end)


def _domain(domain):
    return _Domain(domain.minimum, domain.maximum, domain.step, domain.scale, 0)


def _config(config):
    if not isinstance(config, SliderConfig):
        raise TypeError("expected SliderConfig")
    flags = sum(int(getattr(config, name)) << i for i, name in enumerate(
        ("requests", "readonly", "reverse_fill", "reduced_motion", "hide_focus")))
    return _Config(72, 1, flags, config.orientation, config.source_generation,
                   _domain(config.domain), config.rem, config.length, config.layout, 0)


def _copy_value(wire):
    if wire.reserved or wire.kind not in (0, 1) or (wire.kind == 0 and wire.start != 0):
        raise ValueError("unknown value record")
    return SliderSingle(wire.end) if wire.kind == 0 else SliderRange(wire.start, wire.end)


def _copy_domain(wire):
    if wire.reserved:
        raise ValueError("unknown domain record")
    return SliderDomain(wire.minimum, wire.maximum, wire.step, SliderScale(wire.scale))


def _receipt(wire, namespace):
    if wire.struct_size != 56 or wire.schema_version != 1:
        raise ValueError("unknown receipt record")
    return SliderReceipt(wire.app_epoch, (wire.node.slot, wire.node.generation), wire.source_generation,
        wire.configuration_revision, wire.submission_frame, wire.request_sequence, namespace)


def _copy_request(wire, namespace):
    if wire.struct_size != 296 or wire.schema_version != 1 or wire.thumb not in (0, 1) or wire.event_kind not in (1, 3, 5) or wire.event_source not in range(5) or wire.event_flags & ~7:
        raise ValueError("unknown request vocabulary")
    i = wire.input
    if i.flags & ~3 or i.modifiers & ~15 or (i.flags & 1 and
        (i.type not in (0, 1, 2, 3, 7, 13) or i.key not in (0, 4, 5, 6, 7, 8, 9, 10, 19, 20) or
         i.pointer_type not in (0, 1, 2) or i.pointer_button not in (0, 1, 2) or i.accessibility_action not in (0, 2, 3, 4))):
        raise ValueError("unknown input vocabulary")
    if i.flags & 2 and (not i.flags & 1 or i.type != 13 or i.accessibility_action != 4 or not math.isfinite(i.accessibility_value)):
        raise ValueError("incoherent typed numeric evidence")
    if not wire.event_flags & 1 and wire.event_frame != 0:
        raise ValueError("inactive event frame must be zero")
    if not wire.event_flags & 2 and wire.event_index != (1 << 64) - 1:
        raise ValueError("inactive event index must be UINT64_MAX")
    copied = SliderInput(*(getattr(i, name) for name, _ in _Input._fields_))
    return SliderRequest(SliderPhase(wire.phase), SliderReason(wire.reason), wire.thumb,
        wire.event_kind, wire.event_source, wire.event_flags, _receipt(wire.receipt, namespace),
        wire.event_target, wire.event_frame, wire.event_index, _copy_value(wire.current),
        _copy_value(wire.requested), _copy_domain(wire.domain), _copy_domain(wire.gesture_domain), copied)


def poll(app):
    api = _api(app)
    while app._ptr:
        wire = _Request()
        status = api.poll_slider_request_v1(app._ptr, 296, C.byref(wire))
        if status == 1:
            return
        app._api.check(status)
        try:
            request = _copy_request(wire, app._slider_namespace)
        except (ValueError, TypeError, OverflowError) as error:
            raise BridgeError(8, "invalid controlled Slider request record") from error
        yield request  # No ctypes view or C call remains borrowed across user code.


def accept(app, receipt, value):
    api = _api(app)
    if not isinstance(receipt, SliderReceipt):
        raise TypeError("expected SliderReceipt")
    if receipt._namespace is not getattr(app, "_slider_namespace", None) or receipt._namespace is None:
        raise ValueError("receipt belongs to another app/library instance")
    slot, generation = receipt.node
    wire = _Receipt(56, 1, _uint(receipt.app_epoch), _Handle(_uint(slot, 32), _uint(generation, 32)),
        *(_uint(getattr(receipt, n)) for n in ("source_generation", "configuration_revision", "submission_frame", "request_sequence")))
    pair = _value(value)
    app._api.check(api.accept_slider_request_v1(app._ptr, 56, C.byref(wire), 24, C.byref(pair)))


def dispatch(app):
    if getattr(app, "_slider_dispatching", False):
        raise RuntimeError("nested controlled Slider dispatch is unavailable")
    app._slider_dispatching = True
    count = 0
    try:
        for request in poll(app):
            node = app._by_handle.get(request.receipt.node)
            callback = node._request_callback if isinstance(node, SliderModel) else None
            if callback is not None:
                callback(request)
            count += 1
    finally:
        app._slider_dispatching = False
    return count


class SliderModel(Widget):
    def __init__(self, key: str | None, config: SliderConfig,
                 confirmed: SliderSingle | SliderRange, text: str = "Selection") -> None:
        super().__init__("slider_model", key, text)
        _config(config)
        _value(confirmed, config.domain)
        self._slider_config, self._confirmed = config, confirmed
        self._request_callback = None

    def on_request(self, callback: Callable[[SliderRequest], object] | None) -> SliderModel:
        if callback is not None and not callable(callback):
            raise TypeError("request handler must be callable or None")
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        self._request_callback = callback
        return self

    def configure(self, config: SliderConfig, confirmed: SliderSingle | SliderRange) -> SliderModel:
        c, value = _config(config), _value(confirmed, config.domain)
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            app = self._app
            app._api.check(_api(app).slider_model_configure_v1(app._ptr, self._handle,
                72, C.byref(c), 24, C.byref(value)))
        self._slider_config, self._confirmed = config, confirmed
        return self

    def set_value(self, confirmed: SliderSingle | SliderRange) -> SliderModel:
        value = _value(confirmed, self._slider_config.domain)
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            app = self._app
            app._api.check(_api(app).slider_model_set_value_v1(app._ptr, self._handle, 24, C.byref(value)))
        elif type(confirmed) is not type(self._confirmed):
            raise ValueError("set_value preserves kind; use configure to change it")
        self._confirmed = confirmed
        return self

    def get_value(self) -> SliderSingle | SliderRange:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return self._confirmed
        wire = _Value()
        app = self._app
        app._api.check(_api(app).slider_model_get_value_v1(app._ptr, self._handle, 24, C.byref(wire)))
        return _copy_value(wire)

    def get_config(self) -> SliderConfig:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return self._slider_config
        wire = _Config()
        app = self._app
        app._api.check(_api(app).slider_model_get_config_v1(app._ptr, self._handle, 72, C.byref(wire)))
        if wire.struct_size != 72 or wire.schema_version != 1 or wire.reserved or wire.flags & ~31:
            raise BridgeError(8, "unknown Slider config")
        return SliderConfig(_copy_domain(wire.domain), SliderOrientation(wire.orientation),
            SliderLayout(wire.layout), wire.length, wire.rem, wire.source_generation,
            *(bool(wire.flags & (1 << i)) for i in range(5)))

    def part_bounds(self, part: SliderPart) -> tuple[float, float, float, float] | None:
        if not isinstance(part, SliderPart):
            raise TypeError("expected SliderPart")
        if self._app is None:
            raise RuntimeError("Slider is not mounted")
        app, rect = self._app, _Rect()
        status = _api(app).slider_model_part_bounds_v1(app._ptr, self._handle, part, C.byref(rect))
        if status == 1:
            return None
        app._api.check(status)
        return rect.x, rect.y, rect.width, rect.height

    def presentation(self) -> SliderPresentation | None:
        if self._app is None:
            raise RuntimeError("Slider is not mounted")
        app, wire = self._app, _Presentation()
        status = _api(app).slider_model_get_presentation_v1(app._ptr, self._handle, 32, C.byref(wire))
        if status == 1:
            return None
        app._api.check(status)
        if wire.struct_size != 32 or wire.schema_version != 1 or wire.reserved or wire.render_status not in range(6):
            raise BridgeError(8, "unknown Slider presentation")
        return SliderPresentation(wire.render_status, wire.core_time, wire.next_wake_seconds)

    def remove(self) -> None:
        if self._app is None:
            raise RuntimeError("Slider is not mounted")
        app = self._app
        app._ensure_open()
        app._api.check(app._api.node_remove(app._ptr, self._handle))
        del app._by_handle[(self._handle.slot, self._handle.generation)]
        self._request_callback = None
        self._app = self._handle = None
        self._disposed = True


def slider_model(key: str | None, config: SliderConfig,
                 confirmed: SliderSingle | SliderRange, text: str = "Selection") -> SliderModel:
    return SliderModel(key, config, confirmed, text)


def mount_node(app, node, parent, key, handle):
    c, value = _config(node._slider_config), _value(node._confirmed, node._slider_config.domain)
    app._api.check(_api(app).slider_model_create_v1(app._ptr, parent, _utf8(key),
        72, C.byref(c), 24, C.byref(value), C.byref(handle)))


__all__ = ["SliderSingle", "SliderRange", "SliderDomain", "SliderConfig", "SliderScale",
    "SliderOrientation", "SliderLayout", "SliderPhase", "SliderReason", "SliderPart",
    "SliderReceipt", "SliderInput", "SliderRequest", "SliderPresentation", "SliderModel", "slider_model"]
