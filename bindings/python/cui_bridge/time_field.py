"""Owned localized TimeField bridge records; no Python editor/reducer or C borrows."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
from enum import IntEnum
import threading
from types import SimpleNamespace
from typing import Callable

from . import BridgeError, Widget, _Handle, _Time, _utf8


class TimeCycle(IntEnum):
    H23 = 0
    H12 = 1


class TimePrecision(IntEnum):
    MINUTE = 0
    SECOND = 1


class TimeTextStatus(IntEnum):
    OK = 0
    INVALID_ARGUMENT = 1
    INVALID_CONFIG = 2
    INVALID_SYNTAX = 3
    INVALID_VALUE = 4
    OUT_OF_RANGE = 5
    BUFFER_TOO_SMALL = 6


def _uint(value, bits=64, nonzero=False):
    if type(value) is not int or not int(nonzero) <= value < 1 << bits:
        raise ValueError(f"expected {'nonzero ' if nonzero else ''}uint{bits}")
    return value


@dataclass(frozen=True, order=True)
class TimeValue:
    hour: int
    minute: int
    second: int = 0

    def __post_init__(self):
        for name, limit in (("hour", 24), ("minute", 60), ("second", 60)):
            if type(getattr(self, name)) is not int or not 0 <= getattr(self, name) < limit:
                raise ValueError("time fields require integers in24/60/60 ranges")


@dataclass(frozen=True)
class TimeLocale:
    """Explicit exact UTF-8 vocabulary, not an OS locale or normalization policy."""
    digits: tuple[str, ...] = tuple("0123456789")
    separator: str = ":"
    period_separator: str = " "
    am: str = "AM"
    pm: str = "PM"

    def __post_init__(self):
        if not isinstance(self.digits, (tuple, list)) or len(self.digits) != 10:
            raise ValueError("locale needs ten digit tokens")
        object.__setattr__(self, "digits", tuple(self.digits))
        encoded = [_utf8(token, 16) for token in self.tokens]
        if any(not token for token in encoded):
            raise ValueError("locale tokens must be nonempty")
        if any(a.startswith(b) for i, a in enumerate(encoded) for j, b in enumerate(encoded) if i != j):
            raise ValueError("all fourteen tokens must be pairwise prefix-free")

    @property
    def tokens(self):
        return self.digits + (self.separator, self.period_separator, self.am, self.pm)


@dataclass(frozen=True)
class TimeFieldConfig:
    locale: TimeLocale = TimeLocale()
    cycle: TimeCycle = TimeCycle.H23
    precision: TimePrecision = TimePrecision.SECOND
    minimum: TimeValue | None = None
    maximum: TimeValue | None = None
    edit_generation: int = 1
    segment_names: tuple[str, ...] = ("", "", "", "")
    disabled: bool = False
    readonly: bool = False
    owner_invalid: bool = False

    def __post_init__(self):
        if not isinstance(self.locale, TimeLocale):
            raise TypeError("expected TimeLocale")
        if not isinstance(self.cycle, TimeCycle) or not isinstance(self.precision, TimePrecision):
            raise TypeError("expected TimeCycle and TimePrecision")
        _uint(self.edit_generation, nonzero=True)
        for value in (self.minimum, self.maximum):
            if value is not None:
                if not isinstance(value, TimeValue):
                    raise TypeError("bounds require TimeValue or None")
                if self.precision == TimePrecision.MINUTE and value.second:
                    raise ValueError("minute precision requires zero bound seconds")
        if self.minimum is not None and self.maximum is not None and self.minimum > self.maximum:
            raise ValueError("same-day minimum exceeds maximum")
        if not isinstance(self.segment_names, (tuple, list)) or len(self.segment_names) != 4:
            raise ValueError("segment_names needs Hour/Minute/Second/Period names")
        object.__setattr__(self, "segment_names", tuple(self.segment_names))
        for name in self.segment_names:
            _utf8(name, 64)
        for name in ("disabled", "readonly", "owner_invalid"):
            if type(getattr(self, name)) is not bool:
                raise TypeError("configuration flags require bool")


@dataclass(frozen=True)
class TimeFieldChange:
    handle: tuple[int, int]
    frame_sequence: int
    previous: TimeValue
    value: TimeValue
    validation: TimeTextStatus
    rejected_count: int
    frame_status: int


@dataclass(frozen=True)
class TimeFieldResult:
    handle: tuple[int, int]
    frame_sequence: int
    status: TimeTextStatus
    validation: TimeTextStatus
    changed: bool
    rejected_count: int
    previous: TimeValue
    value: TimeValue
    queued_change_count: int
    frame_status: int


class _Token(C.Structure):
    _fields_ = [("length", C.c_uint32), ("bytes", C.c_uint8 * 16)]


class _Name(C.Structure):
    _fields_ = [("length", C.c_uint32), ("bytes", C.c_uint8 * 64)]


class _Config(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "version", "flags", "cycle", "precision", "bounds_flags")] + [
        ("reserved", C.c_uint32 * 2), ("minimum", _Time), ("maximum", _Time),
        ("edit_generation", C.c_uint64), ("tokens", _Token * 14), ("names", _Name * 4)]


class _Result(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("version", C.c_uint32), ("reserved", C.c_uint32 * 2),
        ("node", _Handle), ("frame_sequence", C.c_uint64)] + [(n, C.c_uint32) for n in
        ("status", "validation", "changed", "rejected_count")] + [
        ("previous", _Time), ("value", _Time), ("queued_change_count", C.c_uint32), ("frame_status", C.c_uint32)]


class _Change(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "version", "frame_status", "reserved")] + [
        ("node", _Handle), ("frame_sequence", C.c_uint64), ("previous", _Time), ("value", _Time),
        ("validation", C.c_uint32), ("rejected_count", C.c_uint32)]


def bind_api(api, bind):
    specs = {
        "time_field_versions": (C.c_uint32,),
        **{f"time_field_{name}_size_v1": (C.c_uint32,) for name in ("config", "result", "change")},
        "time_field_enable_v1": (C.c_int, C.c_void_p, C.c_uint32),
        "time_field_configure_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "time_field_get_config_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "time_field_get_result_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Result)),
        "time_field_clear_validation_v1": (C.c_int, C.c_void_p, _Handle),
        "time_field_poll_change_v1": (C.c_int, C.c_void_p, C.POINTER(_Change)),
    }
    api.time_field = None
    if all(hasattr(api.lib, "cui_bridge_" + name) for name in specs):
        api.time_field = SimpleNamespace(**{name: bind(name, *spec) for name, spec in specs.items()})


def supported(app):
    app._ensure_open()
    api = app._api.time_field
    if api is None or not api.time_field_versions() & 1:
        return False
    return all(C.sizeof(wire) == size and getattr(api, f"time_field_{name}_size_v1")() == size
        for name, wire, size in (("config", _Config, 616), ("result", _Result, 80), ("change", _Change, 64)))


def _api(app):
    if not supported(app):
        raise RuntimeError("localized TimeField V1 optional API unavailable or incompatible")
    return app._api.time_field


def enable(app, capacity=4096):
    if type(capacity) is not int or not 0 <= capacity <= 4096:
        raise ValueError("TimeField capacity must be0..4096")
    api = _api(app)
    app._api.check(api.time_field_enable_v1(app._ptr, capacity))
    if not app._time_field_enabled:
        app._time_field_owner_thread = threading.get_ident()
    app._time_field_enabled = True
    return app


def _time(value):
    if not isinstance(value, TimeValue):
        raise TypeError("expected TimeValue")
    return _Time(value.hour, value.minute, value.second)


def _config(config):
    if not isinstance(config, TimeFieldConfig):
        raise TypeError("expected TimeFieldConfig")
    out = _Config(struct_size=616, version=1, flags=int(config.disabled) | (int(config.readonly) << 1) |
        (int(config.owner_invalid) << 2), cycle=config.cycle, precision=config.precision,
        bounds_flags=int(config.minimum is not None) | (int(config.maximum is not None) << 1),
        minimum=_time(config.minimum or TimeValue(0, 0)), maximum=_time(config.maximum or TimeValue(0, 0)),
        edit_generation=config.edit_generation)
    for wire, text in zip(out.tokens, config.locale.tokens):
        data = _utf8(text, 16)
        wire.length = len(data)
        wire.bytes[:len(data)] = data
    for wire, text in zip(out.names, config.segment_names):
        data = _utf8(text, 64)
        wire.length = len(data)
        wire.bytes[:len(data)] = data
    return out


def _copy_text(wire, empty=False):
    length = wire.length
    data = bytes(wire.bytes)
    if length > len(data) or (not empty and not length) or any(data[length:]) or b"\0" in data[:length]:
        raise ValueError("noncanonical token/name bytes")
    return data[:length].decode("utf-8", errors="strict")


def _copy_time(wire):
    return TimeValue(wire.hour, wire.minute, wire.second)


def _copy_config(wire):
    if wire.struct_size != 616 or wire.version != 1 or any(wire.reserved) or wire.flags & ~7 or wire.bounds_flags & ~3:
        raise ValueError("invalid config schema")
    low, high = _copy_time(wire.minimum), _copy_time(wire.maximum)
    if (not wire.bounds_flags & 1 and low != TimeValue(0, 0)) or (not wire.bounds_flags & 2 and high != TimeValue(0, 0)):
        raise ValueError("inactive bounds must be zero")
    tokens = tuple(_copy_text(token) for token in wire.tokens)
    locale = TimeLocale(tokens[:10], *tokens[10:])
    return TimeFieldConfig(locale, TimeCycle(wire.cycle), TimePrecision(wire.precision),
        low if wire.bounds_flags & 1 else None, high if wire.bounds_flags & 2 else None,
        wire.edit_generation, tuple(_copy_text(name, True) for name in wire.names),
        *(bool(wire.flags & (1 << i)) for i in range(3)))


def _record_common(wire, size):
    if wire.struct_size != size or wire.version != 1:
        raise ValueError("invalid record schema")
    if not wire.node.slot or not wire.node.generation or not wire.frame_sequence:
        raise ValueError("missing node/frame identity")
    if wire.frame_status not in (0, 2, 3, 4, 5, 6, 7, 8, 9, 10):
        raise ValueError("unknown/inactive frame status")
    return (wire.node.slot, wire.node.generation), wire.frame_sequence


def _copy_change(wire):
    handle, frame = _record_common(wire, 64)
    if wire.reserved or wire.validation not in (0, 3, 4, 5):
        raise ValueError("invalid change validation/reserved")
    previous, value = _copy_time(wire.previous), _copy_time(wire.value)
    if previous == value:
        raise ValueError("change must have a net value difference")
    return TimeFieldChange(handle, frame, previous, value, TimeTextStatus(wire.validation),
                           wire.rejected_count, wire.frame_status)


def _copy_result(wire):
    handle, frame = _record_common(wire, 80)
    if (any(wire.reserved) or wire.changed not in (0, 1) or wire.queued_change_count not in (0, 1) or
            wire.validation not in (0, 3, 4, 5) or wire.frame_status != 0):
        raise ValueError("invalid result flags/reserved")
    previous, value = _copy_time(wire.previous), _copy_time(wire.value)
    if bool(wire.changed) != (previous != value) or wire.queued_change_count > wire.changed:
        raise ValueError("incoherent result values/count")
    return TimeFieldResult(handle, frame, TimeTextStatus(wire.status), TimeTextStatus(wire.validation),
        bool(wire.changed), wire.rejected_count, previous, value, wire.queued_change_count, wire.frame_status)


def _decode(function, wire):
    try:
        return function(wire)
    except (ValueError, TypeError, UnicodeError, OverflowError) as error:
        raise BridgeError(8, "malformed localized TimeField record") from error


def _poll(app):
    api = _api(app)
    while app._ptr:
        wire = _Change(struct_size=64, version=1)
        status = api.time_field_poll_change_v1(app._ptr, C.byref(wire))
        if status == 1:
            return
        app._api.check(status)
        copied = _decode(_copy_change, wire)
        node = app._by_handle.get(copied.handle)
        if isinstance(node, LocalizedTimeField):
            yield copied  # Only owned immutable Python values cross user code.


def poll(app):
    if app._time_field_dispatching:
        raise RuntimeError("nested localized TimeField polling is unavailable")
    yield from _poll(app)


def dispatch(app):
    if app._time_field_dispatching or app._slider_dispatching:
        raise RuntimeError("nested typed dispatch is unavailable")
    app._time_field_dispatching = True
    count = 0
    try:
        for event in _poll(app):
            node = app._by_handle.get(event.handle)
            callback = node._time_callback if isinstance(node, LocalizedTimeField) else None
            if callback is not None:
                callback(event)
            count += 1
    finally:
        app._time_field_dispatching = False
    return count


class LocalizedTimeField(Widget):
    """Bridge-owned localized editor; time_value/current_time retain scalar semantics."""
    def __init__(self, key: str | None, config: TimeFieldConfig, value: TimeValue, text: str = ""):
        _config(config)
        _time(value)
        super().__init__("time_field", key, text)
        self._time_config, self._time = config, (value.hour, value.minute, value.second)
        self._time_callback = None

    def configure(self, config: TimeFieldConfig) -> LocalizedTimeField:
        wire = _config(config)
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            app = self._app
            app._api.check(_api(app).time_field_configure_v1(app._ptr, self._handle, C.byref(wire)))
        self._time_config = config  # Commit only after native success.
        return self

    def get_config(self) -> TimeFieldConfig:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return self._time_config
        app, wire = self._app, _Config(struct_size=616, version=1)
        app._api.check(_api(app).time_field_get_config_v1(app._ptr, self._handle, C.byref(wire)))
        return _decode(_copy_config, wire)

    def get_result(self) -> TimeFieldResult:
        if self._app is None:
            raise RuntimeError("localized TimeField is not mounted")
        app, wire = self._app, _Result(struct_size=80, version=1)
        app._api.check(_api(app).time_field_get_result_v1(app._ptr, self._handle, C.byref(wire)))
        copied = _decode(_copy_result, wire)
        if copied.handle != (self._handle.slot, self._handle.generation):
            raise BridgeError(8, "result belongs to another node")
        return copied

    def clear_validation(self) -> LocalizedTimeField:
        if self._app is None:
            raise RuntimeError("localized TimeField is not mounted")
        app = self._app
        app._api.check(_api(app).time_field_clear_validation_v1(app._ptr, self._handle))
        return self

    def get_value(self) -> TimeValue:
        return TimeValue(*self.current_time())

    def on_time_change(self, callback: Callable[[TimeFieldChange], object] | None) -> LocalizedTimeField:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if callback is not None and not callable(callback):
            raise TypeError("time change handler must be callable or None")
        self._time_callback = callback
        return self

    def on_change(self, callback):
        raise TypeError("localized TimeField uses on_time_change; generic CHANGE is the legacy route")

    def remove(self) -> None:
        if self._app is None:
            raise RuntimeError("localized TimeField is not mounted")
        app = self._app
        app._ensure_open()
        app._api.check(app._api.node_remove(app._ptr, self._handle))
        del app._by_handle[(self._handle.slot, self._handle.generation)]
        self._time_callback = None
        self._app = self._handle = None
        self._disposed = True


def localized_time_field(key: str | None, config: TimeFieldConfig = TimeFieldConfig(),
                         value: TimeValue = TimeValue(0, 0), text: str = "") -> LocalizedTimeField:
    return LocalizedTimeField(key, config, value, text)


def mount_node(app, node):
    wire = _config(node._time_config)
    app._api.check(_api(app).time_field_configure_v1(app._ptr, node._handle, C.byref(wire)))


__all__ = ["TimeValue", "TimeCycle", "TimePrecision", "TimeTextStatus", "TimeLocale", "TimeFieldConfig",
           "TimeFieldChange", "TimeFieldResult", "LocalizedTimeField", "localized_time_field"]
