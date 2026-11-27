"""Rotating Spinner authoring over the optional C bridge V1."""
from __future__ import annotations

import ctypes as C
import math
from dataclasses import dataclass
from enum import IntEnum
from types import SimpleNamespace

from . import BridgeError, Widget, _Handle


class SpinnerSize(IntEnum):
    CUSTOM = 0
    XSMALL = 1
    SMALL = 2
    MEDIUM = 3
    LARGE = 4


class SpinnerEasing(IntEnum):
    QUADRATIC = 0
    LINEAR = 1


@dataclass(frozen=True)
class SpinnerConfig:
    """Copied decorative rotating Icon style; embedded Loader by default."""
    size: SpinnerSize = SpinnerSize.MEDIUM
    pixels: float = 0.0
    rem_size: float = 0.0
    duration: float = 0.0
    rgba: int | None = None
    easing: SpinnerEasing = SpinnerEasing.QUADRATIC
    icon: str = ""
    reduced_motion: bool = False

    def __post_init__(self):
        if not isinstance(self.size, SpinnerSize) or not isinstance(self.easing, SpinnerEasing):
            raise TypeError("size and easing must be SpinnerSize and SpinnerEasing")
        if type(self.reduced_motion) is not bool:
            raise TypeError("reduced_motion must be bool")
        if self.rgba is not None and (type(self.rgba) is not int or not 0 <= self.rgba <= 0xFFFFFFFF):
            raise ValueError("rgba must be None or numeric 0xRRGGBBAA")
        if not isinstance(self.icon, str) or "\0" in self.icon or len(self.icon.encode("utf-8")) > 63:
            raise ValueError("icon must be an embedded catalog key of at most 63 UTF-8 bytes without NUL")
        for name, limit in (("pixels", 4096), ("rem_size", 4096), ("duration", 60)):
            value = getattr(self, name)
            if type(value) not in (float, int) or not 0 <= value <= limit:
                raise ValueError(f"{name} must be a number in [0, {limit}]")
            narrowed = C.c_float(value).value
            if value and not narrowed:
                raise ValueError(f"{name} is too small for a bridge float")
            object.__setattr__(self, name, narrowed)
        if self.size == SpinnerSize.CUSTOM:
            if not self.pixels or self.rem_size:
                raise ValueError("custom size requires positive pixels and rem_size=0")
        elif self.pixels or C.c_float((self.rem_size or 16) * (0, .75, .875, 1, 1.5)[self.size]).value > 4096:
            raise ValueError("named size requires pixels=0 and resolved size <=4096")


@dataclass(frozen=True)
class SpinnerPresentation:
    """Read-only sample from the most recent successful frame."""
    frame_sequence: int
    core_time: float
    next_wake_seconds: float
    phase: float


class _Config(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in ("struct_size", "schema_version", "flags", "size")] + [
        (name, C.c_float) for name in ("pixels", "rem_size", "duration")
    ] + [("rgba", C.c_uint32), ("easing", C.c_uint32), ("icon", C.c_char * 64)]


class _Presentation(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("schema_version", C.c_uint32),
                ("frame_sequence", C.c_uint64), ("core_time", C.c_double),
                ("next_wake_seconds", C.c_double), ("phase", C.c_float), ("reserved", C.c_uint32)]


def bind_api(api, bind):
    specs = {
        "versions": (C.c_uint32,),
        "config_size_v1": (C.c_uint32,),
        "presentation_size_v1": (C.c_uint32,),
        "configure_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "get_config_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "get_presentation_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Presentation)),
    }
    api.spinner_presentation = None
    if all(hasattr(api.lib, "cui_bridge_spinner_" + name) for name in specs):
        api.spinner_presentation = SimpleNamespace(
            **{name: bind("spinner_" + name, *spec) for name, spec in specs.items()})


def supported(app) -> bool:
    app._ensure_open()
    api = app._api.spinner_presentation
    return bool(api is not None and api.versions() & 1
                and api.config_size_v1() == C.sizeof(_Config) == 100
                and api.presentation_size_v1() == C.sizeof(_Presentation) == 40)


