"""App-owned SVG resources, styled Icons, and configured Badge containers."""
from __future__ import annotations

import ctypes as C
import math
from dataclasses import dataclass
from enum import IntEnum
from types import SimpleNamespace

from . import BridgeError, DesignSize, Widget, _Handle, _Rect, _SIZE, _utf8


_U32 = C.c_uint32
_U64 = C.c_uint64
_F32 = C.c_float
_P = C.POINTER
_APP = C.c_void_p
_BUFFER = C.c_void_p
_TEXT = C.c_char_p
_HEADER = [("struct_size", _U32), ("schema_version", _U32)]
_FLOAT_MAX = 3.4028234663852886e38
_SVG_MAX = 1024 * 1024
_BITMAP_MAX = 64 * 1024 * 1024
_SIZE_T_MAX = (1 << (8 * C.sizeof(C.c_size_t))) - 1
_DEFAULT_TRANSFORM = (1, 0, 0, 1, 0, 0)


class IconSourceKind(IntEnum):
    EMPTY = 0
    NAME = 1
    PATH = 2
    DATA = 3


class IconError(IntEnum):
    OK = 0
    INVALID = 1
    NOT_FOUND = 2
    PARSE = 3
    LIMIT = 4
    ALLOCATION = 5


class IconAvailability(IntEnum):
    UNKNOWN = 0
    CATALOG_NOT_EMBEDDED = 1
    EMBEDDED = 2


class IconSize(IntEnum):
    INHERITED = 0
    XSMALL = 1
    SMALL = 2
    MEDIUM = 3
    LARGE = 4
    CUSTOM = 5


class IconAlign(IntEnum):
    START = 0
    CENTER = 1
    END = 2


class BadgeVariant(IntEnum):
    COUNT = 0
    DOT = 1
    ICON = 2


class BadgeSize(IntEnum):
    MEDIUM = 0
    SMALL = 1
    LARGE = 2
    XSMALL = 3


def _expect(value, cls, name):
    if not isinstance(value, cls):
        raise TypeError(f"{name} must be {cls.__name__}")


def _uint(value, name, limit):
    if type(value) is not int or not 0 <= value <= limit:
        raise ValueError(f"{name} must be an integer in [0, {limit}]")


def _float(value, name, minimum=0, maximum=_FLOAT_MAX):
    if type(value) not in (float, int) or not minimum <= value <= maximum:
        raise ValueError(f"{name} must be a finite number in [{minimum}, {maximum}]")
    narrowed = _F32(value).value
    if value and not narrowed:
        raise ValueError(f"{name} is too small for a bridge float")
    return narrowed


def _array(value, size, name, limits):
    if not isinstance(value, (list, tuple)) or len(value) != size:
        raise ValueError(f"{name} must contain {size} numbers")
    return tuple(_float(item, name, minimum, maximum)
                 for item, (minimum, maximum) in zip(value, limits))


def _transform(value):
    return _array(value, 6, "transform", [(-64, 64)] * 4 + [(-1000000, 1000000)] * 2)


def icon_rotation(radians: float) -> tuple[float, float, float, float, float, float]:
    """A rotation about the center of the Icon box."""
    radians = _float(radians, "radians", -_FLOAT_MAX)
    c, s = math.cos(radians), math.sin(radians)
    return (c, s, -s, c, 0, 0)


def _design(name):
    if not isinstance(name, str) or name not in _SIZE:
        raise ValueError("unknown CUI design size")
    return _SIZE[name]


def _bools(record, names):
    for name in names:
        if type(getattr(record, name)) is not bool:
            raise TypeError(f"{name} must be bool")


def _floats(record, names, minimum=0, maximum=_FLOAT_MAX, *, optional=False):
    for name in names:
        value = getattr(record, name)
        if value is not None or not optional:
            object.__setattr__(record, name, _float(value, name, minimum, maximum))


@dataclass(frozen=True)
class IconSource:
    kind: IconSourceKind = IconSourceKind.EMPTY
    text: str = ""

    def __post_init__(self):
        _expect(self.kind, IconSourceKind, "kind")
        _utf8(self.text, _SVG_MAX if self.kind == IconSourceKind.DATA else 4096)
        if self.kind == IconSourceKind.EMPTY and self.text:
            raise ValueError("an empty Icon source has no text")

    @classmethod
    def named(cls, name: str) -> IconSource:
        return cls(IconSourceKind.NAME, name)

    @classmethod
    def path(cls, path: str) -> IconSource:
        return cls(IconSourceKind.PATH, path)

    @classmethod
    def svg(cls, data: str | bytes) -> IconSource:
        if isinstance(data, bytes):
            if len(data) > _SVG_MAX:
                raise ValueError("SVG source exceeds 1 MiB")
            data = data.decode("utf-8")
        return cls(IconSourceKind.DATA, data)

    def _encode(self):
        return _utf8(self.text, _SVG_MAX if self.kind == IconSourceKind.DATA else 4096)


