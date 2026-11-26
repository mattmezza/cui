"""Copied retained Attachment presentation and decoded RGBA previews."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
from threading import current_thread
from typing import Literal

from . import _Handle

AttachmentStatus = Literal["pending", "uploading", "processing", "failed", "complete"]
STATUSES = {"pending": 0, "uploading": 1, "processing": 2, "failed": 3, "complete": 4}


class Text(C.Structure):
    _fields_ = [
        ("flags", C.c_uint32),
        ("weight", C.c_uint32),
        ("font_size", C.c_float),
        ("letter_spacing", C.c_float),
        ("color_rgba", C.c_uint32),
        ("reserved", C.c_uint32),
    ]


class Config(C.Structure):
    _fields_ = (
        [(name, C.c_uint32) for name in ("struct_size", "schema_version", "flags", "reserved")]
        + [("media_aspect", C.c_float)]
        + [(name, C.c_uint32) for name in ("title_status", "description_status", "reserved2")]
        + [("title", Text), ("description", Text)]
    )


def number(value, minimum, maximum):
    if type(value) not in (int, float) or not minimum <= value <= maximum:
        raise ValueError("invalid Attachment number")
    return value


@dataclass(frozen=True)
class AttachmentTextStyle:
    font_size: float = 0
    weight: int | None = None
    letter_spacing: float = 0
    italic: bool = False
    monospace: bool = False
    color_rgba: int | None = None

    def __post_init__(self):
        number(self.font_size, 0, 4096)
        number(self.letter_spacing, -4096, 4096)
        if self.weight is not None and (type(self.weight) is not int or not 0 <= self.weight <= 3):
            raise ValueError("invalid Attachment weight")
        if type(self.italic) is not bool or type(self.monospace) is not bool:
            raise TypeError("text flags must be bool")
        if self.color_rgba is not None and (
            type(self.color_rgba) is not int or not 0 <= self.color_rgba <= 0xffffffff
        ):
            raise ValueError("invalid RGBA")

    def _wire(self):
        styled = self.weight is not None or self.italic or self.monospace or self.letter_spacing != 0
        flags = (
            int(styled)
            | (2 if self.color_rgba is not None else 0)
            | (4 if self.italic else 0)
            | (8 if self.monospace else 0)
        )
        return Text(
            flags, self.weight or 0, self.font_size, self.letter_spacing, self.color_rgba or 0, 0
        )


@dataclass(frozen=True)
class AttachmentOptions:
    vertical: bool = False
    media: bool = False
    indeterminate: bool = False
    media_aspect: float = 0
    title_status: AttachmentStatus | None = None
    description_status: AttachmentStatus | None = None
    title: AttachmentTextStyle | None = None
    description: AttachmentTextStyle | None = None

    def __post_init__(self):
        for name in ("vertical", "media", "indeterminate"):
            if type(getattr(self, name)) is not bool:
                raise TypeError("Attachment flags must be bool")
        number(self.media_aspect, 0, 16)
        if self.media_aspect and self.media_aspect < 1 / 16:
            raise ValueError("invalid media aspect")
        for name in ("title_status", "description_status"):
            value = getattr(self, name)
            if value is not None and (type(value) is not str or value not in STATUSES):
                raise ValueError("invalid text status")
        for name in ("title", "description"):
            value = getattr(self, name)
            if value is not None and type(value) is not AttachmentTextStyle:
                raise TypeError("expected AttachmentTextStyle")

    def _wire(self):
        flags = (
            int(self.vertical)
            | int(self.media) << 1
            | int(self.indeterminate) << 2
            | int(self.title_status is not None) << 3
            | int(self.description_status is not None) << 4
        )
        return Config(
            C.sizeof(Config), 1, flags, 0, self.media_aspect,
            STATUSES.get(self.title_status, 0), STATUSES.get(self.description_status, 0), 0,
            (self.title or AttachmentTextStyle())._wire(),
            (self.description or AttachmentTextStyle())._wire(),
        )


def bind_api(api, bind):
    setters = (
        ("attachment_configure_v1", [C.POINTER(Config)]),
        ("attachment_set_preview_rgba_v1", [C.c_void_p, *([C.c_uint32] * 4)]),
    )
    for name, args in setters:
        setter = None
        if hasattr(api.lib, "cui_bridge_" + name):
            setter = bind(name, C.c_int, C.c_void_p, _Handle, *args)
        setattr(api, name, setter)


def supported(app, preview=False):
    app._ensure_open()
    name = "attachment_set_preview_rgba_v1" if preview else "attachment_configure_v1"
    return getattr(app._api, name) is not None


def apply(node, value, preview=False):
    app = node._app
    if current_thread() is not app._creation_thread:
        raise RuntimeError("Attachment requires owner thread")
    if not supported(app, preview):
        family = "preview" if preview else "configuration"
        raise RuntimeError(f"Attachment requires optional {family} bridge symbol")
    if preview:
        data, width, height, stride = value if value is not None else (None, 0, 0, 0)
        app._api.check(app._api.attachment_set_preview_rgba_v1(
            app._ptr, node._handle, data, width, height, stride, len(data) if data else 0
        ))
    else:
        wire = value._wire() if value is not None else None
        app._api.check(app._api.attachment_configure_v1(
            app._ptr, node._handle, C.byref(wire) if wire is not None else None
        ))


def target(node):
    if node._disposed:
        raise RuntimeError("Widget belongs to disposed App")
    if node.kind != "attachment":
        raise ValueError("requires Attachment")


def single_line(text):
    if not isinstance(text, str):
        raise TypeError("Attachment text must be a string")
    if "\r" in text or "\n" in text:
        raise ValueError("configured Attachment labels must be single-line")


def configure(node, value):
    target(node)
    if value is not None and type(value) is not AttachmentOptions:
        raise TypeError("expected AttachmentOptions or None")
    if value is not None:
        single_line(node._text)
        single_line(node._detail)
    if node._app:
        apply(node, value)
    node._attachment_options = value
    return node


def preview(node, pixels, width, height, stride=None):
    target(node)
    for value in (width, height):
        if type(value) is not int or not 1 <= value <= 2048:
            raise ValueError("invalid preview dimensions")
    stride = width * 4 if stride is None else stride
    if type(stride) is not int or not width * 4 <= stride <= 0xffffffff:
        raise ValueError("invalid preview stride")
    if not isinstance(pixels, (bytes, bytearray, memoryview)):
        raise TypeError("expected bytes-like preview")
    with memoryview(pixels) as view:
        if view.nbytes != stride * height or view.nbytes > 16 * 1024 * 1024:
            raise ValueError("invalid preview byte count")
        # ctypes borrows immutable bytes for the synchronous bridge copy. Other
        # buffers need only a bounded temporary copy once the node is mounted.
        data = pixels if isinstance(pixels, bytes) else view.tobytes()
    value = (data, width, height, stride)
    if node._app:
        apply(node, value, True)
        node._attachment_preview = None
    else:
        node._attachment_preview = value
    return node


def clear(node):
    target(node)
    if node._app:
        apply(node, None, True)
    node._attachment_preview = None
    return node


def preflight(node, app):
    for attr, preview in (("_attachment_options", False), ("_attachment_preview", True)):
        if getattr(node, attr, None) is not None and not supported(app, preview):
            raise RuntimeError("Attachment requires optional bridge symbol")


def mount(node):
    for attr, preview in (("_attachment_options", False), ("_attachment_preview", True)):
        value = getattr(node, attr, None)
        if value is not None:
            apply(node, value, preview)


def finish_mount(node):
    """Release pending bytes only after the entire mount transaction succeeds."""
    if node.kind == "attachment":
        node._attachment_preview = None
