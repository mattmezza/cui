"""Optional copied retained Rating presentation."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
from threading import current_thread
from types import SimpleNamespace
from typing import TYPE_CHECKING

from . import BridgeError, DesignSize, Widget, _Handle, _SIZE

if TYPE_CHECKING:
    from .icons import IconResource

_FLAGS = ("half_steps", "hover_preview", "step_down_filled", "clearable", "readonly")
_COLORS = ("fill_rgba", "outline_rgba", "preview_rgba")
_MASKS = ("outline_icon", "fill_icon")
_SIZES = {value: name for name, value in _SIZE.items() if value}
_BYTES = 48
_VERSION = 1


class _Config(C.Structure):
    _fields_ = [
        (name, C.c_uint32)
        for name in ("struct_size", "schema_version", "flags", "design_size")
    ] + [
        ("width", C.c_float),
        *((name, C.c_uint32) for name in _COLORS),
        *((name, _Handle) for name in _MASKS),
    ]


@dataclass(frozen=True)
class RatingConfig:
    """Copied presentation with optional app-owned masks and 0xRRGGBBAA colors."""

    half_steps: bool = False
    hover_preview: bool = False
    step_down_filled: bool = False
    clearable: bool = False
    readonly: bool = False
    design_size: DesignSize | None = None
    width: float = 0
    fill_rgba: int | None = None
    outline_rgba: int | None = None
    preview_rgba: int | None = None
    outline_icon: IconResource | None = None
    fill_icon: IconResource | None = None

    def __post_init__(self) -> None:
        from .icons import IconResource

        if any(type(getattr(self, name)) is not bool for name in _FLAGS):
            raise TypeError("Rating flags must be bool")
        size = self.design_size
        if size is not None and (not isinstance(size, str) or size not in _SIZE):
            raise ValueError("unsupported Rating design size")
        if size == "inherit":
            object.__setattr__(self, "design_size", None)
        if type(self.width) not in (int, float) or not (self.width == 0 or 1 <= self.width <= 4096):
            raise ValueError("Rating width must be zero or in [1, 4096]")
        object.__setattr__(self, "width", C.c_float(self.width).value)
        for name in _COLORS:
            color = getattr(self, name)
            if color is not None and (type(color) is not int or not 0 <= color <= 0xffffffff):
                raise ValueError("Rating colors must be uint32 RGBA or None")
        for name in _MASKS:
            resource = getattr(self, name)
            if resource is not None and not isinstance(resource, IconResource):
                raise TypeError("Rating masks must be IconResource or None")


def bind_api(api, bind) -> None:
    specs = {
        "config_size_v1": (C.c_uint32,),
        "configure_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "get_config_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
    }
    api.rating_options = None
    if all(hasattr(api.lib, "cui_bridge_rating_" + name) for name in specs):
        api.rating_options = SimpleNamespace(
            **{name: bind("rating_" + name, *spec) for name, spec in specs.items()}
        )


def supported(app) -> bool:
    app._ensure_open()
    api = app._api.rating_options
    return api is not None and api.config_size_v1() == C.sizeof(_Config) == _BYTES


def _require(app):
    if current_thread() is not app._creation_thread:
        raise RuntimeError("Rating requires the app's owner thread")
    if not supported(app):
        raise RuntimeError("Rating options require a complete, compatible bridge symbol group")
    return app._api.rating_options


def _mask(config, name, app) -> _Handle:
    resource = getattr(config, name, None)
    return resource._live_handle(app) if resource is not None and app is not None else _Handle()


def _encode(config: RatingConfig, app) -> _Config:
    if not isinstance(config, RatingConfig):
        raise TypeError("expected RatingConfig or None")
    flags = sum(1 << index for index, name in enumerate(_FLAGS) if getattr(config, name))
    flags |= sum(1 << (index + 5) for index, name in enumerate(_COLORS)
                 if getattr(config, name) is not None)
    size = 0 if config.design_size is None else _SIZE[config.design_size]
    return _Config(
        _BYTES, _VERSION, flags, size, config.width,
        *(getattr(config, name) or 0 for name in _COLORS),
        *(_mask(config, name, app) for name in _MASKS),
    )


def _decode(record: _Config, configured: RatingConfig | None, app) -> RatingConfig:
    malformed = BridgeError(8, "malformed Rating configuration")
    if (record.struct_size != _BYTES or record.schema_version != _VERSION
            or record.flags & ~255 or record.design_size not in _SIZE.values()):
        raise malformed
    for name in _MASKS:
        actual, expected = getattr(record, name), _mask(configured, name, app)
        if actual.slot != expected.slot or actual.generation != expected.generation:
            raise malformed
    options = {name: bool(record.flags & (1 << index)) for index, name in enumerate(_FLAGS)}
    for index, name in enumerate(_COLORS, start=5):
        color = getattr(record, name)
        if record.flags & (1 << index):
            options[name] = color
        elif color:
            raise malformed
    try:
        return RatingConfig(
            **options, design_size=_SIZES.get(record.design_size), width=record.width,
            **{name: getattr(configured, name, None) for name in _MASKS},
        )
    except (TypeError, ValueError) as error:
        raise malformed from error


class Rating(Widget):
    def __init__(self, key: str, text: str = ""):
        super().__init__("rating", key, text)
        self._rating_config: RatingConfig | None = None

    def configure(self, config: RatingConfig | None) -> Rating:
        """Copy presentation or restore the legacy integer presenter with None."""
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        app = self._app
        record = _encode(config, app) if config is not None else None
        if app is not None:
            api = _require(app)
            pointer = C.byref(record) if record is not None else None
            app._api.check(api.configure_v1(app._ptr, self._handle, pointer))
        self._rating_config = config
        return self

    def reset_config(self) -> Rating:
        return self.configure(None)

    def get_config(self) -> RatingConfig | None:
        """Copy the active options, or return None for an ordinary Rating."""
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        app = self._app
        if app is None:
            return self._rating_config
        if not supported(app):
            return None
        record = _Config(_BYTES, _VERSION)
        status = _require(app).get_config_v1(app._ptr, self._handle, C.byref(record))
        if status == 1:
            return None
        app._api.check(status)
        return _decode(record, self._rating_config, app)

    def _preflight_rating(self, app) -> None:
        if self._rating_config is not None:
            _require(app)
            _encode(self._rating_config, app)

    def _mount_rating(self) -> None:
        if self._rating_config is not None:
            self.configure(self._rating_config)