@dataclass(frozen=True)
class IconLimits:
    """Zero selects the corresponding native default."""
    max_svg_bytes: int = 0
    max_pixels: int = 0
    max_cache_bytes: int = 0
    max_versions: int = 0

    def __post_init__(self):
        for name, limit in zip(_LIMIT_FIELDS, (_SVG_MAX, 16 * 1024 * 1024, _BITMAP_MAX, 128)):
            _uint(getattr(self, name), name, limit)


_LIMIT_FIELDS = ("max_svg_bytes", "max_pixels", "max_cache_bytes", "max_versions")
_EDGES = tuple[float, float, float, float]
_AFFINE = tuple[float, float, float, float, float, float]
_RECT = tuple[float, float, float, float]
_BOX_BOOLS = ("row", "clip", "fixed_width", "fixed_height", "layout_only", "inert",
              "width_percent", "height_percent")
_BOX_OPTIONAL = ("corners", "border_edges", "opacity", "padding_edges")
_BOUNDS = ("min_width", "min_height", "max_width", "max_height")


@dataclass(frozen=True)
class IconStyle:
    """Options of the underlying Core box. Icon owns final width and height."""
    width: float = 0
    height: float = 0
    padding: float = 0
    gap: float = 0
    radius: float = 0
    background_rgba: int = 0
    border_rgba: int = 0
    border_width: float = 0
    align_x: IconAlign = IconAlign.START
    align_y: IconAlign = IconAlign.START
    offset_x: float = 0
    offset_y: float = 0
    corners: _EDGES | None = None  # TL, TR, BL, BR
    border_edges: _EDGES | None = None  # L, R, T, B
    max_width: float = 0
    max_height: float = 0
    opacity: float | None = None
    padding_edges: _EDGES | None = None  # L, R, T, B
    row: bool = False
    clip: bool = False
    fixed_width: bool = False
    fixed_height: bool = False
    layout_only: bool = False
    inert: bool = False
    width_percent: bool = False
    height_percent: bool = False

    def __post_init__(self):
        _bools(self, _BOX_BOOLS)
        for name in ("align_x", "align_y"):
            _expect(getattr(self, name), IconAlign, name)
        for name in ("background_rgba", "border_rgba"):
            _uint(getattr(self, name), name, 0xffffffff)
        _floats(self, ("width", "height", "offset_x", "offset_y"), -_FLOAT_MAX)
        _floats(self, ("opacity",), -_FLOAT_MAX, optional=True)
        _floats(self, ("padding", "gap", "radius", "border_width", "max_width", "max_height"))
        for name in ("corners", "border_edges", "padding_edges"):
            value = getattr(self, name)
            if value is not None:
                object.__setattr__(self, name, _array(value, 4, name, [(0, _FLOAT_MAX)] * 4))


@dataclass(frozen=True)
class IconConstraints:
    min_width: float | None = None
    min_height: float | None = None
    max_width: float | None = None
    max_height: float | None = None

    def __post_init__(self):
        _floats(self, _BOUNDS, 0, 4096, optional=True)


@dataclass(frozen=True)
class IconOptions:
    size: IconSize = IconSize.INHERITED
    design_size: DesignSize = "inherit"
    custom_size: float = 0
    inherited_text_size: float = 0
    width: float | None = None
    height: float | None = None
    opacity: float | None = None
    rem_size: float = 0
    color_rgba: int | None = None
    inherited_color_rgba: int | None = None
    transform: _AFFINE | None = None
    decorative: bool = False
    style: IconStyle | None = None
    constraints: IconConstraints | None = None
    debug_placeholder: bool = False

    def __post_init__(self):
        _expect(self.size, IconSize, "size")
        _design(self.design_size)
        _bools(self, ("decorative", "debug_placeholder"))
        _floats(self, ("custom_size", "inherited_text_size", "rem_size"), 0, 4096)
        _floats(self, ("width", "height"), 0, 4096, optional=True)
        _floats(self, ("opacity",), -_FLOAT_MAX, optional=True)
        for name in ("color_rgba", "inherited_color_rgba"):
            value = getattr(self, name)
            if value is not None:
                _uint(value, name, 0xffffffff)
        if self.transform is not None:
            object.__setattr__(self, "transform", _transform(self.transform))
        if self.style is not None:
            _expect(self.style, IconStyle, "style")
        if self.constraints is not None:
            _expect(self.constraints, IconConstraints, "constraints")


@dataclass(frozen=True)
class IconInfo:
    error: IconError
    references: int
    parse_count: int
    raster_count: int
    cache_bytes: int
    cache_versions: int
    source_revision: int
    source_kind: IconSourceKind


@dataclass(frozen=True)
class IconBitmap:
    width: int
    height: int
    stride: int
    revision: int
    paint_bounds: _RECT
    rgba: bytes


@dataclass(frozen=True)
class IconCatalogEntry:
    name: str
    path: str
    embedded_by_default: bool


@dataclass(frozen=True)
class IconResult:
    frame_sequence: int
    error: IconError
    frame_status: int


@dataclass(frozen=True)
class BadgeResult:
    frame_sequence: int
    submitted: bool
    icon_error: IconError | None
    frame_status: int


