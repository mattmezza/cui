"""Responsive Skeleton authoring over the optional C bridge V1."""
from __future__ import annotations

import ctypes as C
import math
from dataclasses import dataclass
from enum import IntEnum
from types import SimpleNamespace

from . import BridgeError, Widget, _Handle


class SkeletonWidth(IntEnum):
    FIXED = 0
    FRACTION = 1
    GROW = 2


@dataclass(frozen=True)
class SkeletonConfig:
    """Owned placeholder style. Fractional width resolves against the parent."""
    width_mode: SkeletonWidth = SkeletonWidth.FRACTION
    width: float = 1.0
    height: float = 16.0
    max_width: float = 0.0
    radius: float = 0.0
    duration: float = 0.0
    background_rgba: int | None = None
    secondary: bool = False
    reduced_motion: bool = False

    def __post_init__(self) -> None:
        if not isinstance(self.width_mode, SkeletonWidth):
            raise TypeError("width_mode must be a SkeletonWidth")
        if type(self.secondary) is not bool or type(self.reduced_motion) is not bool:
            raise TypeError("secondary and reduced_motion must be bool")
        if self.background_rgba is not None and (
                type(self.background_rgba) is not int or not 0 <= self.background_rgba <= 0xFFFFFFFF):
            raise ValueError("background_rgba must be None or a numeric 0xRRGGBBAA color")
        width_limit = {SkeletonWidth.FIXED: 16384, SkeletonWidth.FRACTION: 1,
                       SkeletonWidth.GROW: 0}[self.width_mode]
        for name, limit in (("width", width_limit), ("height", 16384), ("max_width", 16384),
                            ("radius", 16384), ("duration", 60)):
            value = getattr(self, name)
            if type(value) not in (float, int) or not 0 <= value <= limit:
                raise ValueError(f"{name} must be a number in [0, {limit}]")
            narrowed = C.c_float(value).value
            if value and not narrowed:
                raise ValueError(f"{name} is too small for a bridge float")
            object.__setattr__(self, name, narrowed)
        if not self.height or (self.width_mode != SkeletonWidth.GROW and not self.width):
            raise ValueError("height and fixed/fractional widths must be positive")
        if self.width_mode == SkeletonWidth.FRACTION and self.max_width:
            raise ValueError("fractional width requires max_width=0")


@dataclass(frozen=True)
class SkeletonPresentation:
    """Read-only sample from the most recent successful frame."""
    frame_sequence: int
    core_time: float
    next_wake_seconds: float
    phase: float
    opacity: float


class _Config(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in ("struct_size", "schema_version", "flags", "width_mode")] + [
        (name, C.c_float) for name in ("width", "height", "max_width", "radius", "duration")
    ] + [("background_rgba", C.c_uint32)]


class _Presentation(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("schema_version", C.c_uint32),
                ("frame_sequence", C.c_uint64), ("core_time", C.c_double),
                ("next_wake_seconds", C.c_double), ("phase", C.c_float), ("opacity", C.c_float)]


def bind_api(api, bind):
    specs = {
        "versions": (C.c_uint32,),
        "config_size_v1": (C.c_uint32,),
        "presentation_size_v1": (C.c_uint32,),
        "configure_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "get_config_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "get_presentation_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Presentation)),
    }
    api.skeleton_presentation = None
    if all(hasattr(api.lib, "cui_bridge_skeleton_" + name) for name in specs):
        api.skeleton_presentation = SimpleNamespace(
            **{name: bind("skeleton_" + name, *spec) for name, spec in specs.items()})


def supported(app) -> bool:
    app._ensure_open()
    api = app._api.skeleton_presentation
    return bool(api is not None and api.versions() & 1
                and api.config_size_v1() == C.sizeof(_Config) == 40
                and api.presentation_size_v1() == C.sizeof(_Presentation) == 40)


def require_api(app):
    if not supported(app):
        raise RuntimeError("responsive Skeleton V1 requires a complete, compatible bridge symbol group")
    return app._api.skeleton_presentation


