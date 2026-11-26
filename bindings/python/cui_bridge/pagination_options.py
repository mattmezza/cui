"""Optional V1 presentation of the existing retained Pagination widget."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
from threading import current_thread
from types import SimpleNamespace

from . import BridgeError, DesignSize, Widget, _Handle, _SIZE


_LABEL_BYTES = 128
_LABELS = ("group", "first", "previous", "next", "last", "earlier_pages", "later_pages")
_FLAGS = ("counter", "icon_only", "chevrons", "jump_menu")


def _label_bytes(label: str) -> bytes:
    if not isinstance(label, str):
        raise TypeError("Pagination labels must be strings")
    if any(ord(ch) < 32 or ch in "\x7f\x85\u2028\u2029" for ch in label):
        raise ValueError("Pagination labels must be single-line text without control characters")
    encoded = label.encode("utf-8")
    if len(encoded) >= _LABEL_BYTES:
        raise ValueError("Pagination labels must be at most 127 UTF-8 bytes")
    return encoded


@dataclass(frozen=True)
class PaginationConfig:
    """Navigation and labels. Supply the container's inner width to allow wrapping."""

    counter: bool = False
    icon_only: bool = False
    chevrons: bool = False
    jump_menu: bool = False
    visible_pages: int = 0
    available_width: float = 0
    group: str = ""
    first: str = ""
    previous: str = ""
    next: str = ""
    last: str = ""
    earlier_pages: str = ""
    later_pages: str = ""

    def __post_init__(self) -> None:
        if any(type(getattr(self, name)) is not bool for name in _FLAGS):
            raise TypeError("Pagination flags must be bool")
        if (type(self.visible_pages) is not int or
                self.visible_pages != 0 and not 5 <= self.visible_pages <= 32):
            raise ValueError("visible_pages must be 0 or an integer from 5 to 32")
        width = self.available_width
        if type(width) not in (int, float) or not 0 <= width <= 32768:
            raise ValueError("available_width must be a number in [0, 32768]")
        narrowed = C.c_float(width).value
        if width and not narrowed:
            raise ValueError("available_width is too small for a bridge float")
        object.__setattr__(self, "available_width", narrowed)
        for name in _LABELS:
            _label_bytes(getattr(self, name))


class _Config(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in
                ("struct_size", "schema_version", "flags", "visible_pages")] + [
        ("available_width", C.c_float), *[(name, C.c_char * _LABEL_BYTES) for name in _LABELS]
    ]


def bind_api(api, bind):
    specs = {
        "versions": (C.c_uint32,),
        "config_size_v1": (C.c_uint32,),
        "configure_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
        "get_config_v1": (C.c_int, C.c_void_p, _Handle, C.POINTER(_Config)),
    }
    api.pagination_custom_size = None
    custom = {"set_custom_size": (C.c_int, C.c_void_p, _Handle, C.c_float),
              "get_custom_size": (C.c_int, C.c_void_p, _Handle, C.POINTER(C.c_float))}
    if all(hasattr(api.lib, "cui_bridge_pagination_" + name) for name in custom):
        api.pagination_custom_size = SimpleNamespace(
            **{name: bind("pagination_" + name, *spec) for name, spec in custom.items()})
    api.pagination_options = None
    if all(hasattr(api.lib, "cui_bridge_pagination_" + name) for name in specs):
        api.pagination_options = SimpleNamespace(
            **{name: bind("pagination_" + name, *spec) for name, spec in specs.items()})


def supported(app) -> bool:
    app._ensure_open()
    api = app._api.pagination_options
    return bool(api is not None and api.versions() & 1 and
                api.config_size_v1() == C.sizeof(_Config) == 916)


def custom_supported(app) -> bool:
    app._ensure_open()
    return app._api.pagination_custom_size is not None


def require_custom_api(app):
    if current_thread() is not app._creation_thread:
        raise RuntimeError("Pagination custom size requires the app's owner thread")
    if not custom_supported(app):
        raise RuntimeError("Pagination custom size requires a complete, compatible bridge symbol group")
    return app._api.pagination_custom_size


