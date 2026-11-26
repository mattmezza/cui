"""Copied paint layers for retained Radio controls."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
from enum import IntEnum
import math


class RadioPaintLayer(IntEnum):
    BASE = 0
    CHECKED = 1
    HOVERED = 2
    PRESSED = 3
    FOCUSED = 4
    DISABLED = 5


@dataclass(frozen=True, kw_only=True)
class RadioPaint:
    """Override selected paint fields. None inherits, and RGBA accepts 0xRRGGBBAA."""

    root_background_rgba: int | None = None
    root_border_rgba: int | None = None
    root_border_width: float | None = None
    root_radius: float | None = None
    root_padding: float | None = None
    root_gap: float | None = None
    indicator_background_rgba: int | None = None
    indicator_border_rgba: int | None = None
    indicator_border_width: float | None = None
    indicator_radius: float | None = None
    indicator_extent: float | None = None
    mark_rgba: int | None = None
    label_rgba: int | None = None
    description_rgba: int | None = None
    label_size: float | None = None
    description_size: float | None = None
    label_weight: int | None = None
    description_weight: int | None = None


# The mask follows the public C flags, and the wire follows the separate record order.
_FIELDS = (
    "root_background_rgba", "root_border_rgba", "root_border_width", "root_radius",
    "root_padding", "root_gap", "indicator_background_rgba", "indicator_border_rgba",
    "indicator_border_width", "indicator_radius", "indicator_extent", "mark_rgba",
    "label_rgba", "description_rgba", "label_size", "description_size", "label_weight",
    "description_weight",
)
_ROOT_FIELDS = frozenset(_FIELDS[:6])
_DESCRIPTION_FIELDS = frozenset(("description_rgba", "description_size", "description_weight"))
_COLORS = (
    "root_background_rgba", "root_border_rgba", "indicator_background_rgba",
    "indicator_border_rgba", "mark_rgba", "label_rgba", "description_rgba",
)
_DIMENSIONS = {
    "root_border_width": (0, 64),
    "root_radius": (0, 256),
    "root_padding": (0, 256),
    "root_gap": (0, 256),
    "indicator_border_width": (0, 64),
    "indicator_radius": (0, 256),
    "indicator_extent": (4, 128),
    "label_size": (1, 128),
    "description_size": (1, 128),
}
_WEIGHTS = ("label_weight", "description_weight")
_BASE_FIELDS = (frozenset(_DIMENSIONS) - {"root_border_width", "indicator_border_width"}
                | frozenset(_WEIGHTS))


class _RadioPaintV1(C.Structure):
    _fields_ = ([(name, C.c_uint32) for name in ("struct_size", "schema_version", "fields", "reserved")]
                + [(name, C.c_uint32) for name in _COLORS]
                + [(name, C.c_float) for name in _DIMENSIONS]
                + [(name, C.c_uint32) for name in _WEIGHTS])


def bind_radio_paint(api, bind, handle_type):
    api.radio_set_paint = None
    if not hasattr(api.lib, "cui_bridge_radio_set_paint_v1"):
        return
    if (C.sizeof(_RadioPaintV1), _RadioPaintV1.root_border_width.offset,
            _RadioPaintV1.label_weight.offset) != (88, 44, 80):
        raise RuntimeError("bridge Radio paint V1 layout mismatch")
    api.radio_set_paint = bind("radio_set_paint_v1", C.c_int, C.c_void_p, handle_type,
                               C.c_uint32, C.POINTER(_RadioPaintV1))


def radio_paint_wire(kind: str, layer: RadioPaintLayer, paint: RadioPaint | None) -> _RadioPaintV1 | None:
    if kind not in {"radio", "radio_group", "item"}:
        raise ValueError("radio_paint requires Radio, RadioGroup or a direct RadioGroup Item")
    if not isinstance(layer, RadioPaintLayer):
        raise TypeError("layer must be a RadioPaintLayer")
    if kind == "radio_group" and layer not in (RadioPaintLayer.BASE, RadioPaintLayer.DISABLED):
        raise ValueError("RadioGroup frame accepts BASE and DISABLED paint only")
    if paint is None:
        return None
    if not isinstance(paint, RadioPaint):
        raise TypeError("paint must be a RadioPaint or None")
    wire = _RadioPaintV1(struct_size=C.sizeof(_RadioPaintV1), schema_version=1)
    for index, name in enumerate(_FIELDS):
        value = getattr(paint, name)
        if value is None:
            continue
        if kind == "radio_group" and name not in _ROOT_FIELDS:
            raise ValueError(f"{name} is unavailable on the RadioGroup frame")
        if kind != "item" and name in _DESCRIPTION_FIELDS:
            raise ValueError(f"{name} requires a direct RadioGroup Item")
        if layer != RadioPaintLayer.BASE and name in _BASE_FIELDS:
            raise ValueError(f"{name} requires the BASE layer")
        if name in _DIMENSIONS:
            low, high = _DIMENSIONS[name]
            if type(value) not in (int, float) or not low <= value <= high or not math.isfinite(value):
                raise ValueError(f"{name} must be a finite number from {low} to {high}")
        else:
            high = 3 if name in _WEIGHTS else 0xFFFFFFFF
            if type(value) is not int or not 0 <= value <= high:
                raise ValueError(f"{name} must be an integer from 0 to {high}")
        wire.fields |= 1 << index
        setattr(wire, name, value)
    return wire if wire.fields else None