class _Limits(C.Structure):
    _fields_ = _HEADER + [("reserved", _U32 * 2)] + [(name, _U64) for name in _LIMIT_FIELDS]


class _Info(C.Structure):
    _fields_ = _HEADER + [("icon_error", _U32), ("reference_count", _U32)] + [
        (name, _U64) for name in ("parse_count", "raster_count", "cache_bytes",
                                "cache_versions", "source_revision")
    ] + [("source_kind", _U32), ("flags", _U32), ("handle", _Handle), ("reserved", _U64)]


class _Bitmap(C.Structure):
    _fields_ = _HEADER + [(name, _U32) for name in (
        "format", "reserved", "width", "height", "stride", "reserved2"
    )] + [("required_bytes", _U64), ("revision", _U64), ("paint", _Rect)]


class _Style(C.Structure):
    _fields_ = _HEADER + [("flags", _U32), ("reserved", _U32)] + [
        (name, _F32) for name in ("width", "height", "padding", "gap", "radius")
    ] + [("background_rgba", _U32), ("border_rgba", _U32), ("border_width", _F32),
         ("align_x", _U32), ("align_y", _U32), ("offset_x", _F32), ("offset_y", _F32),
         ("corners", _F32 * 4), ("border_edges", _F32 * 4), ("max_width", _F32),
         ("max_height", _F32), ("opacity", _F32), ("padding_edges", _F32 * 4),
         ("reserved_tail", _U32)]


class _Constraints(C.Structure):
    _fields_ = _HEADER + [("flags", _U32), ("reserved", _U32)] + [(n, _F32) for n in _BOUNDS]


class _Options(C.Structure):
    _fields_ = _HEADER + [("flags", _U32), ("size", _U32), ("design_size", _U32),
                          ("reserved", _U32 * 3)] + [
        (name, _F32) for name in ("custom_size", "inherited_text_size", "width", "height",
                                "opacity", "rem_size")
    ] + [("color_rgba", _U32), ("inherited_color_rgba", _U32), ("transform", _F32 * 6),
         ("style", _Style), ("constraints", _Constraints), ("reserved_tail", _U32 * 2)]


class _IconResult(C.Structure):
    _fields_ = _HEADER + [("icon_error", _U32), ("frame_status", _U32), ("node", _Handle),
                          ("frame_sequence", _U64), ("reserved", _U64)]


class _BadgeConfig(C.Structure):
    _fields_ = _HEADER + [("reserved", _U32 * 2)] + [(name, _U32) for name in (
        "variant", "size", "design_size", "flags"
    )] + [("count", _U64), ("maximum", _U64), ("color_rgba", _U32),
          ("reserved_tail", _U32), ("artwork", _Handle)]


class _BadgeResult(C.Structure):
    _fields_ = _HEADER + [("submitted", _U32), ("icon_error", _U32), ("node", _Handle),
                          ("frame_sequence", _U64), ("frame_status", _U32), ("flags", _U32)]


_RESOURCE_SPECS = {
    "versions": (_U32,),
    "record_size_v1": (_U32, _U32),
    "default_limits_v1": (C.c_int, _P(_Limits)),
    "create_v1": (C.c_int, _APP, _P(_Limits), _P(_Handle)),
    "destroy_v1": (C.c_int, _APP, _Handle),
    "get_info_v1": (C.c_int, _APP, _Handle, _P(_Info)),
    "get_limits_v1": (C.c_int, _APP, _Handle, _P(_Limits)),
    "set_limits_v1": (C.c_int, _APP, _Handle, _P(_Limits)),
    "set_source_v1": (C.c_int, _APP, _Handle, _U32, _TEXT, _U32),
    "get_source_v1": (C.c_int, _APP, _Handle, _P(_U32), _BUFFER, _U32, _P(_U32)),
    "reload_v1": (C.c_int, _APP, _Handle),
    "collect_v1": (C.c_int, _APP, _Handle),
    "asset_put_v1": (C.c_int, _APP, _Handle, _TEXT, _U32, _TEXT, _U32),
    "asset_remove_v1": (C.c_int, _APP, _Handle, _TEXT, _U32),
    "asset_clear_v1": (C.c_int, _APP, _Handle),
    "catalog_count_v1": (_U32,),
    "catalog_entry_v1": (C.c_int, _U32, _P(_U32), _BUFFER, _U32, _P(_U32),
                         _BUFFER, _U32, _P(_U32)),
    "query_v1": (C.c_int, _TEXT, _U32, _P(_U32)),
    "asset_copy_v1": (C.c_int, _TEXT, _U32, _BUFFER, _U32, _P(_U32)),
    "prepare_v1": (C.c_int, _APP, _Handle, _F32, _F32, _P(_F32), _P(_U32)),
    "bitmap_copy_v1": (C.c_int, _APP, _Handle, _P(_Bitmap), _BUFFER, _U32, _P(_U32)),
}
_PRESENTATION_SPECS = {
    "presentation_versions": (_U32,),
    "presentation_record_size_v1": (_U32, _U32),
    "configure_node_v1": (C.c_int, _APP, _Handle, _Handle, _P(_Options), _TEXT, _U32),
    "get_node_config_v1": (C.c_int, _APP, _Handle, _P(_Handle), _P(_Options)),
    "copy_node_name_v1": (C.c_int, _APP, _Handle, _BUFFER, _U32, _P(_U32)),
    "reset_node_v1": (C.c_int, _APP, _Handle),
    "get_node_result_v1": (C.c_int, _APP, _Handle, _P(_IconResult)),
}
_BADGE_SPECS = {
    "versions": (_U32,),
    "record_size_v1": (_U32, _U32),
    "default_config_v1": (C.c_int, _P(_BadgeConfig)),
    "create_v1": (C.c_int, _APP, _Handle, _TEXT, _P(_BadgeConfig), _P(_Options),
                  _TEXT, _U32, _TEXT, _U32, _P(_Handle)),
    "configure_v1": (C.c_int, _APP, _Handle, _P(_BadgeConfig), _P(_Options),
                     _TEXT, _U32, _TEXT, _U32),
    "get_config_v1": (C.c_int, _APP, _Handle, _P(_BadgeConfig), _P(_Options)),
    "copy_text_v1": (C.c_int, _APP, _Handle, _U32, _BUFFER, _U32, _P(_U32)),
    "get_result_v1": (C.c_int, _APP, _Handle, _P(_BadgeResult)),
    "indicator_bounds_v1": (C.c_int, _APP, _Handle, _P(_Rect)),
}