def _config_wire(config: SkeletonConfig) -> _Config:
    if not isinstance(config, SkeletonConfig):
        raise TypeError("expected SkeletonConfig")
    flags = (int(config.secondary) | (2 if config.background_rgba is not None else 0) |
             (4 if config.reduced_motion else 0))
    return _Config(40, 1, flags, config.width_mode, config.width, config.height,
                   config.max_width, config.radius, config.duration, config.background_rgba or 0)


class ResponsiveSkeleton(Widget):
    """Decorative, bridge-owned placeholder. Compose it inside a laid-out parent."""
    def __init__(self, key: str | None, config: SkeletonConfig = SkeletonConfig()):
        if not isinstance(config, SkeletonConfig):
            raise TypeError("expected SkeletonConfig")
        super().__init__("skeleton", key, "")
        self._skeleton_config = config

    def configure(self, config: SkeletonConfig) -> ResponsiveSkeleton:
        wire = _config_wire(config)
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            app = self._app
            status = require_api(app).configure_v1(app._ptr, self._handle, C.byref(wire))
            app._api.check(status)
        self._skeleton_config = config
        return self

    def get_config(self) -> SkeletonConfig:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return self._skeleton_config
        app, wire = self._app, _Config(40, 1)
        app._api.check(require_api(app).get_config_v1(app._ptr, self._handle, C.byref(wire)))
        if (wire.struct_size != 40 or wire.schema_version != 1 or wire.flags & ~7 or
                (not wire.flags & 2 and wire.background_rgba)):
            raise BridgeError(8, "malformed Skeleton configuration")
        try:
            return SkeletonConfig(SkeletonWidth(wire.width_mode), wire.width, wire.height,
                                  wire.max_width, wire.radius, wire.duration,
                                  wire.background_rgba if wire.flags & 2 else None,
                                  bool(wire.flags & 1), bool(wire.flags & 4))
        except (ValueError, TypeError) as error:
            raise BridgeError(8, "malformed Skeleton configuration") from error

    def presentation(self) -> SkeletonPresentation | None:
        """Return the current painted sample, or None until that sample is available."""
        if self._app is None:
            raise RuntimeError("responsive Skeleton is not mounted")
        app, wire = self._app, _Presentation(40, 1)
        status = require_api(app).get_presentation_v1(app._ptr, self._handle, C.byref(wire))
        if status == 1:  # CUI_BRIDGE_EMPTY
            return None
        app._api.check(status)
        if (wire.struct_size != 40 or wire.schema_version != 1 or not wire.frame_sequence
                or not math.isfinite(wire.core_time) or wire.core_time < 0
                or not math.isfinite(wire.next_wake_seconds)
                or (wire.next_wake_seconds != 0 and wire.next_wake_seconds <= wire.core_time)
                or not 0 <= wire.phase <= 1 or not .5 <= wire.opacity <= 1):
            raise BridgeError(8, "malformed Skeleton presentation")
        return SkeletonPresentation(wire.frame_sequence, wire.core_time, wire.next_wake_seconds,
                                    wire.phase, wire.opacity)

    def visual_size(self, width: float, height: float) -> ResponsiveSkeleton:
        raise TypeError("use SkeletonConfig to set the size of a responsive Skeleton")

    def remove(self) -> None:
        """Release a mounted placeholder permanently."""
        if self._app is None:
            raise RuntimeError("responsive Skeleton is not mounted")
        app, handle = self._app, self._handle
        app._ensure_open()
        app._api.check(app._api.node_remove(app._ptr, handle))
        del app._by_handle[(handle.slot, handle.generation)]
        self._app = self._handle = None
        self._disposed = True


def responsive_skeleton(key: str | None, config: SkeletonConfig = SkeletonConfig()) -> ResponsiveSkeleton:
    return ResponsiveSkeleton(key, config)


def mount_node(node):
    node.configure(node._skeleton_config)


__all__ = ["SkeletonWidth", "SkeletonConfig", "SkeletonPresentation",
           "ResponsiveSkeleton", "responsive_skeleton"]
