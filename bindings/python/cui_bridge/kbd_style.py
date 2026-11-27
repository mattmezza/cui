"""Validated copied Kbd style records using numeric RGBA colors."""
import ctypes as C
import math


class _Stop(C.Structure):
    _fields_ = [("position", C.c_float), ("color", C.c_uint32)]


class KbdStyleWire(C.Structure):
    _fields_ = [
        ("struct_size", C.c_uint32), ("schema_version", C.c_uint32),
        ("fields", C.c_uint32), ("reserved", C.c_uint32),
        ("background", C.c_uint32), ("border_color", C.c_uint32),
        ("text_color", C.c_uint32), ("weight", C.c_uint32),
        ("max_width", C.c_float), ("padding_x", C.c_float), ("padding_y", C.c_float),
        ("radius", C.c_float), ("border_width", C.c_float),
        ("font_size", C.c_float), ("line_height", C.c_float), ("stop_count", C.c_uint32),
        ("x0", C.c_float), ("y0", C.c_float), ("x1", C.c_float), ("y1", C.c_float),
        ("stops", _Stop * 8),
    ]


_DIMENSIONS = (
    ("max_width", 0, 4096, True), ("radius", 4, 4096, False),
    ("font_size", 7, 512, True), ("line_height", 8, 4096, False),
)
_FLAGS = (("fill", 1), ("wrap", 2), ("italic", 10), ("monospace", 11))
_COLORS = (("background", 5), ("text_color", 12))
_ALLOWED = {field[0] for field in _DIMENSIONS + _FLAGS + _COLORS} | {
    "padding_x", "padding_y", "border_width", "border_color", "weight", "text_gradient",
}


def _number(value, maximum=4096, positive=False):
    if (isinstance(value, bool) or not isinstance(value, (int, float))
            or not 0 <= value <= maximum or not math.isfinite(value)):
        raise ValueError("invalid Kbd style number")
    if positive and C.c_float(value).value == 0:
        raise ValueError("Kbd style number must be positive")
    return value


def _color(value):
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= 0xffffffff:
        raise ValueError("Kbd color must be numeric RGBA")
    return value


def _write_gradient(wire, gradient):
    if not isinstance(gradient, dict) or gradient.keys() - {"stops", "x0", "y0", "x1", "y1"}:
        raise ValueError("invalid Kbd gradient")
    stops = gradient.get("stops")
    if not isinstance(stops, (list, tuple)) or not 2 <= len(stops) <= 8:
        raise ValueError("Kbd gradient requires 2..8 stops")
    previous = 0
    for index, stop in enumerate(stops):
        if not isinstance(stop, dict) or stop.keys() != {"position", "color"}:
            raise ValueError("invalid Kbd gradient stop")
        position = _number(stop["position"], maximum=1)
        if position < previous:
            raise ValueError("Kbd stops must be ordered")
        previous = position
        wire.stops[index] = _Stop(position, _color(stop["color"]))
    for name, default in (("x0", 0), ("y0", 0), ("x1", 1), ("y1", 0)):
        setattr(wire, name, _number(gradient.get(name, default), maximum=1))
    if wire.x0 == wire.x1 and wire.y0 == wire.y1:
        raise ValueError("Kbd gradient endpoints must differ")
    wire.stop_count = len(stops)


def style_wire(style):
    if style is None:
        return None
    if not isinstance(style, dict):
        raise TypeError("Kbd style must be a dict or None")
    if style.keys() - _ALLOWED:
        raise ValueError("unknown Kbd style option")
    wire = KbdStyleWire(struct_size=C.sizeof(KbdStyleWire), schema_version=1)
    for name, bit, maximum, positive in _DIMENSIONS:
        if name in style:
            setattr(wire, name, _number(style[name], maximum, positive))
            wire.fields |= 1 << bit
    for name, bit in _FLAGS:
        if name not in style:
            continue
        if type(style[name]) is not bool:
            raise TypeError("Kbd flags must be bool")
        if style[name]:
            wire.fields |= 1 << bit
    for name, bit in _COLORS:
        if name in style:
            setattr(wire, name, _color(style[name]))
            wire.fields |= 1 << bit
    if "padding_x" in style or "padding_y" in style:
        wire.padding_x = _number(style.get("padding_x", 4))
        wire.padding_y = _number(style.get("padding_y", 2))
        wire.fields |= 1 << 3
    has_width = "border_width" in style
    has_color = "border_color" in style
    if has_width != has_color:
        raise ValueError("Kbd border requires width and color")
    if has_width:
        wire.border_width = _number(style["border_width"])
        wire.border_color = _color(style["border_color"])
        wire.fields |= 1 << 6
    if "weight" in style:
        weights = ("normal", "medium", "semibold", "bold")
        if style["weight"] not in weights:
            raise ValueError("invalid Kbd weight")
        wire.weight = weights.index(style["weight"])
        wire.fields |= 1 << 9
    if "text_gradient" in style:
        if "text_color" in style:
            raise ValueError("Kbd text color and gradient are exclusive")
        _write_gradient(wire, style["text_gradient"])
        wire.fields |= 1 << 13
    return wire if wire.fields else None
