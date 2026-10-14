"""Small retained Python authoring layer over CUI's portable bridge ABI v1.

The widget tree lives in Python. ``step`` submits it through real CUI components;
``dispatch`` invokes Python callbacks only after the C frame has returned.
"""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
import math
import os
from pathlib import Path
import time
from typing import Callable, Iterator


def _utf8(value: str) -> bytes:
    if not isinstance(value, str):
        raise TypeError("expected a string")
    if "\x00" in value:
        raise ValueError("NUL is not allowed in bridge strings")
    encoded = value.encode("utf-8")
    if len(encoded) > 4096:
        raise ValueError("bridge strings are limited to 4096 UTF-8 bytes")
    return encoded


class BridgeError(RuntimeError):
    """A rejected bridge operation with its C status code."""

    def __init__(self, code: int, message: str):
        super().__init__(f"CUI bridge {code}: {message}")
        self.code = code


class _Handle(C.Structure):
    _fields_ = [("slot", C.c_uint32), ("generation", C.c_uint32)]


class _Rect(C.Structure):
    _fields_ = [("x", C.c_float), ("y", C.c_float),
                ("width", C.c_float), ("height", C.c_float)]


class _Event(C.Structure):
    _fields_ = [("type", C.c_int), ("node", _Handle),
                ("frame_sequence", C.c_uint64)]


class _Options(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("abi_version", C.c_uint32),
                ("backend", C.c_int), ("max_nodes", C.c_uint32),
                ("max_events", C.c_uint32), ("width", C.c_float),
                ("height", C.c_float), ("title_utf8", C.c_char_p),
                ("font_path_utf8", C.c_char_p)]


class _API:
    def __init__(self, path: str | os.PathLike[str] | None):
        if path is None:
            path = os.environ.get("CUI_BRIDGE_LIBRARY")
        if not path:
            raise FileNotFoundError("Pass library= or set CUI_BRIDGE_LIBRARY to libcui_bridge.so")
        self.lib = C.CDLL(os.fspath(path))
        lib = self.lib
        def bind(name: str, restype, *argtypes):
            fn = getattr(lib, f"cui_bridge_{name}")
            fn.restype = restype
            fn.argtypes = list(argtypes)
            return fn
        self.abi_version = bind("abi_version", C.c_uint32)
        self.options_size = bind("options_size", C.c_uint32)
        self.status_message = bind("status_message", C.c_char_p, C.c_int)
        self.create = bind("create", C.c_void_p, C.POINTER(_Options), C.POINTER(C.c_int))
        self.destroy = bind("destroy", None, C.c_void_p)
        self.node_create = bind("node_create", C.c_int, C.c_void_p, _Handle,
                                C.c_int, C.c_char_p, C.POINTER(_Handle))
        self.node_remove = bind("node_remove", C.c_int, C.c_void_p, _Handle)
        self.set_text = bind("set_text", C.c_int, C.c_void_p, _Handle, C.c_char_p)
        self.set_gap = bind("set_gap", C.c_int, C.c_void_p, _Handle, C.c_float)
        self.set_variant = bind("set_button_variant", C.c_int, C.c_void_p, _Handle, C.c_int)
        self.set_size = bind("set_button_size", C.c_int, C.c_void_p, _Handle, C.c_int)
        self.set_rounding = bind("set_button_rounding", C.c_int, C.c_void_p,
                                 _Handle, C.c_int, C.c_float)
        self.set_disabled = bind("set_button_disabled", C.c_int, C.c_void_p, _Handle,
                                  C.c_uint32)
        self.set_hover = bind("set_button_hover_rgba", C.c_int, C.c_void_p, _Handle,
                              C.c_uint8, C.c_uint8, C.c_uint8, C.c_uint8)
        self.queue_pointer = bind("queue_pointer", C.c_int, C.c_void_p, C.c_int,
                                  C.c_float, C.c_float)
        self.queue_key = bind("queue_key", C.c_int, C.c_void_p, C.c_int, C.c_int,
                              C.c_uint32, C.c_uint32)
        self.step = bind("step", C.c_int, C.c_void_p, C.c_float)
        self.poll_event = bind("poll_event", C.c_int, C.c_void_p, C.POINTER(_Event))
        self.bounds = bind("node_bounds", C.c_int, C.c_void_p, _Handle, C.POINTER(_Rect))
        self.request_close = bind("request_close", C.c_int, C.c_void_p)
        if self.abi_version() != 1 or self.options_size() != C.sizeof(_Options):
            raise RuntimeError("Incompatible CUI bridge ABI")

    def check(self, status: int) -> None:
        if status != 0:
            message = self.status_message(status)
            raise BridgeError(status, message.decode("utf-8") if message else "unknown")


