"""Copied presentation for retained Accordion and Collapsible."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
from enum import IntEnum
from threading import current_thread
from . import _Handle, _SIZE, _utf8, DesignSize

CURSORS = (
    "default", "pointer", "text", "resize_horizontal", "resize_vertical", "move",
    "not_allowed", "crosshair", "grab", "grabbing", "zoom_in", "zoom_out", "wait", "help",
)
FLAGS = ("joined", "bordered", "single", "allow_collapse", "custom_card", "clip_card", "borderless")
DIMENSIONS = ("width", "card_radius", "card_border_width", "card_padding", "card_gap")
CARD_COLORS = ("card_background_rgba", "card_border_rgba")
TRIGGER_COLORS = ("hover_rgba", "pressed_rgba", "expanded_rgba", "disabled_rgba", "focus_rgba")
ICONS = ("collapsed_icon", "expanded_icon")


class Config(C.Structure):
    _fields_ = (
        [(name, C.c_uint32) for name in ("struct_size", "schema_version", "flags", "design_size")]
        + [(name, C.c_float) for name in DIMENSIONS]
        + [(name, C.c_uint32) for name in CARD_COLORS]
        + [(name, C.c_char * 64) for name in ICONS]
    )


class Style(C.Structure):
    _fields_ = [
        (name, C.c_uint32)
        for name in ("struct_size", "schema_version", "fields", "reserved", *TRIGGER_COLORS, "cursor")
    ]


def _color(value):
    if type(value) is not int or not 0 <= value <= 0xffffffff:
        raise ValueError("color must be uint32 RGBA")


@dataclass(frozen=True)
class DisclosureConfig:
    """Width -1 fills, 0 fits and a positive width sets logical pixels."""

    joined: bool = False
    bordered: bool = False
    single: bool = False
    allow_collapse: bool = False
    custom_card: bool = False
    clip_card: bool = False
    borderless: bool = False
    design_size: DesignSize | None = None
    width: float = 0
    card_radius: float = 0
    card_border_width: float = 0
    card_padding: float = 0
    card_gap: float = 0
    card_background_rgba: int = 0
    card_border_rgba: int = 0
    collapsed_icon: str = ""
    expanded_icon: str = ""

    def __post_init__(self):
        if any(type(getattr(self, name)) is not bool for name in FLAGS):
            raise TypeError("disclosure flags must be bool")
        if self.design_size is not None and self.design_size not in _SIZE:
            raise ValueError("invalid disclosure size")
        if self.clip_card and not self.custom_card:
            raise ValueError("clip_card requires custom_card")
        for name in DIMENSIONS:
            value = getattr(self, name)
            if type(value) not in (int, float) or not (
                (name == "width" and value == -1) or 0 <= value <= 4096
            ):
                raise ValueError("invalid disclosure dimension")
        for name in CARD_COLORS:
            _color(getattr(self, name))
        for name in ICONS:
            _utf8(getattr(self, name), 63)

    def _wire(self):
        flags = sum(1 << i for i, name in enumerate(FLAGS) if getattr(self, name))
        return Config(
            C.sizeof(Config), 1, flags, _SIZE.get(self.design_size, 0),
            *(getattr(self, name) for name in (*DIMENSIONS, *CARD_COLORS)),
            _utf8(self.collapsed_icon, 63), _utf8(self.expanded_icon, 63),
        )


@dataclass(frozen=True)
class DisclosureTriggerStyle:
    """Omitted fields inherit. Zero sets an explicitly transparent color."""

    hover_rgba: int | None = None
    pressed_rgba: int | None = None
    expanded_rgba: int | None = None
    disabled_rgba: int | None = None
    focus_rgba: int | None = None
    cursor: str | None = None

    def __post_init__(self):
        for name in TRIGGER_COLORS:
            value = getattr(self, name)
            if value is not None:
                _color(value)
        if self.cursor is not None and self.cursor not in CURSORS:
            raise ValueError("invalid disclosure cursor")

    def _wire(self):
        fields = sum(1 << i for i, name in enumerate((*TRIGGER_COLORS, "cursor"))
                     if getattr(self, name) is not None)
        return Style(
            C.sizeof(Style), 1, fields, 0,
            *(getattr(self, name) or 0 for name in TRIGGER_COLORS),
            CURSORS.index(self.cursor) if self.cursor is not None else 0,
        ) if fields else None


class Heading(C.Structure):
    _fields_ = (
        [(name, C.c_uint32) for name in ("struct_size", "schema_version", "fields", "reserved")]
        + [(name, C.c_float) for name in ("font_size", "line_height", "letter_spacing")]
        + [(name, C.c_uint32) for name in ("weight", "align", "text_flags", "title_rgba")]
        + [(name, C.c_float) for name in
           ("width", "height", "padding", "gap", "radius", "border_width")]
        + [(name, C.c_uint32) for name in ("background_rgba", "border_rgba")]
    )


class Divider(C.Structure):
    _fields_ = (
        [(name, C.c_uint32) for name in ("struct_size", "schema_version", "reserved0", "reserved1")]
        + [(name, C.c_float) for name in ("thickness", "inset_left", "inset_right")]
        + [("color_rgba", C.c_uint32)]
    )


class DisclosureHeadingLayer(IntEnum):
    """Heading layers resolved in normal, expanded, then disabled order."""

    NORMAL = 0
    EXPANDED = 1
    DISABLED = 2


def _dimension(value, minimum=0):
    if type(value) not in (int, float) or not minimum <= value <= 4096:
        raise ValueError('invalid disclosure dimension')


@dataclass(frozen=True)
class DisclosureHeadingBox:
    """Complete trigger row box. Width -1 fills and zero dimensions fit."""

    width: float = 0
    height: float = 0
    padding: float = 0
    gap: float = 0
    radius: float = 0
    border_width: float = 0
    background_rgba: int = 0
    border_rgba: int = 0

    def __post_init__(self):
        for n in ('width', 'height', 'padding', 'gap', 'radius', 'border_width'):
            v = getattr(self, n)
            if n == 'width' and type(v) in (int, float) and (v == -1):
                continue
            _dimension(v)
        _color(self.background_rgba)
        _color(self.border_rgba)


@dataclass(frozen=True)
class DisclosureTypography:
    """Complete title typography with an explicit positive font size."""

    font_size: float
    line_height: float = 0
    letter_spacing: float = 0
    weight: int = 0
    align: int = 0
    wrap: bool = False
    italic: bool = False
    monospace: bool = False

    def __post_init__(self):
        _dimension(self.font_size)
        if C.c_float(self.font_size).value <= 0:
            raise ValueError('font_size must be positive')
        _dimension(self.line_height)
        _dimension(self.letter_spacing, -4096)
        for n, maximum in (('weight', 3), ('align', 2)):
            if type(getattr(self, n)) is not int or not 0 <= getattr(self, n) <= maximum:
                raise ValueError('invalid typography enum')
        if any((type(getattr(self, n)) is not bool for n in ('wrap', 'italic', 'monospace'))):
            raise TypeError('typography flags must be bool')


@dataclass(frozen=True)
class DisclosureHeading:
    """Omitted heading fields inherit. An explicit RGBA zero is transparent."""

    box: DisclosureHeadingBox | None = None
    typography: DisclosureTypography | None = None
    title_rgba: int | None = None

    def __post_init__(self):
        records = ((self.box, DisclosureHeadingBox), (self.typography, DisclosureTypography))
        for value, kind in records:
            if value is not None and type(value) is not kind:
                raise TypeError('invalid heading record')
        if self.title_rgba is not None:
            _color(self.title_rgba)

    def _wire(self):
        r = Heading()
        r.struct_size = C.sizeof(r)
        r.schema_version = 1
        for bit, value in ((1, self.box), (2, self.typography)):
            if value is None:
                continue
            r.fields |= bit
            for n in value.__dataclass_fields__:
                if n not in ('wrap', 'italic', 'monospace'):
                    setattr(r, n, getattr(value, n))
        if self.typography:
            r.text_flags = sum(
                1 << index for index, name in enumerate(("wrap", "italic", "monospace"))
                if getattr(self.typography, name)
            )
        if self.title_rgba is not None:
            r.fields |= 4
            r.title_rgba = self.title_rgba
        return r if r.fields else None


@dataclass(frozen=True)
class DisclosureDivider:
    """Joined Accordion divider. Zero thickness omits the divider."""

    thickness: float = 0
    inset_left: float = 0
    inset_right: float = 0
    color_rgba: int = 0

    def __post_init__(self):
        for n in ('thickness', 'inset_left', 'inset_right'):
            _dimension(getattr(self, n))
        _color(self.color_rgba)

    def _wire(self):
        return Divider(
            C.sizeof(Divider), 1, 0, 0, self.thickness,
            self.inset_left, self.inset_right, self.color_rgba,
        )


def supports_optional(app, family):
    app._ensure_open()
    return getattr(app._api, family) is not None


def apply_optional(node, family, value, layer=None):
    app = node._app
    if current_thread() is not app._creation_thread:
        raise RuntimeError('disclosure requires owner thread')
    if not supports_optional(app, family):
        raise RuntimeError(f'{family} requires optional bridge symbol')
    r = value._wire() if value is not None else None
    args = [app._ptr, node._handle]
    if layer is not None:
        args.append(int(layer))
    args.append(C.byref(r) if r is not None else None)
    app._api.check(getattr(app._api, family)(*args))


def heading(node, layer, value):
    if node._disposed:
        raise RuntimeError('Widget belongs to disposed App')
    if type(layer) is not DisclosureHeadingLayer:
        raise TypeError('expected DisclosureHeadingLayer')
    if node.kind not in ('accordion', 'item', 'collapsible'):
        raise ValueError('invalid heading target')
    if value is not None and type(value) is not DisclosureHeading:
        raise TypeError('expected DisclosureHeading or None')
    if value is not None and value._wire() is None:
        value = None
    if node._app:
        apply_optional(node, 'disclosure_heading', value, layer)
    if not hasattr(node, '_disclosure_headings'):
        node._disclosure_headings = {}
    if value is None:
        node._disclosure_headings.pop(layer, None)
    else:
        node._disclosure_headings[layer] = value


def divider(node, value):
    if node._disposed:
        raise RuntimeError('Widget belongs to disposed App')
    if node.kind != 'accordion':
        raise ValueError('divider requires Accordion')
    if value is not None and type(value) is not DisclosureDivider:
        raise TypeError('expected DisclosureDivider or None')
    if node._app:
        apply_optional(node, 'disclosure_divider', value)
    node._disclosure_divider = value


def bind_api(api, bind):
    for family, record in (("disclosure_heading", Heading), ("disclosure_divider", Divider)):
        name = family.replace("disclosure_", "disclosure_set_") + "_v1"
        setter = None
        if hasattr(api.lib, "cui_bridge_" + name):
            arguments = [C.c_void_p, _Handle]
            if family == "disclosure_heading":
                arguments.append(C.c_uint32)
            setter = bind(name, C.c_int, *arguments, C.POINTER(record))
        setattr(api, family, setter)
    names = ("disclosure_configure_v1", "disclosure_set_trigger_style_v1")
    api.disclosure = None
    if all(hasattr(api.lib, "cui_bridge_" + name) for name in names):
        api.disclosure = tuple(
            bind(name, C.c_int, C.c_void_p, _Handle, C.POINTER(record))
            for name, record in zip(names, (Config, Style))
        )


def supported(app):
    app._ensure_open()
    return app._api.disclosure is not None


def require(app):
    if current_thread() is not app._creation_thread:
        raise RuntimeError("disclosure requires owner thread")
    if not supported(app):
        raise RuntimeError("disclosure presentation requires both optional bridge symbols")


def apply(node, value, style=False):
    app = node._app
    require(app)
    record = value._wire() if value is not None else None
    setter = app._api.disclosure[int(style)]
    app._api.check(setter(app._ptr, node._handle, C.byref(record) if record is not None else None))


def configure(node, config):
    if node._disposed:
        raise RuntimeError("Widget belongs to disposed App")
    if node.kind not in ("accordion", "collapsible"):
        raise ValueError("disclosure configuration requires Accordion or Collapsible")
    if config is not None:
        if not isinstance(config, DisclosureConfig):
            raise TypeError("expected DisclosureConfig or None")
        if node.kind == "collapsible" and any(getattr(config, name) for name in FLAGS[:4]):
            raise ValueError("Accordion flags require Accordion")
        if node.kind == "accordion" and config.borderless:
            raise ValueError("borderless requires Collapsible")
    if node._app:
        apply(node, config)
    node._disclosure_config = config


def trigger_style(node, style):
    if node._disposed:
        raise RuntimeError("Widget belongs to disposed App")
    if node.kind not in ("collapsible", "item"):
        raise ValueError("trigger style requires Collapsible or direct Accordion Item")
    if style is not None:
        if not isinstance(style, DisclosureTriggerStyle):
            raise TypeError("expected DisclosureTriggerStyle or None")
        if style._wire() is None:
            style = None
    if node._app:
        apply(node, style, True)
    node._disclosure_style = style


def preflight(node, parent, app):
    headings = getattr(node, "_disclosure_headings", {})
    divider_style = getattr(node, "_disclosure_divider", None)
    for family, active in (
        ("disclosure_heading", bool(headings)),
        ("disclosure_divider", divider_style is not None),
    ):
        if active and not supports_optional(app, family):
            raise RuntimeError(f"{family} requires optional bridge symbol")
    if headings and node.kind == "item" and (parent is None or parent.kind != "accordion"):
        raise ValueError("heading Item must be direct Accordion child")
    if node._disclosure_config is not None or node._disclosure_style is not None:
        require(app)
    if node._disclosure_style is not None and node.kind == "item":
        if parent is None or parent.kind != "accordion":
            raise ValueError("styled Item must be a direct Accordion child")


def mount(node):
    for layer, value in getattr(node, "_disclosure_headings", {}).items():
        apply_optional(node, "disclosure_heading", value, layer)
    if getattr(node, "_disclosure_divider", None) is not None:
        apply_optional(node, "disclosure_divider", node._disclosure_divider)
    if node._disclosure_config is not None:
        apply(node, node._disclosure_config)
    if node._disclosure_style is not None:
        apply(node, node._disclosure_style, True)