def _group(api, bind, prefix, specs, records, inquiry_prefix=""):
    if not all(hasattr(api.lib, "cui_bridge_" + prefix + key) for key in specs):
        return None
    group = SimpleNamespace(**{name: bind(prefix + name, *spec)
                               for name, spec in specs.items()})
    versions = getattr(group, inquiry_prefix + "versions")
    size = getattr(group, inquiry_prefix + "record_size_v1")
    if versions() & 1 and all(size(i) == C.sizeof(wire) == expected
                              for i, wire, expected in records):
        return group
    return None


def bind_api(api, bind):
    api.icon_resources = _group(api, bind, "icon_", _RESOURCE_SPECS, (
        (3, _Limits, 48), (4, _Info, 80), (5, _Bitmap, 64)))
    api.icon_presentation = _group(api, bind, "icon_", _PRESENTATION_SPECS, (
        (0, _Style, 128), (1, _Constraints, 32), (2, _Options, 256), (3, _IconResult, 40)),
        "presentation_") if api.icon_resources else None
    api.configured_badge = _group(api, bind, "badge_", _BADGE_SPECS, (
        (0, _BadgeConfig, 64), (1, _BadgeResult, 40))) if api.icon_presentation else None


def supported(app, group):
    app._ensure_open()
    return getattr(app._api, group) is not None


def _require(app, group):
    if not supported(app, group):
        raise RuntimeError(f"{group.replace('_', ' ')} V1 requires a complete, compatible bridge")
    app._ensure_icon_thread()
    return getattr(app._api, group)


def _record(cls, **fields):
    return cls(struct_size=C.sizeof(cls), schema_version=1, **fields)


def _header(wire):
    return wire.struct_size == C.sizeof(wire) and wire.schema_version == 1


def _malformed(what):
    return BridgeError(8, f"malformed {what} from bridge")


def _enum(cls, value):
    try:
        return cls(value)
    except ValueError as error:
        raise _malformed(cls.__name__) from error


def _style_wire(style):
    wire = _record(_Style)
    for name, bit in zip(_BOX_BOOLS, (0, 1, 4, 5, 6, 7, 9, 10)):
        if getattr(style, name):
            wire.flags |= 1 << bit
    for name, bit in zip(_BOX_OPTIONAL, (2, 3, 8, 11)):
        value = getattr(style, name)
        if value is not None:
            wire.flags |= 1 << bit
            setattr(wire, name, (_F32 * 4)(*value) if name != "opacity" else value)
    for name in ("width", "height", "padding", "gap", "radius", "background_rgba", "border_rgba",
                 "border_width", "align_x", "align_y", "offset_x", "offset_y", "max_width",
                 "max_height"):
        setattr(wire, name, getattr(style, name))
    return wire


def _options_wire(options):
    _expect(options, IconOptions, "options")
    wire = _record(_Options, size=options.size, design_size=_design(options.design_size),
                   custom_size=options.custom_size, inherited_text_size=options.inherited_text_size,
                   rem_size=options.rem_size)
    for bit, name in enumerate(("width", "height", "color_rgba", "inherited_color_rgba", "opacity")):
        value = getattr(options, name)
        if value is not None:
            wire.flags |= 1 << bit
            setattr(wire, name, value)
    if options.transform is not None:
        wire.flags |= 1 << 5
        wire.transform[:] = options.transform
    if options.decorative:
        wire.flags |= 1 << 6
    if options.style is not None:
        wire.flags |= 1 << 7
        wire.style = _style_wire(options.style)
    if options.constraints is not None:
        wire.flags |= 1 << 8
        wire.constraints = _record(_Constraints)
        for bit, name in enumerate(_BOUNDS):
            value = getattr(options.constraints, name)
            if value is not None:
                wire.constraints.flags |= 1 << bit
                setattr(wire.constraints, name, value)
    if options.debug_placeholder:
        wire.flags |= 1 << 9
    return wire