def require_api(app):
    if current_thread() is not app._creation_thread:
        raise RuntimeError("Pagination options require the app's owner thread")
    if not supported(app):
        raise RuntimeError("Pagination options require a complete, compatible bridge symbol group")
    return app._api.pagination_options


def _config_wire(config: PaginationConfig) -> _Config:
    if not isinstance(config, PaginationConfig):
        raise TypeError("expected PaginationConfig or None")
    flags = sum(1 << index for index, name in enumerate(_FLAGS) if getattr(config, name))
    return _Config(916, 1, flags, config.visible_pages, config.available_width,
                   *(_label_bytes(getattr(config, name)) for name in _LABELS))


def _read_config(wire: _Config) -> PaginationConfig:
    try:
        if wire.struct_size != 916 or wire.schema_version != 1 or wire.flags & ~15:
            raise ValueError("invalid bridge header")
        labels = {}
        for name in _LABELS:
            encoded = getattr(wire, name)
            if len(encoded) >= _LABEL_BYTES:
                raise ValueError("unterminated label")
            labels[name] = encoded.decode("utf-8")
        return PaginationConfig(
            visible_pages=wire.visible_pages, available_width=wire.available_width, **labels,
            **{name: bool(wire.flags & (1 << index)) for index, name in enumerate(_FLAGS)})
    except (ValueError, TypeError) as error:
        raise BridgeError(8, "malformed Pagination configuration") from error


class Pagination(Widget):
    """Retained pager. Page commits use the ordinary on_change callback."""

    def __init__(self, key: str, text: str = ""):
        super().__init__("pagination", key, text)
        self._pagination_config: PaginationConfig | None = None
        self._pagination_custom_size = 0.0

    def custom_size(self, value: float) -> Pagination:
        """Set a logical pixel height, or zero to restore the named design size."""
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if type(value) not in (int, float) or not (value == 0 or 1 <= value <= 512):
            raise ValueError("custom_size must be zero or a number in [1, 512]")
        value = C.c_float(value).value
        if self._app is not None:
            app = self._app
            app._api.check(require_custom_api(app).set_custom_size(app._ptr, self._handle, value))
        self._pagination_custom_size = value
        return self

    def get_custom_size(self) -> float:
        """Return the custom logical pixel height; zero means named design size."""
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return self._pagination_custom_size
        app = self._app
        value = C.c_float()
        app._api.check(require_custom_api(app).get_custom_size(app._ptr, self._handle, C.byref(value)))
        return value.value

    def configure(self, config: PaginationConfig | None) -> Pagination:
        """Copy presentation options. Pass None to clear them."""
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        wire = _config_wire(config) if config is not None else None
        if self._app is not None:
            app = self._app
            status = require_api(app).configure_v1(app._ptr, self._handle,
                                                   C.byref(wire) if wire is not None else None)
            app._api.check(status)
        self._pagination_config = config
        return self

    def get_config(self) -> PaginationConfig | None:
        """Return the copied presentation configuration, or None for default presentation."""
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return self._pagination_config
        app = self._app
        if self._pagination_config is None and not supported(app):
            return None
        wire = _Config(916, 1)
        status = require_api(app).get_config_v1(app._ptr, self._handle, C.byref(wire))
        if status == 1:  # CUI_BRIDGE_EMPTY
            return None
        app._api.check(status)
        return _read_config(wire)

    def design_size(self, value: DesignSize) -> Pagination:
        """Use the shared design scale independently of presentation configuration."""
        if not isinstance(value, str) or value not in _SIZE:
            raise ValueError("unsupported design size")
        if self._app is not None:
            require_api(self._app)
        self._change("set_design_size", _SIZE[value])
        self._design_size = value
        return self

    def _preflight_pagination(self, app) -> None:
        if self._pagination_custom_size:
            require_custom_api(app)
        if self._pagination_config is not None or self._design_size is not None:
            require_api(app)

    def _mount_pagination(self) -> None:
        if self._pagination_custom_size:
            self.custom_size(self._pagination_custom_size)
        if self._pagination_config is not None:
            self.configure(self._pagination_config)


__all__ = ["Pagination", "PaginationConfig"]