_SIZE = {name: i for i, name in enumerate(("inherit", "xs", "sm", "base", "md", "lg",
                                           "xl", "2xl", "3xl", "4xl", "5xl", "6xl",
                                           "7xl", "8xl", "9xl"))}
_KIND = {"row": 1, "column": 2, "label": 3, "button": 4}
_VARIANT = {"primary": 0, "secondary": 1, "outline": 2, "ghost": 3}
_ROUND = {"theme": 0, "none": 1, "small": 2, "large": 3, "custom": 4}


@dataclass(frozen=True)
class Event:
    type: str
    node: "Widget"
    frame_sequence: int


class Widget:
    def __init__(self, kind: str, key: str | None = None, text: str = ""):
        if kind not in _KIND:
            raise ValueError(kind)
        self.kind = kind
        self.key = key
        self._text = text
        self._children: list[Widget] = []
        self._gap = 0.0
        self._variant = "primary"
        self._size = "base"
        self._rounding = "theme"
        self._radius = 0.0
        self._disabled = False
        self._hover: tuple[int, int, int, int] | None = None
        self._callback: Callable[[Event], object] | None = None
        self._app: App | None = None
        self._handle: _Handle | None = None
        self._disposed = False

    def _change(self, name: str, *args) -> None:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return
        app = self._app
        app._ensure_open()
        assert self._handle is not None
        fn = getattr(app._api, name)
        app._api.check(fn(app._ptr, self._handle, *args))

    def children(self, *children: "Widget") -> "Widget":
        if self.kind not in ("row", "column"):
            raise ValueError("Only rows and columns accept children")
        if self._app is not None:
            raise RuntimeError("Mount children before attaching the tree")
        if any(not isinstance(child, Widget) for child in children):
            raise TypeError("children must be widgets")
        self._children.extend(children)
        return self

    def gap(self, px: float) -> "Widget":
        if self.kind not in ("row", "column"):
            raise ValueError("gap belongs to a row or column")
        if not math.isfinite(px) or px < 0 or px > 4096:
            raise ValueError("gap must be 0..4096 logical pixels")
        self._change("set_gap", float(px))
        self._gap = float(px)
        return self

    def gap_px(self, px: float) -> "Widget":
        return self.gap(px)

    def gap_units(self, units: float) -> "Widget":
        return self.gap(units * 4)

    def label(self, text: str) -> "Widget":
        if self.kind not in ("label", "button"):
            raise ValueError("label belongs to text or button")
        if not isinstance(text, str):
            raise TypeError("label must be a string")
        self._change("set_text", _utf8(text))
        self._text = text
        return self

    def variant(self, value: str) -> "Widget":
        if self.kind != "button" or value not in _VARIANT:
            raise ValueError("unsupported button variant")
        self._change("set_variant", _VARIANT[value])
        self._variant = value
        return self

    def primary(self) -> "Widget":
        return self.variant("primary")

    def secondary(self) -> "Widget":
        return self.variant("secondary")

    def outline(self) -> "Widget":
        return self.variant("outline")

    def ghost(self) -> "Widget":
        return self.variant("ghost")

    def size(self, value: str) -> "Widget":
        if self.kind != "button" or value not in _SIZE:
            raise ValueError("unsupported design size")
        self._change("set_size", _SIZE[value])
        self._size = value
        return self

    def rounded_md(self) -> "Widget":
        return self.rounding("theme")

    def rounding(self, value: str, radius: float = 0) -> "Widget":
        if self.kind != "button" or value not in _ROUND:
            raise ValueError("unsupported rounding")
        if value == "custom" and (not math.isfinite(radius) or radius < 0 or radius > 4096):
            raise ValueError("custom radius must be 0..4096 logical pixels")
        self._change("set_rounding", _ROUND[value], float(radius))
        self._rounding, self._radius = value, radius
        return self

    def disabled(self, value: bool = True) -> "Widget":
        if self.kind != "button":
            raise ValueError("disabled belongs to button")
        self._change("set_disabled", int(bool(value)))
        self._disabled = bool(value)
        return self

    def hover_color(self, color: str | tuple[int, int, int, int]) -> "Widget":
        if self.kind != "button" or self._variant == "ghost":
            raise ValueError("hover color is supported on primary, secondary and outline buttons")
        if isinstance(color, str):
            if len(color) not in (7, 9) or not color.startswith("#"):
                raise ValueError("expected #RRGGBB or #RRGGBBAA")
            try:
                pieces = [int(color[i:i + 2], 16) for i in range(1, len(color), 2)]
            except ValueError as exc:
                raise ValueError("invalid hex color") from exc
            if len(pieces) == 3:
                pieces.append(255)
            rgba = tuple(pieces)
        else:
            rgba = tuple(color)
        if len(rgba) != 4 or any(not isinstance(x, int) or x < 0 or x > 255 for x in rgba):
            raise ValueError("expected four RGBA bytes")
        self._change("set_hover", *rgba)
        self._hover = rgba
        return self

    def on_click(self, callback: Callable[[Event], object]) -> "Widget":
        if self.kind != "button" or not callable(callback):
            raise ValueError("button callback must be callable")
        self._callback = callback
        return self