def _limits_wire(limits):
    _expect(limits, IconLimits, "limits")
    return _record(_Limits, **{name: getattr(limits, name) for name in _LIMIT_FIELDS})


def _read_limits(wire):
    if not _header(wire) or any(wire.reserved) or any(not getattr(wire, n) for n in _LIMIT_FIELDS):
        raise _malformed("Icon limits")
    try:
        return IconLimits(*(getattr(wire, name) for name in _LIMIT_FIELDS))
    except ValueError as error:
        raise _malformed("Icon limits") from error


def _text_from_buffer(buffer, size):
    if not 1 <= size <= len(buffer) or buffer[size - 1] != b"\0":
        raise _malformed("text copy")
    text = buffer[:size - 1]
    if b"\0" in text:
        raise _malformed("text copy")
    try:
        return text.decode("utf-8")
    except UnicodeDecodeError as error:
        raise _malformed("text copy") from error


def _copy_text(app, operation, args, limit=4096, *, allow_empty=False):
    size = _U32()
    status = operation(*args, None, 0, C.byref(size))
    if status == 1 and allow_empty:
        return None
    if status != 5:
        app._api.check(status)
        raise _malformed("text size query")
    if not 1 <= size.value <= limit + 1:
        raise _malformed("text size query")
    buffer = C.create_string_buffer(size.value)
    app._api.check(operation(*args, buffer, len(buffer), C.byref(size)))
    return _text_from_buffer(buffer, size.value)


def _handle_key(handle):
    return handle.slot, handle.generation


def _rect(wire):
    result = wire.x, wire.y, wire.width, wire.height
    if any(not math.isfinite(value) for value in result) or wire.width < 0 or wire.height < 0:
        raise _malformed("rectangle")
    return result


def default_limits(app):
    wire = _record(_Limits)
    app._api.check(_require(app, "icon_resources").default_limits_v1(C.byref(wire)))
    return _read_limits(wire)


def catalog_entry_count(app):
    return _require(app, "icon_resources").catalog_count_v1()


def catalog_entry(app, index):
    _uint(index, "catalog index", 0xffffffff)
    api = _require(app, "icon_resources")
    name, path = C.create_string_buffer(4097), C.create_string_buffer(4097)
    name_size, path_size, embedded = _U32(), _U32(), _U32()
    app._api.check(api.catalog_entry_v1(index, C.byref(embedded), name, len(name),
                                        C.byref(name_size), path, len(path), C.byref(path_size)))
    if embedded.value not in (0, 1):
        raise _malformed("catalog entry")
    return IconCatalogEntry(_text_from_buffer(name, name_size.value),
                            _text_from_buffer(path, path_size.value), bool(embedded.value))


def query_icon(app, name):
    text = _utf8(name)
    result = _U32()
    app._api.check(_require(app, "icon_resources").query_v1(text, len(text), C.byref(result)))
    return _enum(IconAvailability, result.value)


def copy_asset(app, name):
    text = _utf8(name)
    api = _require(app, "icon_resources")
    return _copy_text(app, api.asset_copy_v1, (text, len(text)), _SVG_MAX, allow_empty=True)