def require_api(app):
    if not supported(app):
        raise RuntimeError("rotating Spinner V1 requires a complete, compatible bridge symbol group")
    return app._api.spinner_presentation


def _config_wire(config: SpinnerConfig) -> _Config:
    if not isinstance(config, SpinnerConfig):
        raise TypeError("expected SpinnerConfig")
    flags = (1 if config.rgba is not None else 0) | (2 if config.reduced_motion else 0)
    return _Config(100, 1, flags, config.size, config.pixels, config.rem_size,
                   config.duration, config.rgba or 0, config.easing, config.icon.encode("utf-8"))


class RotatingSpinner(Widget):
    """Decorative rotating embedded Icon; compose beside an application status label."""
    def __init__(self, key: str | None, config: SpinnerConfig = SpinnerConfig()):
        if not isinstance(config, SpinnerConfig):
            raise TypeError("expected SpinnerConfig")
        super().__init__("spinner", key, "")
        self._spinner_config = config

    def configure(self, config: SpinnerConfig) -> RotatingSpinner:
        wire = _config_wire(config)
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            app = self._app
            status = require_api(app).configure_v1(app._ptr, self._handle, C.byref(wire))
            app._api.check(status)
        self._spinner_config = config
        return self

    def get_config(self) -> SpinnerConfig:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return self._spinner_config
        app, wire = self._app, _Config(100, 1)
        app._api.check(require_api(app).get_config_v1(app._ptr, self._handle, C.byref(wire)))
        if (wire.struct_size != 100 or wire.schema_version != 1 or wire.flags & ~3 or
                (not wire.flags & 1 and wire.rgba)):
            raise BridgeError(8, "malformed Spinner configuration")
        try:
            return SpinnerConfig(SpinnerSize(wire.size), wire.pixels, wire.rem_size, wire.duration,
                                 wire.rgba if wire.flags & 1 else None, SpinnerEasing(wire.easing),
                                 wire.icon.decode("utf-8"), bool(wire.flags & 2))
        except (ValueError, TypeError, UnicodeError) as error:
            raise BridgeError(8, "malformed Spinner configuration") from error

    def presentation(self) -> SpinnerPresentation | None:
        """Return the current painted sample, or None until that sample is available."""
        if self._app is None:
            raise RuntimeError("rotating Spinner is not mounted")
        app, wire = self._app, _Presentation(40, 1)
        status = require_api(app).get_presentation_v1(app._ptr, self._handle, C.byref(wire))
        if status == 1:  # CUI_BRIDGE_EMPTY
            return None
        app._api.check(status)
        if (wire.struct_size != 40 or wire.schema_version != 1 or not wire.frame_sequence
                or not math.isfinite(wire.core_time) or wire.core_time < 0
                or not math.isfinite(wire.next_wake_seconds)
                or (wire.next_wake_seconds != 0 and wire.next_wake_seconds <= wire.core_time)
                or not 0 <= wire.phase <= 1 or wire.reserved != 0):
            raise BridgeError(8, "malformed Spinner presentation")
        return SpinnerPresentation(wire.frame_sequence, wire.core_time, wire.next_wake_seconds,
                                    wire.phase)

    def visual_size(self, width: float, height: float) -> RotatingSpinner:
        raise TypeError("use SpinnerConfig to set the size of a rotating Spinner")

    def remove(self) -> None:
        """Release a mounted indicator permanently."""
        if self._app is None:
            raise RuntimeError("rotating Spinner is not mounted")
        app, handle = self._app, self._handle
        app._ensure_open()
        app._api.check(app._api.node_remove(app._ptr, handle))
        del app._by_handle[(handle.slot, handle.generation)]
        self._app = self._handle = None
        self._disposed = True


def rotating_spinner(key: str | None, config: SpinnerConfig = SpinnerConfig()) -> RotatingSpinner:
    return RotatingSpinner(key, config)


def mount_node(node):
    node.configure(node._spinner_config)


__all__ = ["SpinnerSize", "SpinnerEasing", "SpinnerConfig", "SpinnerPresentation",
           "RotatingSpinner", "rotating_spinner"]