def row(key: str | None = None) -> Widget:
    return Widget("row", key)


def column(key: str | None = None) -> Widget:
    return Widget("column", key)


def label(text: str, key: str | None = None) -> Widget:
    return Widget("label", key, text)


def button(key: str) -> Widget:
    return Widget("button", key, key)


class App:
    def __init__(self, *, backend: str = "none", width: float = 480, height: float = 240,
                 title: str = "CUI Bridge", font_path: str | None = None,
                 library: str | os.PathLike[str] | None = None,
                 max_nodes: int = 256, max_events: int = 256):
        if backend not in ("none", "sdl"):
            raise ValueError("backend must be 'none' or 'sdl'")
        if backend == "sdl" and not font_path:
            raise ValueError("SDL requires an explicit font_path")
        if type(max_nodes) is not int or not 1 <= max_nodes <= 1024:
            raise ValueError("max_nodes must be an integer in 1..1024")
        if type(max_events) is not int or not 1 <= max_events <= 4096:
            raise ValueError("max_events must be an integer in 1..4096")
        self._api = _API(library)
        options = _Options(C.sizeof(_Options), 1, 0 if backend == "none" else 1,
                           max_nodes, max_events, width, height,
                           _utf8(title), _utf8(font_path) if font_path else None)
        status = C.c_int()
        self._ptr = self._api.create(C.byref(options), C.byref(status))
        if not self._ptr:
            self._api.check(status.value)
            raise RuntimeError("bridge create failed")
        self._root: Widget | None = None
        self._by_handle: dict[tuple[int, int], Widget] = {}

    def _ensure_open(self) -> None:
        if not self._ptr:
            raise RuntimeError("App is disposed")

    def mount(self, tree: Widget) -> "App":
        self._ensure_open()
        if self._root is not None:
            raise RuntimeError("App already has a root; dispose it before remounting")
        if not isinstance(tree, Widget) or tree.kind not in ("row", "column"):
            raise TypeError("root must be row() or column()")
        seen: set[int] = set()
        keys: set[str] = set()
        order: list[tuple[Widget, Widget | None, str]] = []
        def validate(node: Widget, parent: Widget | None, path: str, depth: int) -> None:
            if depth >= 128:
                raise ValueError("widget tree exceeds 128 levels")
            if id(node) in seen:
                raise ValueError("widget tree contains a cycle or shared node")
            if node._app is not None and node._app is not self:
                raise ValueError("widget belongs to a different App")
            if node._disposed:
                raise ValueError("widget belongs to a disposed App")
            seen.add(id(node))
            key = node.key if node.key is not None else f"{node.kind}@{path}"
            _utf8(key)
            if node.kind in ("label", "button"):
                _utf8(node._text)
            if key in keys:
                raise ValueError(f"duplicate key: {key}")
            keys.add(key)
            order.append((node, parent, key))
            for index, child in enumerate(node._children):
                validate(child, node, f"{path}/{index}", depth + 1)
        validate(tree, None, "root", 0)
        created: list[Widget] = []
        try:
            for node, parent, key in order:
                handle = _Handle()
                parent_handle = parent._handle if parent else _Handle()
                self._api.check(self._api.node_create(self._ptr, parent_handle, _KIND[node.kind],
                                                       _utf8(key), C.byref(handle)))
                node._app, node._handle = self, handle
                created.append(node)
                self._by_handle[(handle.slot, handle.generation)] = node
                if node.kind in ("label", "button"):
                    node._change("set_text", _utf8(node._text))
                if node.kind in ("row", "column"):
                    node._change("set_gap", node._gap)
                if node.kind == "button":
                    node._change("set_variant", _VARIANT[node._variant])
                    node._change("set_size", _SIZE[node._size])
                    node._change("set_rounding", _ROUND[node._rounding], node._radius)
                    node._change("set_disabled", int(node._disabled))
                    if node._hover:
                        node._change("set_hover", *node._hover)
        except Exception:
            if created:
                self._api.node_remove(self._ptr, created[0]._handle)
            for node in created:
                node._app = node._handle = None
            self._by_handle.clear()
            raise
        self._root = tree
        return self

    def step(self, dt: float = 1 / 60) -> None:
        self._ensure_open()
        self._api.check(self._api.step(self._ptr, dt))

    def queue_pointer(self, phase: str, x: float, y: float) -> None:
        self._ensure_open()
        phases = {"move": 0, "down": 1, "up": 2}
        if phase not in phases:
            raise ValueError("phase must be move/down/up")
        self._api.check(self._api.queue_pointer(self._ptr, phases[phase], x, y))

    def queue_key(self, phase: str, key: str, modifiers: int = 0, repeat: bool = False) -> None:
        self._ensure_open()
        phases = {"down": 0, "up": 1}
        keys = {"tab": 1, "enter": 2, "space": 3, "escape": 4,
                "left": 5, "right": 6, "up": 7, "down": 8}
        if phase not in phases or key not in keys:
            raise ValueError("unsupported key or phase")
        if type(modifiers) is not int or not 0 <= modifiers <= 15:
            raise ValueError("modifiers must contain only the four supported bits")
        self._api.check(self._api.queue_key(self._ptr, phases[phase], keys[key],
                                            modifiers, int(repeat)))

    def bounds(self, node: Widget) -> tuple[float, float, float, float]:
        self._ensure_open()
        if node._app is not self or node._handle is None:
            raise ValueError("widget does not belong to this App")
        rect = _Rect()
        self._api.check(self._api.bounds(self._ptr, node._handle, C.byref(rect)))
        return rect.x, rect.y, rect.width, rect.height

    def poll_events(self) -> Iterator[Event]:
        self._ensure_open()
        while True:
            if not self._ptr:
                return
            raw = _Event()
            status = self._api.poll_event(self._ptr, C.byref(raw))
            if status in (1, 2):
                return
            self._api.check(status)
            node = self._by_handle.get((raw.node.slot, raw.node.generation))
            if node is not None:
                yield Event("click", node, raw.frame_sequence)

    def dispatch(self) -> int:
        """Invoke callbacks after step; Python exceptions propagate outside C frames."""
        count = 0
        for event in self.poll_events():
            if event.node._callback:
                event.node._callback(event)
            count += 1
        return count

    def run(self, dt: float = 1 / 60) -> None:
        """Simple native loop; callers may instead integrate step/dispatch into their loop."""
        self._ensure_open()
        while True:
            try:
                self.step(dt)
            except BridgeError as exc:
                if exc.code == 2:
                    return
                raise
            self.dispatch()
            time.sleep(dt)

    def request_close(self) -> None:
        self._ensure_open()
        self._api.check(self._api.request_close(self._ptr))

    def close(self) -> None:
        if self._ptr:
            self._api.destroy(self._ptr)
            self._ptr = None
            for node in self._by_handle.values():
                node._app = node._handle = None
                node._disposed = True
            self._by_handle.clear()

    def __enter__(self) -> "App":
        return self

    def __exit__(self, *_exc) -> None:
        self.close()

    def __del__(self):
        if getattr(self, "_ptr", None):
            self.close()


__all__ = ["App", "Widget", "Event", "BridgeError", "row", "column", "label", "button"]