class IconResource:
    """An SVG resource owned by one App. Explicit close releases it while the app lives."""

    def __init__(self, app, source: IconSource = IconSource(), *, limits: IconLimits = IconLimits()):
        self._app = app
        _expect(source, IconSource, "source")
        self._handle = _Handle()
        wire = _limits_wire(limits)
        api = _require(app, "icon_resources")
        app._api.check(api.create_v1(app._ptr, C.byref(wire), C.byref(self._handle)))
        if not all(_handle_key(self._handle)):
            raise _malformed("Icon handle")
        app._icons_active = True
        if source.kind != IconSourceKind.EMPTY:
            try:
                self.set_source(source)
            except BaseException:
                self.close()
                raise

    def _live_handle(self, app):
        if self._app is not app:
            raise ValueError("Icon resource belongs to a different App")
        app._ensure_open()
        app._ensure_icon_thread()
        if self._handle is None:
            raise RuntimeError("Icon resource is closed")
        return self._handle

    def _call(self, name, *args):
        app = self._app
        handle = self._live_handle(app)
        api = _require(app, "icon_resources")
        return app._api.check(getattr(api, name)(app._ptr, handle, *args))

    def set_source(self, source: IconSource) -> IconResource:
        _expect(source, IconSource, "source")
        text = source._encode()
        self._call("set_source_v1", source.kind, text, len(text))
        return self

    def source(self) -> IconSource:
        app, kind = self._app, _U32()
        handle = self._live_handle(app)
        api = _require(app, "icon_resources")
        text = _copy_text(app, api.get_source_v1, (app._ptr, handle, C.byref(kind)), _SVG_MAX)
        try:
            return IconSource(_enum(IconSourceKind, kind.value), text)
        except ValueError as error:
            raise _malformed("Icon source") from error

    def info(self) -> IconInfo:
        wire = _record(_Info)
        self._call("get_info_v1", C.byref(wire))
        if (not _header(wire) or wire.flags or wire.reserved or not wire.source_revision or
                _handle_key(wire.handle) != _handle_key(self._handle)):
            raise _malformed("Icon info")
        return IconInfo(_enum(IconError, wire.icon_error), wire.reference_count, wire.parse_count,
                        wire.raster_count, wire.cache_bytes, wire.cache_versions,
                        wire.source_revision, _enum(IconSourceKind, wire.source_kind))

    def limits(self) -> IconLimits:
        wire = _record(_Limits)
        self._call("get_limits_v1", C.byref(wire))
        return _read_limits(wire)

    def set_limits(self, limits: IconLimits) -> IconResource:
        wire = _limits_wire(limits)
        self._call("set_limits_v1", C.byref(wire))
        return self

    def put_asset(self, path: str, svg: str | bytes) -> IconResource:
        key, data = _utf8(path), IconSource.svg(svg)._encode()
        self._call("asset_put_v1", key, len(key), data, len(data))
        return self

    def remove_asset(self, path: str) -> IconResource:
        key = _utf8(path)
        self._call("asset_remove_v1", key, len(key))
        return self

    def clear_assets(self) -> IconResource:
        self._call("asset_clear_v1")
        return self

    def reload(self) -> IconResource:
        self._call("reload_v1")
        return self

    def collect(self) -> IconResource:
        """Collect inactive masks after the App's synchronous render returns."""
        self._call("collect_v1")
        return self

    def prepare(self, width: float, height: float, *, transform: _AFFINE = _DEFAULT_TRANSFORM) -> IconError:
        """Prepare a headless bitmap, returning the exact artwork error."""
        w, h = _float(width, "width", 0, 4096), _float(height, "height", 0, 4096)
        matrix = (_F32 * 6)(*_transform(transform))
        result = _U32()
        self._call("prepare_v1", w, h, matrix, C.byref(result))
        return _enum(IconError, result.value)

    def bitmap(self) -> IconBitmap:
        """Copy straight RGBA8 and paint bounds. Check prepare's error before reading."""
        app = self._app
        handle = self._live_handle(app)
        api = _require(app, "icon_resources")
        wire, size = _record(_Bitmap), _U32()
        status = api.bitmap_copy_v1(app._ptr, handle, C.byref(wire), None, 0, C.byref(size))
        data = b""
        if status == 5:
            if not 1 <= size.value <= _BITMAP_MAX:
                raise _malformed("bitmap size query")
            buffer = C.create_string_buffer(size.value)
            app._api.check(api.bitmap_copy_v1(app._ptr, handle, C.byref(wire), buffer,
                                              len(buffer), C.byref(size)))
            if size.value != len(buffer):
                raise _malformed("bitmap size")
            data = buffer.raw
        else:
            app._api.check(status)
        if (not _header(wire) or wire.format or wire.reserved or wire.reserved2 or
                wire.required_bytes != len(data) or wire.stride != wire.width * 4 or
                wire.stride * wire.height != len(data) or
                (not data and any((wire.width, wire.height, wire.revision))) or
                (data and not wire.revision)):
            raise _malformed("Icon bitmap")
        return IconBitmap(wire.width, wire.height, wire.stride, wire.revision, _rect(wire.paint), data)

    def close(self) -> None:
        """Destroy this resource. Referencing Icons or Badges must release it first."""
        if self._handle is not None and self._app._ptr:
            self._call("destroy_v1")
        self._handle = None

    def __copy__(self):
        raise TypeError("IconResource cannot be copied")

    def __deepcopy__(self, memo):
        raise TypeError("IconResource cannot be copied")


@dataclass(frozen=True)
class IconConfig:
    resource: IconResource
    options: IconOptions = IconOptions()
    name: str = ""

    def __post_init__(self):
        _expect(self.resource, IconResource, "resource")
        _expect(self.options, IconOptions, "options")
        _utf8(self.name)


