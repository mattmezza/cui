"""Copied presentation for retained Accordion and Collapsible."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
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


def bind_api(api, bind):
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
    if node._disclosure_config is not None or node._disclosure_style is not None:
        require(app)
    if node._disclosure_style is not None and node.kind == "item":
        if parent is None or parent.kind != "accordion":
            raise ValueError("styled Item must be a direct Accordion child")


def mount(node):
    if node._disclosure_config is not None:
        apply(node, node._disclosure_config)
    if node._disclosure_style is not None:
        apply(node, node._disclosure_style, True)