@dataclass(frozen=True)
class BadgeConfig:
    variant: BadgeVariant = BadgeVariant.COUNT
    count: int = 0
    maximum: int = 99
    size: BadgeSize = BadgeSize.MEDIUM
    design_size: DesignSize = "inherit"
    color_rgba: int | None = None
    name: str = ""
    named_icon: str = ""
    artwork: IconResource | None = None
    artwork_options: IconOptions | None = None

    def __post_init__(self):
        _expect(self.variant, BadgeVariant, "variant")
        _expect(self.size, BadgeSize, "size")
        _design(self.design_size)
        _uint(self.count, "count", _SIZE_T_MAX)
        _uint(self.maximum, "maximum", _SIZE_T_MAX)
        if self.color_rgba is not None:
            _uint(self.color_rgba, "color_rgba", 0xffffffff)
        _utf8(self.name)
        _utf8(self.named_icon)
        if self.artwork is not None:
            _expect(self.artwork, IconResource, "artwork")
            if self.artwork_options is None:
                object.__setattr__(self, "artwork_options", IconOptions())
            else:
                _expect(self.artwork_options, IconOptions, "artwork_options")
        elif self.artwork_options is not None:
            raise ValueError("artwork_options requires an owned IconResource")
        if self.variant == BadgeVariant.ICON:
            if bool(self.named_icon) == (self.artwork is not None):
                raise ValueError("Icon Badge requires exactly one named or owned artwork source")
        elif self.named_icon or self.artwork is not None:
            raise ValueError("only an Icon Badge accepts artwork")


def _badge_wire(config, app):
    handle = config.artwork._live_handle(app) if config.artwork is not None else _Handle()
    badge = _record(_BadgeConfig, variant=config.variant, size=config.size,
                    design_size=_design(config.design_size), flags=int(config.color_rgba is not None),
                    count=config.count, maximum=config.maximum, color_rgba=config.color_rgba or 0,
                    artwork=handle)
    art = _options_wire(config.artwork_options) if config.artwork_options is not None else None
    return badge, art


def _badge_arguments(config, app):
    badge, artwork = _badge_wire(config, app)
    name, named = _utf8(config.name), _utf8(config.named_icon)
    return (C.byref(badge), C.byref(artwork) if artwork is not None else None,
            name, len(name), named, len(named))


def _attached(widget):
    if widget._disposed:
        raise RuntimeError("Widget belongs to a disposed App or was removed")
    if widget._app is None:
        raise RuntimeError("Widget is not mounted")
    return widget._app, widget._handle


def _remove(widget):
    app, handle = _attached(widget)
    app._ensure_open()
    app._api.check(app._api.node_remove(app._ptr, handle))
    pending = [widget]
    while pending:
        current = pending.pop()
        if current._app is app:
            del app._by_handle[_handle_key(current._handle)]
            current._app = current._handle = None
            current._disposed = True
            if isinstance(current, (ConfiguredIcon, ConfiguredBadge)):
                current._release_icon_config()
            pending.extend(current._children)


def _valid_result(wire, handle):
    return (_header(wire) and _handle_key(wire.node) == _handle_key(handle)
            and wire.frame_sequence > 0 and wire.frame_status <= 10)


class ConfiguredIcon(Widget):
    """An SVG node using a retained, App-owned resource and typed options."""

    def __init__(self, key: str | None, resource: IconResource, options: IconOptions = IconOptions(),
                 *, name: str = "", fallback: str | None = None):
        if fallback is not None and not _utf8(fallback):
            raise ValueError("fallback must be a nonempty catalog name")
        super().__init__("icon", key, fallback or "")
        self._icon_fallback = fallback
        self._icon_config = IconConfig(resource, options, name)

    def _preflight_icon(self, app):
        _require(app, "icon_presentation")
        self._icon_config.resource._live_handle(app)
        if self._icon_fallback and query_icon(app, self._icon_fallback) != IconAvailability.EMBEDDED:
            raise ValueError("fallback must be a catalog icon embedded in the bridge")

    def _set_icon(self, app, handle, config):
        api = _require(app, "icon_presentation")
        resource = config.resource._live_handle(app)
        wire, name = _options_wire(config.options), _utf8(config.name)
        app._api.check(api.configure_node_v1(app._ptr, handle, resource, C.byref(wire),
                                             name, len(name)))

    def configure(self, resource: IconResource, options: IconOptions = IconOptions(),
                  *, name: str = "") -> ConfiguredIcon:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App or was removed")
        config = IconConfig(resource, options, name)
        if self._app is not None:
            self._set_icon(self._app, self._handle, config)
        self._icon_config = config
        return self

    def get_config(self) -> IconConfig | None:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App or was removed")
        config = self._icon_config
        if self._app is None:
            return config
        app, handle = self._app, self._handle
        api = _require(app, "icon_presentation")
        wire, resource = _record(_Options), _Handle()
        status = api.get_node_config_v1(app._ptr, handle, C.byref(resource), C.byref(wire))
        if status == 1 and config is None:
            return None
        app._api.check(status)
        if (config is None or _handle_key(resource) != _handle_key(config.resource._live_handle(app))
                or bytes(wire) != bytes(_options_wire(config.options))):
            raise _malformed("Icon configuration")
        if _copy_text(app, api.copy_node_name_v1, (app._ptr, handle)) != config.name:
            raise _malformed("Icon name")
        return config

    def result(self) -> IconResult | None:
        app, handle = _attached(self)
        api = _require(app, "icon_presentation")
        wire = _record(_IconResult)
        status = api.get_node_result_v1(app._ptr, handle, C.byref(wire))
        if status == 1:
            return None
        app._api.check(status)
        if not _valid_result(wire, handle) or wire.reserved:
            raise _malformed("Icon result")
        return IconResult(wire.frame_sequence, _enum(IconError, wire.icon_error), wire.frame_status)

    def reset(self) -> ConfiguredIcon:
        """Release the resource and restore the embedded fallback chosen at creation."""
        app, handle = _attached(self)
        if self._icon_fallback is None:
            raise ValueError("provide an embedded fallback when creating a resettable Icon")
        api = _require(app, "icon_presentation")
        app._api.check(api.reset_node_v1(app._ptr, handle))
        self._icon_config = None
        return self

    def remove(self) -> None:
        _remove(self)

    def _release_icon_config(self):
        self._icon_config = None

    def label(self, text: str) -> ConfiguredIcon:
        raise TypeError("use configure() with an accessible name")

    def visual_size(self, width: float, height: float) -> ConfiguredIcon:
        raise TypeError("set the size in IconOptions")


class ConfiguredBadge(Widget):
    """A zero-flow indicator over ordinary retained children."""

    def __init__(self, key: str | None, config: BadgeConfig = BadgeConfig()):
        _expect(config, BadgeConfig, "config")
        super().__init__("configured_badge", key, "")
        self._badge_config = config

    def _preflight_badge(self, app):
        _require(app, "configured_badge")
        if self._badge_config.artwork is not None:
            self._badge_config.artwork._live_handle(app)

    def _create_badge(self, app, parent, key, handle):
        api = _require(app, "configured_badge")
        args = _badge_arguments(self._badge_config, app)
        app._api.check(api.create_v1(app._ptr, parent, _utf8(key), *args, C.byref(handle)))
        app._icons_active = True

    def configure(self, config: BadgeConfig) -> ConfiguredBadge:
        _expect(config, BadgeConfig, "config")
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App or was removed")
        if self._app is not None:
            app = self._app
            api = _require(app, "configured_badge")
            args = _badge_arguments(config, app)
            app._api.check(api.configure_v1(app._ptr, self._handle, *args))
        self._badge_config = config
        return self

    def get_config(self) -> BadgeConfig:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App or was removed")
        config = self._badge_config
        if self._app is None:
            return config
        app, handle = self._app, self._handle
        api = _require(app, "configured_badge")
        badge, artwork = _record(_BadgeConfig), _record(_Options)
        app._api.check(api.get_config_v1(app._ptr, handle, C.byref(badge), C.byref(artwork)))
        expected, expected_art = _badge_wire(config, app)
        if expected_art is None:
            expected_art = _record(_Options)
        if bytes(badge) != bytes(expected) or bytes(artwork) != bytes(expected_art):
            raise _malformed("Badge configuration")
        for field, text in enumerate((config.name, config.named_icon)):
            if _copy_text(app, api.copy_text_v1, (app._ptr, handle, field)) != text:
                raise _malformed("Badge text")
        return config

    def result(self) -> BadgeResult | None:
        app, handle = _attached(self)
        api = _require(app, "configured_badge")
        wire = _record(_BadgeResult)
        status = api.get_result_v1(app._ptr, handle, C.byref(wire))
        if status == 1:
            return None
        app._api.check(status)
        if (not _valid_result(wire, handle) or wire.submitted > 1 or wire.flags & ~1
                or (not wire.flags and wire.icon_error)):
            raise _malformed("Badge result")
        error = _enum(IconError, wire.icon_error) if wire.flags else None
        return BadgeResult(wire.frame_sequence, bool(wire.submitted), error, wire.frame_status)

    def indicator_bounds(self) -> _RECT | None:
        app, handle = _attached(self)
        api = _require(app, "configured_badge")
        wire = _Rect()
        status = api.indicator_bounds_v1(app._ptr, handle, C.byref(wire))
        if status == 1:
            return None
        app._api.check(status)
        result = _rect(wire)
        if not wire.width or not wire.height:
            raise _malformed("Badge indicator bounds")
        return result

    def remove(self) -> None:
        """Release the Badge and its mounted subtree."""
        _remove(self)

    def _release_icon_config(self):
        self._badge_config = None

    def label(self, text: str) -> ConfiguredBadge:
        raise TypeError("set the indicator name in BadgeConfig")


def configured_icon(key: str | None, resource: IconResource, options: IconOptions = IconOptions(),
                    *, name: str = "", fallback: str | None = None) -> ConfiguredIcon:
    return ConfiguredIcon(key, resource, options, name=name, fallback=fallback)


def configured_badge(key: str | None, config: BadgeConfig = BadgeConfig()) -> ConfiguredBadge:
    return ConfiguredBadge(key, config)


__all__ = [
    "IconSourceKind", "IconError", "IconAvailability", "IconSize", "IconAlign", "BadgeVariant",
    "BadgeSize", "IconSource", "IconLimits", "IconStyle", "IconConstraints", "IconOptions",
    "IconInfo", "IconBitmap", "IconCatalogEntry", "IconResult", "BadgeResult", "IconResource",
    "IconConfig", "BadgeConfig", "ConfiguredIcon", "ConfiguredBadge", "configured_icon",
    "configured_badge", "icon_rotation",
]
